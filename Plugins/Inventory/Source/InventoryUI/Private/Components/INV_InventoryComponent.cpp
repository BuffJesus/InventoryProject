// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/INV_InventoryComponent.h"
#include "Components/INV_EquipmentComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interaction/INV_ContainerProvider.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "InventoryManagement/Transfer/INV_StorageTransferUtils.h"
#include "Items/Manifest/INV_ItemManifestRuntimeOps.h"
#include "InventoryManagement/Rules/INV_InventoryAddResolver.h"
#include "InventoryManagement/Utils/INV_DropLocationCalculator.h"
#include "EquipmentManagement/ProxyMesh/INV_ProxyMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Controllers/INV_PlayerController.h"
#include "UI/Base/INV_InventoryBase.h"

bool UINV_InventoryComponent::HasAuthorityOnOwner() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) && OwnerActor->HasAuthority();
}

void UINV_InventoryComponent::TrimRecentDropLocations()
{
	if (MaxRememberedDropLocations >= 0 && RecentDropLocations.Num() > MaxRememberedDropLocations)
	{
		const int32 NumToRemove = RecentDropLocations.Num() - MaxRememberedDropLocations;
		RecentDropLocations.RemoveAt(0, NumToRemove, EAllowShrinking::No);
	}
}

FVector UINV_InventoryComponent::ResolveVisualDropSeparation(const FVector& ProposedLocation)
{
	if (DropVisualSeparationDistance <= 0.f)
	{
		return ProposedLocation;
	}

	// Keep only a bounded history to avoid unbounded growth.
	TrimRecentDropLocations();

	auto IsFarEnoughFromAllRecentDrops = [this](const FVector& Candidate)
	{
		for (const FVector& ExistingLocation : RecentDropLocations)
		{
			const FVector Delta2D(Candidate.X - ExistingLocation.X, Candidate.Y - ExistingLocation.Y, 0.f);
			if (Delta2D.SizeSquared() < FMath::Square(DropVisualSeparationDistance))
			{
				return false;
			}
		}
		return true;
	};

	if (IsFarEnoughFromAllRecentDrops(ProposedLocation))
	{
		return ProposedLocation;
	}

	// Spiral-like 2D search around the proposed point.
	constexpr int32 MaxAttempts = 16;
	const float StepDistance = FMath::Max(10.f, DropVisualSeparationDistance);
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const float Ring = 1.f + Attempt / 6.f;
		const float AngleDeg = 360.f * (Attempt / static_cast<float>(MaxAttempts));
		const FVector OffsetDir = FVector::ForwardVector.RotateAngleAxis(AngleDeg, FVector::UpVector);
		const FVector Candidate = ProposedLocation + FVector(OffsetDir.X, OffsetDir.Y, 0.f) * StepDistance * Ring;
		if (IsFarEnoughFromAllRecentDrops(Candidate))
		{
			return Candidate;
		}
	}

	// Fallback: return original location if we could not find a better one.
	return ProposedLocation;
}

void UINV_InventoryComponent::RememberSuccessfulDropLocation(const FVector& DropLocation)
{
	RecentDropLocations.Add(DropLocation);
	TrimRecentDropLocations();
}

UINV_InventoryComponent::UINV_InventoryComponent() : InventoryFastArray(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	// Replicate items as registered subobjects.
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenuOpen = false;
}

void UINV_InventoryComponent::OnRegister()
{
	Super::OnRegister();
	ConfigureFastArrayCallbacks();
}

void UINV_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFastArrayCallbacks();
	OwningController = Cast<APlayerController>(GetOwner());
	EnsureLiveEquipmentComponent();
	
	ConstructInventory();
}

void UINV_InventoryComponent::EnsureLiveEquipmentComponent()
{
	if (!OwningController.IsValid())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (!IsValid(LiveEquipmentComponent))
	{
		LiveEquipmentComponent = OwnerActor->FindComponentByClass<UINV_EquipmentComponent>();
	}

	if (!IsValid(LiveEquipmentComponent))
	{
		LiveEquipmentComponent = NewObject<UINV_EquipmentComponent>(OwnerActor, TEXT("LiveEquipmentComponent"));
		if (!IsValid(LiveEquipmentComponent))
		{
			return;
		}

		OwnerActor->AddInstanceComponent(LiveEquipmentComponent);
		LiveEquipmentComponent->RegisterComponent();
	}

	USkeletalMeshComponent* AttachMesh = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(OwningController->GetPawn()))
	{
		AttachMesh = Character->GetMesh();
	}

	LiveEquipmentComponent->InitializeEquipmentContext(OwningController.Get(), AttachMesh, false);
}

void UINV_InventoryComponent::ConfigureFastArrayCallbacks()
{
	FINV_FastArrayCallbacks Callbacks;
	Callbacks.CreateItemFromPickup = [](UINV_ItemComponent* ItemComponent, AActor* OwningActor) -> UINV_InventoryItem*
	{
		if (!IsValid(ItemComponent) || !IsValid(OwningActor)) return nullptr;
		UINV_InventoryItem* Item = FINV_ItemManifestRuntimeOps::CreateItemFromManifest(
			ItemComponent->GetItemManifestMutable(),
			OwningActor);
		if (!IsValid(Item)) return nullptr;
		Item->SetItemRarityOptions(ItemComponent->IsItemRarityEnabled(), ItemComponent->GetItemRarityTag());
		return Item;
	};
	Callbacks.MatchesItemByTypeAndRarity = [](const UINV_InventoryItem* Item, const FGameplayTag& ItemType, const bool bUseItemRarity, const FGameplayTag& ItemRarityTag)
	{
		return UINV_InventoryItem::MatchesTypeAndRarity(Item, ItemType, bUseItemRarity, ItemRarityTag);
	};
	Callbacks.OnItemAdded = [this](UINV_InventoryItem* Item)
	{
		OnItemAdded.Broadcast(Item);
	};
	Callbacks.OnItemRemoved = [this](UINV_InventoryItem* Item)
	{
		OnItemRemoved.Broadcast(Item);
	};
	Callbacks.OnItemChanged = [this](UINV_InventoryItem* Item)
	{
		OnItemChanged.Broadcast(Item);
	};
	Callbacks.RegisterReplicatedSubObject = [this](UObject* SubObj)
	{
		AddRepSubObj(SubObj);
	};
	Callbacks.UnregisterReplicatedSubObject = [this](UObject* SubObj)
	{
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
		{
			RemoveReplicatedSubObject(SubObj);
		}
	};
	Callbacks.CanUseReplicationSubObjectList = [this]()
	{
		return IsUsingRegisteredSubObjectList() && IsReadyForReplication();
	};
	InventoryFastArray.SetCallbacks(MoveTemp(Callbacks));
}

void UINV_InventoryComponent::ToggleInventoryMenu()
{
	// Toggle UI visibility and input mode.
	bInventoryMenuOpen ? HandleInventoryMenu(ESlateVisibility::Collapsed, false)
		: HandleInventoryMenu(ESlateVisibility::Visible, true);
}

TArray<UINV_InventoryItem*> UINV_InventoryComponent::GetAllItems() const
{
	return InventoryFastArray.GetAllItems();
}

bool UINV_InventoryComponent::HasItem(UINV_InventoryItem* Item) const
{
	return IsValid(Item) && InventoryFastArray.GetAllItems().Contains(Item);
}

void UINV_InventoryComponent::RequestOpenContainer(AActor* ContainerActor)
{
	if (!IsValid(ContainerActor))
	{
		return;
	}

	if (HasAuthorityOnOwner())
	{
		Server_RequestOpenContainer_Implementation(ContainerActor);
		return;
	}

	Server_RequestOpenContainer(ContainerActor);
}

void UINV_InventoryComponent::CloseActiveContainer()
{
	if (!ActiveContainer.IsValid())
	{
		return;
	}

	if (HasAuthorityOnOwner())
	{
		Server_CloseActiveContainer_Implementation();
		return;
	}

	Server_CloseActiveContainer();
}

void UINV_InventoryComponent::OnInputMethodChanged(const bool bIsGamepadInput)
{
	if (!bInventoryMenuOpen) return;
	if (!OwningController.IsValid()) return;

	ApplyPointerInputMode(true);
	if (bIsGamepadInput && IsValid(Inventory))
	{
		Inventory->SetKeyboardFocus();
	}
}

void UINV_InventoryComponent::ApplyPointerInputMode(const bool bIsOpen) const
{
	if (!OwningController.IsValid()) return;

	const bool bUseMousePointerUI = bIsOpen;
	OwningController->SetShowMouseCursor(bUseMousePointerUI);
	OwningController->bEnableClickEvents = bUseMousePointerUI;
	OwningController->bEnableMouseOverEvents = bUseMousePointerUI;
}

void UINV_InventoryComponent::EnsureProxyMeshSpawned()
{
	if (!OwningController.IsValid()) return;
	if (!OwningController->IsLocalController()) return;
	if (!IsValid(ProxyMeshClass)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (!IsValid(CachedProxyMesh))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwningController.Get();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CachedProxyMesh = World->SpawnActor<AINV_ProxyMesh>(
			ProxyMeshClass,
			BuildProxyMeshSpawnTransform(),
			SpawnParams);
	}

	if (!IsValid(CachedProxyMesh)) return;

	CachedProxyMesh->SetActorTransform(BuildProxyMeshSpawnTransform());
	CachedProxyMesh->InitializeProxy(OwningController.Get());
}

void UINV_InventoryComponent::UpdateProxyMeshVisibility(const bool bVisible)
{
	if (!IsValid(CachedProxyMesh)) return;

	if (bVisible)
	{
		CachedProxyMesh->SetActorTransform(BuildProxyMeshSpawnTransform());
		CachedProxyMesh->SetPreviewVisible(true);
		return;
	}

	CachedProxyMesh->SetPreviewVisible(false);
}

FTransform UINV_InventoryComponent::BuildProxyMeshSpawnTransform() const
{
	if (!OwningController.IsValid())
	{
		return FTransform::Identity;
	}

	const APawn* ControlledPawn = OwningController->GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return FTransform::Identity;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector Forward = ControlledPawn->GetActorForwardVector();
	const FVector Left = -ControlledPawn->GetActorRightVector();
	const FVector SpawnLocation = PawnLocation
		+ Forward * ProxyPreviewForwardOffset
		+ Left * ProxyPreviewLeftOffset
		+ FVector::UpVector * ProxyPreviewVerticalOffset;

	const FVector LookDirection = PawnLocation - SpawnLocation;
	const FRotator SpawnRotation = LookDirection.IsNearlyZero()
		? ControlledPawn->GetActorRotation()
		: LookDirection.Rotation();

	return FTransform(SpawnRotation, SpawnLocation);
}

UINV_ContainerComponent* UINV_InventoryComponent::ResolveContainerFromActor(AActor* ContainerActor) const
{
	if (!IsValid(ContainerActor))
	{
		return nullptr;
	}

	if (ContainerActor->GetClass()->ImplementsInterface(UINV_ContainerProvider::StaticClass()))
	{
		return IINV_ContainerProvider::Execute_GetContainerComponent(ContainerActor);
	}

	return ContainerActor->FindComponentByClass<UINV_ContainerComponent>();
}

void UINV_InventoryComponent::SetActiveContainerLocal(UINV_ContainerComponent* NewContainer)
{
	UINV_ContainerComponent* PreviousContainer = ActiveContainer.Get();
	if (PreviousContainer == NewContainer)
	{
		return;
	}

	if (IsValid(PreviousContainer))
	{
		OnContainerClosed.Broadcast(PreviousContainer);
	}

	ActiveContainer = NewContainer;

	if (IsValid(NewContainer))
	{
		OnContainerOpened.Broadcast(NewContainer);
	}
}

FIntPoint UINV_InventoryComponent::GetPlayerGridSize(const EINV_ItemCategory Category) const
{
	switch (Category)
	{
	case EINV_ItemCategory::Equippable:
		return EquippableGridSize;
	case EINV_ItemCategory::Consumable:
		return ConsumableGridSize;
	case EINV_ItemCategory::Craftable:
		return CraftableGridSize;
	case EINV_ItemCategory::None:
	default:
		return EquippableGridSize;
	}
}

TArray<UINV_InventoryItem*> UINV_InventoryComponent::GetItemsForCategory(const EINV_ItemCategory Category, const UINV_InventoryItem* IgnoredItem) const
{
	TArray<UINV_InventoryItem*> Results;
	for (UINV_InventoryItem* Item : InventoryFastArray.GetAllItems())
	{
		if (!IsValid(Item) || Item == IgnoredItem)
		{
			continue;
		}

		if (Item->GetItemManifest().GetItemCategory() != Category)
		{
			continue;
		}

		Results.Add(Item);
	}

	return Results;
}

UINV_InventoryItem* UINV_InventoryComponent::CloneItemForOwner(UINV_InventoryItem* SourceItem, UObject* NewOuter, const int32 StackCount) const
{
	if (!IsValid(SourceItem) || !IsValid(NewOuter))
	{
		return nullptr;
	}

	FINV_ItemManifest ManifestCopy = SourceItem->GetItemManifest();
	UINV_InventoryItem* ClonedItem = FINV_ItemManifestRuntimeOps::CreateItemFromManifest(ManifestCopy, NewOuter);
	if (!IsValid(ClonedItem))
	{
		return nullptr;
	}

	ClonedItem->SetItemRarityOptions(SourceItem->IsItemRarityEnabled(), SourceItem->GetItemRarityTag());
	ClonedItem->SetTotalStackCount(StackCount);
	return ClonedItem;
}

bool UINV_InventoryComponent::CanTransferItem(UINV_InventoryItem* Item) const
{
	if (!IsValid(Item))
	{
		return false;
	}

	const FINV_EquipmentFragment* EquipmentFragment = Item->GetItemManifest().GetFragmentOfType<FINV_EquipmentFragment>();
	return !EquipmentFragment || !EquipmentFragment->bEquipped;
}

void UINV_InventoryComponent::BroadcastPlayerItemChanged(UINV_InventoryItem* Item)
{
	if (ShouldBroadcastLocalInventoryEvents() && IsValid(Item))
	{
		OnItemChanged.Broadcast(Item);
	}
}

bool UINV_InventoryComponent::ShouldBroadcastLocalInventoryEvents() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) && (OwnerActor->GetNetMode() == NM_ListenServer || OwnerActor->GetNetMode() == NM_Standalone);
}

bool UINV_InventoryComponent::TransferItemBetweenStores(UINV_InventoryItem* Item, const int32 RequestedQuantity, const bool bSourceIsPlayer)
{
	UINV_ContainerComponent* Container = ActiveContainer.Get();
	if (!IsValid(Container) || !IsValid(Item) || !CanTransferItem(Item))
	{
		return false;
	}

	if (bSourceIsPlayer ? !HasItem(Item) : !Container->HasItem(Item))
	{
		return false;
	}

	const EINV_ItemCategory ItemCategory = Item->GetItemManifest().GetItemCategory();
	TArray<UINV_InventoryItem*> SourceItems = bSourceIsPlayer
		? GetItemsForCategory(ItemCategory)
		: Container->GetAllItems();
	TArray<UINV_InventoryItem*> DestinationItems = bSourceIsPlayer
		? Container->GetAllItems()
		: GetItemsForCategory(ItemCategory);

	const FIntPoint SourceGridSize = bSourceIsPlayer ? GetPlayerGridSize(ItemCategory) : Container->GetGridSize();
	const FIntPoint DestinationGridSize = bSourceIsPlayer ? Container->GetGridSize() : GetPlayerGridSize(ItemCategory);
	const int32 SourceStackCount = Item->IsStackable() ? FMath::Max(Item->GetTotalStackCount(), 1) : 1;
	const int32 QuantityToTransfer = Item->IsStackable()
		? FMath::Clamp(RequestedQuantity, 1, SourceStackCount)
		: 1;
	const bool bFullTransfer = !Item->IsStackable() || QuantityToTransfer >= SourceStackCount;

	UINV_InventoryItem* MergeTarget = FINV_StorageTransferUtils::FindFirstMergeTarget(DestinationItems, Item);
	const FINV_StackableFragment* StackableFragment = Item->GetCachedStackableFragment();
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : SourceStackCount;
	const int32 PendingMergeAmount = IsValid(MergeTarget)
		? FMath::Min(QuantityToTransfer, FMath::Max(0, MaxStackSize - MergeTarget->GetTotalStackCount()))
		: 0;
	int32 RemainingQuantity = QuantityToTransfer - PendingMergeAmount;

	if (RemainingQuantity > 0)
	{
		UINV_InventoryItem* SwapCandidate = nullptr;
		if (bFullTransfer && RemainingQuantity == SourceStackCount)
		{
			SwapCandidate = FINV_StorageTransferUtils::FindSwapCandidate(
				SourceItems,
				SourceGridSize,
				DestinationItems,
				DestinationGridSize,
				Item);
		}

		if (IsValid(SwapCandidate))
		{
			UObject* SourceOuter = bSourceIsPlayer ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(Container->GetOwner());
			UObject* DestinationOuter = bSourceIsPlayer ? static_cast<UObject*>(Container->GetOwner()) : static_cast<UObject*>(GetOwner());
			UINV_InventoryItem* ItemForDestination = CloneItemForOwner(Item, DestinationOuter, SourceStackCount);
			UINV_InventoryItem* SwapForSource = CloneItemForOwner(SwapCandidate, SourceOuter, SwapCandidate->GetTotalStackCount());
			if (!IsValid(ItemForDestination) || !IsValid(SwapForSource))
			{
				return false;
			}

			if (bSourceIsPlayer)
			{
				if (!Container->RemoveItem(SwapCandidate))
				{
					return false;
				}

				InventoryFastArray.RemoveEntry(Item);
				Container->AddItem(ItemForDestination);
				InventoryFastArray.AddEntry(SwapForSource);

				if (ShouldBroadcastLocalInventoryEvents())
				{
					OnItemRemoved.Broadcast(Item);
					OnItemAdded.Broadcast(SwapForSource);
				}
			}
			else
			{
				if (!Container->RemoveItem(Item))
				{
					return false;
				}

				InventoryFastArray.RemoveEntry(SwapCandidate);
				InventoryFastArray.AddEntry(ItemForDestination);
				Container->AddItem(SwapForSource);

				if (ShouldBroadcastLocalInventoryEvents())
				{
					OnItemRemoved.Broadcast(SwapCandidate);
					OnItemAdded.Broadcast(ItemForDestination);
				}
			}

			return true;
		}

		if (!FINV_StorageTransferUtils::CanInventoryFitItem(DestinationItems, DestinationGridSize, Item))
		{
			return false;
		}

		UObject* DestinationOuter = bSourceIsPlayer ? static_cast<UObject*>(Container->GetOwner()) : static_cast<UObject*>(GetOwner());
		UINV_InventoryItem* NewDestinationItem = CloneItemForOwner(Item, DestinationOuter, RemainingQuantity);
		if (!IsValid(NewDestinationItem))
		{
			return false;
		}

		if (bSourceIsPlayer)
		{
			Container->AddItem(NewDestinationItem);
		}
		else
		{
			InventoryFastArray.AddEntry(NewDestinationItem);
			if (ShouldBroadcastLocalInventoryEvents())
			{
				OnItemAdded.Broadcast(NewDestinationItem);
			}
		}
	}

	if (PendingMergeAmount > 0)
	{
		MergeTarget->SetTotalStackCount(MergeTarget->GetTotalStackCount() + PendingMergeAmount);
		if (bSourceIsPlayer)
		{
			Container->NotifyItemChanged(MergeTarget);
		}
		else
		{
			BroadcastPlayerItemChanged(MergeTarget);
		}
	}

	const int32 NewSourceStackCount = SourceStackCount - QuantityToTransfer;
	if (!Item->IsStackable() || NewSourceStackCount <= 0)
	{
		if (bSourceIsPlayer)
		{
			InventoryFastArray.RemoveEntry(Item);
			if (ShouldBroadcastLocalInventoryEvents())
			{
				OnItemRemoved.Broadcast(Item);
			}
		}
		else
		{
			Container->RemoveItem(Item);
		}
	}
	else
	{
		Item->SetTotalStackCount(NewSourceStackCount);
		if (bSourceIsPlayer)
		{
			BroadcastPlayerItemChanged(Item);
		}
		else
		{
			Container->NotifyItemChanged(Item);
		}
	}

	return true;
}

void UINV_InventoryComponent::OpenInventoryMenuIfClosed()
{
	if (!bInventoryMenuOpen)
	{
		HandleInventoryMenu(ESlateVisibility::Visible, true);
	}
}

void UINV_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	// Only register when replication is ready.
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
	{
		AddReplicatedSubObject(SubObj);

	}
}

void UINV_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, InventoryFastArray);
}

void UINV_InventoryComponent::Server_DropItem_Implementation(UINV_InventoryItem* Item, int32 StackCount)
{
	if (!HasAuthorityOnOwner()) return;
	if (!IsValid(Item)) return;

	const bool bIsStackable = Item->IsStackable();
	const int32 AvailableStackCount = bIsStackable ? FMath::Max(Item->GetTotalStackCount(), 1) : 1;
	const int32 DroppedStackCount = FMath::Clamp(StackCount, 1, AvailableStackCount);
	const int32 NewStackCount = AvailableStackCount - DroppedStackCount;

	if (!bIsStackable || NewStackCount <= 0)
	{
		InventoryFastArray.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}
	
	SpawnDroppedItem(Item, DroppedStackCount);
}

void UINV_InventoryComponent::SpawnDroppedItem(UINV_InventoryItem* Item, int32 StackCount)
{
	if (!HasAuthorityOnOwner()) return;
	if (!IsValid(Item)) return;
	if (!OwningController.IsValid()) return;

	const APawn* OwningPawn = OwningController->GetPawn();
	if (!IsValid(OwningPawn)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	const auto RestoreDroppedItemToInventory = [this, Item, StackCount]()
	{
		// Restore by type/rarity to preserve stacking behavior and avoid item loss on failed spawn.
		if (UINV_InventoryItem* ExistingItem = InventoryFastArray.FindFirstItemByType(
			Item->GetItemManifest().GetItemType(),
			Item->IsItemRarityEnabled(),
			Item->GetItemRarityTag()))
		{
			ExistingItem->SetTotalStackCount(ExistingItem->GetTotalStackCount() + StackCount);
		}
		else
		{
			InventoryFastArray.AddEntry(Item);
		}
	};

	// Compute a validated drop transform (surface + clearance + overlap checks).
	const FINV_DropLocationResult DropResult = UINV_DropLocationCalculator::CalculateDropLocation(
		World,
		OwningPawn,
		GetOwner(),
		DropSpawnAngleMin,
		DropSpawnAngleMax,
		DropSpawnDistanceMin,
		DropSpawnDistanceMax,
		DropValidationRadius,
		DropValidationHalfHeight,
		DropPlayerClearance,
		RelativeSpawnElevation,
		DropGroundTraceStartHeight,
		DropGroundTraceDepth,
		DropSurfaceOffset);

	if (!DropResult.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to calculate valid drop location. Item not spawned."));
		RestoreDroppedItemToInventory();
		return;
	}

	// Apply collision-independent separation so non-colliding pickups still spread out visually.
	const FVector SpawnLocation = ResolveVisualDropSeparation(DropResult.Location);
	const FRotator SpawnRotation = DropResult.Rotation;
	
	FINV_ItemManifest& ItemManifest { Item->GetItemManifestMutable() };
	if (FINV_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	
	UINV_ItemComponent* SpawnedItemComponent = FINV_ItemManifestRuntimeOps::SpawnPickupActorFromManifest(
		ItemManifest,
		this,
		SpawnLocation,
		SpawnRotation,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	// Some larger pickup actors (commonly equippables) can fail this strict mode near the player.
	// Retry with AlwaysSpawn to ensure drop action is reliable.
	if (!SpawnedItemComponent)
	{
		SpawnedItemComponent = FINV_ItemManifestRuntimeOps::SpawnPickupActorFromManifest(
			ItemManifest,
			this,
			SpawnLocation,
			SpawnRotation,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}
	if (SpawnedItemComponent)
	{
		RememberSuccessfulDropLocation(SpawnLocation);
		SpawnedItemComponent->SetItemRarityOptions(Item->IsItemRarityEnabled(), Item->GetItemRarityTag());
	}
	else
	{
		// Failed to spawn pickup - restore item to inventory to prevent item loss
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn dropped item pickup at location %s. Restoring item to inventory."), *SpawnLocation.ToString());
		RestoreDroppedItemToInventory();
	}
}

void UINV_InventoryComponent::Server_EquipSlotClicked_Implementation(UINV_InventoryItem* ItemToEquip,
	UINV_InventoryItem* ItemToUnequip)
{
	const auto ApplyServerEquipState = [this](UINV_InventoryItem* Item, const bool bEquip)
	{
		if (!IsValid(Item)) return;

		FINV_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
		FINV_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_EquipmentFragment>();
		if (!EquipmentFragment) return;

		if (bEquip)
		{
			EquipmentFragment->OnEquip(OwningController.Get());
		}
		else
		{
			EquipmentFragment->OnUnequip(OwningController.Get());
		}
	};

	if (IsValid(ItemToEquip))
	{
		ApplyServerEquipState(ItemToEquip, true);
		OnItemEquipped.Broadcast(ItemToEquip);
	}

	if (IsValid(ItemToUnequip))
	{
		ApplyServerEquipState(ItemToUnequip, false);
		OnItemUnequipped.Broadcast(ItemToUnequip);
	}
}

void UINV_InventoryComponent::Server_ConsumeItem_Implementation(UINV_InventoryItem* Item)
{
	if (!IsValid(Item)) return;
	if (!HasAuthorityOnOwner()) return;

	const int32 NewStackCount = Item->GetTotalStackCount() - 1;
	if (NewStackCount <= 0)
	{
		InventoryFastArray.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	if (FINV_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FINV_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
}

void UINV_InventoryComponent::TryAddItem(UINV_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return;
	if (!GetOwner()) return;

	if (!IsValid(Inventory))
	{
		ConstructInventory();
	}
	if (!IsValid(Inventory)) return;
	
	// Ask the UI for available space and stacking info.
	const FINV_SlotAvailabilityResult SpaceResult { Inventory->HasRoomForItem(ItemComponent) };
	UINV_InventoryItem* FoundItem { InventoryFastArray.FindFirstItemByType(
		ItemComponent->GetItemManifest().GetItemType(),
		ItemComponent->IsItemRarityEnabled(),
		ItemComponent->GetItemRarityTag()) };

	const FINV_AddItemDecision Decision = FINV_InventoryAddResolver::Resolve(
		SpaceResult,
		IsValid(FoundItem));

	switch (Decision.Action)
	{
	case EINV_AddItemAction::NoRoom:
		OnNoRoomInInventory.Broadcast();
		break;
	case EINV_AddItemAction::AddStacks:
		{
			FINV_SlotAvailabilityResult StackResult = SpaceResult;
			StackResult.Item = FoundItem;
			OnStackChange.Broadcast(StackResult);
		}
		Server_AddStacksToItem(ItemComponent, Decision.StackCountToAdd, Decision.Remainder);
		break;
	case EINV_AddItemAction::AddNewItem:
		// Create a new inventory entry.
		Server_AddNewItem(ItemComponent, Decision.StackCountToAdd);
		break;
	case EINV_AddItemAction::None:
	default:
		break;
	}
}

void UINV_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("OwningController cannot be null"));
	if (!OwningController->IsLocalController()) return;
	
	// Build and hide the inventory widget for local players.
	Inventory = CreateWidget<UINV_InventoryBase>(OwningController.Get(), InventoryClass);
	checkf(Inventory, TEXT("Inventory cannot be null"));
	Inventory->AddToViewport();
	Inventory->SetVisibility(ESlateVisibility::Collapsed);
}

void UINV_InventoryComponent::HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen)
{
	if (!IsValid(Inventory)) return;

	// If closing the menu while holding an item, put it back into its source slot.
	if (!bIsOpen)
	{
		Inventory->ReturnActiveHoverItemToSource();
		if (ActiveContainer.IsValid())
		{
			if (!HasAuthorityOnOwner())
			{
				Server_CloseActiveContainer();
			}
			SetActiveContainerLocal(nullptr);
		}
	}
	
	// Update visibility and input mode.
	Inventory->SetVisibility(Visibility);
	bInventoryMenuOpen = bIsOpen;
	if (AINV_PlayerController* InventoryPlayerController = Cast<AINV_PlayerController>(OwningController.Get()); IsValid(InventoryPlayerController))
	{
		InventoryPlayerController->SetHUDWidgetVisible(!bIsOpen);
	}
	if (bIsOpen)
	{
		EnsureProxyMeshSpawned();
		UpdateProxyMeshVisibility(true);
		Inventory->SetIsFocusable(true);
		Inventory->SetKeyboardFocus();
	}
	else
	{
		UpdateProxyMeshVisibility(false);
	}
	
	if (!OwningController.IsValid()) return;
	
	if (bIsOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(Inventory->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		OwningController->SetInputMode(InputMode);
	}
	else
	{
		OwningController->SetInputMode(FInputModeGameOnly());
	}
	OwningController->SetIgnoreMoveInput(bIsOpen);
	OwningController->SetIgnoreLookInput(bIsOpen);

	APawn* ControlledPawn = OwningController->GetPawn();
	if (bIsOpen)
	{
		if (ACharacter* CharacterPawn = Cast<ACharacter>(ControlledPawn))
		{
			CharacterPawn->StopJumping();
		}
		if (IsValid(ControlledPawn))
		{
			ControlledPawn->DisableInput(OwningController.Get());
			InputSuppressedPawn = ControlledPawn;
		}
	}
	else
	{
		APawn* PawnToEnable = InputSuppressedPawn.IsValid() ? InputSuppressedPawn.Get() : ControlledPawn;
		if (IsValid(PawnToEnable))
		{
			PawnToEnable->EnableInput(OwningController.Get());
		}
		InputSuppressedPawn = nullptr;
	}
	
	ApplyPointerInputMode(bIsOpen);
}

void UINV_InventoryComponent::Server_RequestOpenContainer_Implementation(AActor* ContainerActor)
{
	UINV_ContainerComponent* ContainerComponent = ResolveContainerFromActor(ContainerActor);
	AController* RequestingController = Cast<AController>(GetOwner());
	if (!IsValid(ContainerComponent) || !ContainerComponent->CanAccess(RequestingController))
	{
		Client_CloseContainer();
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()); IsValid(PlayerController) && PlayerController->IsLocalController())
	{
		SetActiveContainerLocal(ContainerComponent);
		OpenInventoryMenuIfClosed();
		return;
	}

	Client_OpenContainer(ContainerActor);
}

void UINV_InventoryComponent::Server_CloseActiveContainer_Implementation()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()); IsValid(PlayerController) && PlayerController->IsLocalController())
	{
		SetActiveContainerLocal(nullptr);
		return;
	}

	Client_CloseContainer();
}

void UINV_InventoryComponent::Client_OpenContainer_Implementation(AActor* ContainerActor)
{
	UINV_ContainerComponent* ContainerComponent = ResolveContainerFromActor(ContainerActor);
	if (!IsValid(ContainerComponent))
	{
		SetActiveContainerLocal(nullptr);
		return;
	}

	SetActiveContainerLocal(ContainerComponent);
	OpenInventoryMenuIfClosed();
}

void UINV_InventoryComponent::Client_CloseContainer_Implementation()
{
	SetActiveContainerLocal(nullptr);
}

void UINV_InventoryComponent::Server_TransferItemWithActiveContainer_Implementation(UINV_InventoryItem* Item, const int32 RequestedQuantity)
{
	if (!HasAuthorityOnOwner() || !IsValid(Item) || !ActiveContainer.IsValid())
	{
		return;
	}

	const bool bSourceIsPlayer = HasItem(Item);
	const bool bSourceIsContainer = ActiveContainer->HasItem(Item);
	if (bSourceIsPlayer == bSourceIsContainer)
	{
		return;
	}

	TransferItemBetweenStores(Item, RequestedQuantity, bSourceIsPlayer);
}

void UINV_InventoryComponent::Server_AddNewItem_Implementation(UINV_ItemComponent* ItemComponent, int32 StackCount)
{
	if (!IsValid(ItemComponent)) return;
	if (!HasAuthorityOnOwner()) return;
	
	// Create and replicate a new item.
	UINV_InventoryItem* NewItem { InventoryFastArray.AddEntry(ItemComponent) };
	NewItem->SetTotalStackCount(StackCount);
	
	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}
	
	ItemComponent->PickedUp();
}

void UINV_InventoryComponent::Server_AddStacksToItem_Implementation(UINV_ItemComponent* ItemComponent, int32 StackCount,
	int32 Remainder)
{
	if (!IsValid(ItemComponent)) return;
	if (!HasAuthorityOnOwner()) return;

	// Update existing stack count and handle remainder.
	const FGameplayTag& ItemType { ItemComponent->GetItemManifest().GetItemType() };
	UINV_InventoryItem* Item { InventoryFastArray.FindFirstItemByType(
		ItemType,
		ItemComponent->IsItemRarityEnabled(),
		ItemComponent->GetItemRarityTag()) };
	if (!IsValid(Item)) return;

	const int32 OldStackCount = Item->GetTotalStackCount();
	Item->SetTotalStackCount(OldStackCount + StackCount);

	// Mark the fast array as dirty so replication knows it changed
	InventoryFastArray.MarkArrayDirty();

	if (Remainder == 0) ItemComponent->PickedUp();
	else if (FINV_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FINV_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}
