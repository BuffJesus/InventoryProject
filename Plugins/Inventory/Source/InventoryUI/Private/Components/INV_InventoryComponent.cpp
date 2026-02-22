// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/INV_InventoryComponent.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifestRuntimeOps.h"
#include "InventoryManagement/Rules/INV_InventoryAddResolver.h"
#include "InventoryManagement/Utils/INV_DropLocationCalculator.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
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
	
	ConstructInventory();
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
		if (!IsValid(Item)) return false;
		if (!Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType)) return false;
		if (Item->IsItemRarityEnabled() != bUseItemRarity) return false;
		if (!bUseItemRarity) return true;
		return Item->GetItemRarityTag().MatchesTagExact(ItemRarityTag);
	};
	Callbacks.OnItemAdded = [this](UINV_InventoryItem* Item)
	{
		OnItemAdded.Broadcast(Item);
	};
	Callbacks.OnItemRemoved = [this](UINV_InventoryItem* Item)
	{
		OnItemRemoved.Broadcast(Item);
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
	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UINV_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UINV_InventoryItem* ItemToEquip,
	UINV_InventoryItem* ItemToUnequip)
{
	// Equipment component will listen to these delegates
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
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
	if (!IsValid(Inventory)) return;
	if (!GetOwner()) return;
	
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
	}
	
	// Update visibility and input mode.
	Inventory->SetVisibility(Visibility);
	bInventoryMenuOpen = bIsOpen;
	if (bIsOpen)
	{
		Inventory->SetIsFocusable(true);
		Inventory->SetKeyboardFocus();
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
	
	OwningController->SetShowMouseCursor(bIsOpen);
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
	const FGameplayTag& ItemType { IsValid(ItemComponent) ? ItemComponent->GetItemManifest().GetItemType() : FGameplayTag::EmptyTag };
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
