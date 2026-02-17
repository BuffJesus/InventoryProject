// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Inventory.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"

namespace
{
bool IsDropLocationClear(const UWorld* World, const FVector& Location, const APawn* OwningPawn, const AActor* OwnerActor,
	const float Radius, const float HalfHeight)
{
	if (!IsValid(World)) return false;
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(INV_DropLocationClear), false);
	if (IsValid(OwningPawn)) QueryParams.AddIgnoredActor(OwningPawn);
	if (IsValid(OwnerActor)) QueryParams.AddIgnoredActor(OwnerActor);
	
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	return !World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, CollisionShape, QueryParams)
		&& !World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_WorldDynamic, CollisionShape, QueryParams);
}

FVector EnforcePawnClearance(const APawn* OwningPawn, const FVector& Location, const FVector& PreferredDirection,
	const float ItemRadius, const float ExtraClearance)
{
	if (!IsValid(OwningPawn)) return Location;

	const UCapsuleComponent* PawnCapsule { OwningPawn->FindComponentByClass<UCapsuleComponent>() };
	if (!IsValid(PawnCapsule)) return Location;

	const float MinHorizontalDistance = PawnCapsule->GetScaledCapsuleRadius() + ItemRadius + ExtraClearance;
	const FVector PawnLocation = OwningPawn->GetActorLocation();
	FVector Delta2D = Location - PawnLocation;
	Delta2D.Z = 0.0f;
	const float CurrentDistance = Delta2D.Size();
	if (CurrentDistance >= MinHorizontalDistance) return Location;

	FVector PushDirection = Delta2D.GetSafeNormal();
	if (PushDirection.IsNearlyZero())
	{
		FVector FallbackDirection = PreferredDirection;
		FallbackDirection.Z = 0.0f;
		PushDirection = FallbackDirection.GetSafeNormal();
		if (PushDirection.IsNearlyZero())
		{
			PushDirection = FVector::ForwardVector;
		}
	}

	const FVector New2DLocation = PawnLocation + PushDirection * MinHorizontalDistance;
	return FVector(New2DLocation.X, New2DLocation.Y, Location.Z);
}
}

UINV_InventoryComponent::UINV_InventoryComponent() : InventoryFastArray(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	// Replicate items as registered subobjects.
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenuOpen = false;
}

void UINV_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructInventory();
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
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
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
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!IsValid(Item)) return;
	if (!OwningController.IsValid()) return;

	const APawn* OwningPawn { OwningController->GetPawn() };
	if (!IsValid(OwningPawn)) return;

	UWorld* World { GetWorld() };
	if (!IsValid(World)) return;

	FVector RotatedForward { OwningPawn->GetActorForwardVector() };
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);
	FVector Forward2D = RotatedForward;
	Forward2D.Z = 0.0f;
	Forward2D = Forward2D.GetSafeNormal();
	if (Forward2D.IsNearlyZero())
	{
		Forward2D = OwningPawn->GetActorForwardVector().GetSafeNormal2D();
		if (Forward2D.IsNearlyZero())
		{
			Forward2D = FVector::ForwardVector;
		}
	}

	float RequiredDropDistance = DropValidationRadius + DropPlayerClearance;
	if (const UCapsuleComponent* PawnCapsule { OwningPawn->FindComponentByClass<UCapsuleComponent>() })
	{
		RequiredDropDistance += PawnCapsule->GetScaledCapsuleRadius();
	}
	const float RequestedDropDistance = FMath::FRandRange(DropSpawnDistanceMin, DropSpawnDistanceMax);
	const float DropDistance = FMath::Max(RequestedDropDistance, RequiredDropDistance);
	const FVector BaseSpawnLocation = OwningPawn->GetActorLocation() + Forward2D * DropDistance;
	FVector SpawnLocation = BaseSpawnLocation;
	SpawnLocation.Z -= RelativeSpawnElevation;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(INV_DropGroundTrace), false);
	QueryParams.AddIgnoredActor(OwningPawn);
	if (IsValid(GetOwner())) QueryParams.AddIgnoredActor(GetOwner());

	// Snap to a valid surface first to avoid embedding into floor/landscape.
	const FVector TraceStart = SpawnLocation + FVector::UpVector * DropGroundTraceStartHeight;
	const FVector TraceEnd = SpawnLocation - FVector::UpVector * DropGroundTraceDepth;
	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams)
		|| World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldDynamic, QueryParams))
	{
		SpawnLocation = GroundHit.ImpactPoint + GroundHit.ImpactNormal * DropSurfaceOffset;
	}

	SpawnLocation = EnforcePawnClearance(OwningPawn, SpawnLocation, Forward2D, DropValidationRadius, DropPlayerClearance);

	// Validate item volume; if blocked try to nudge out once along hit normal.
	const FCollisionShape DropShape = FCollisionShape::MakeCapsule(DropValidationRadius, DropValidationHalfHeight);
	FHitResult SweepHit;
	if (World->SweepSingleByChannel(SweepHit, SpawnLocation, SpawnLocation, FQuat::Identity, ECC_WorldStatic, DropShape, QueryParams)
		|| World->SweepSingleByChannel(SweepHit, SpawnLocation, SpawnLocation, FQuat::Identity, ECC_WorldDynamic, DropShape, QueryParams))
	{
		const FVector PushNormal = SweepHit.Normal.IsNearlyZero() ? FVector::UpVector : SweepHit.Normal;
		const FVector AdjustedLocation = SpawnLocation + PushNormal * (DropValidationRadius + DropSurfaceOffset);
		if (IsDropLocationClear(World, AdjustedLocation, OwningPawn, GetOwner(), DropValidationRadius, DropValidationHalfHeight))
		{
			SpawnLocation = AdjustedLocation;
		}
	}

	SpawnLocation = EnforcePawnClearance(OwningPawn, SpawnLocation, Forward2D, DropValidationRadius, DropPlayerClearance);
	
	FINV_ItemManifest& ItemManifest { Item->GetItemManifestMutable() };
	if (FINV_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	
	UINV_ItemComponent* SpawnedItemComponent = ItemManifest.SpawnPickupActor(this, SpawnLocation, SpawnRotation, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (SpawnedItemComponent)
	{
		SpawnedItemComponent->SetItemRarityOptions(Item->IsItemRarityEnabled(), Item->GetItemRarityTag());
	}
	else
	{
		// Failed to spawn pickup - restore item to inventory to prevent item loss
		UE_LOG(LogInventory, Warning, TEXT("Failed to spawn dropped item pickup at location %s. Restoring item to inventory."), *SpawnLocation.ToString());

		// Re-add the item to inventory if it was removed
		if (UINV_InventoryItem* ExistingItem = InventoryFastArray.FindFirstItemByType(
			Item->GetItemManifest().GetItemType(),
			Item->IsItemRarityEnabled(),
			Item->GetItemRarityTag()))
		{
			// Item still exists, restore stack count
			ExistingItem->SetTotalStackCount(ExistingItem->GetTotalStackCount() + StackCount);
		}
		else
		{
			// Item was removed, re-add it
			InventoryFastArray.AddEntry(Item);
		}
	}
}

void UINV_InventoryComponent::Server_ConsumeItem_Implementation(UINV_InventoryItem* Item)
{
	if (!IsValid(Item)) return;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

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
	FINV_SlotAvailabilityResult Result { Inventory->HasRoomForItem(ItemComponent) };
	
	UINV_InventoryItem* FoundItem { InventoryFastArray.FindFirstItemByType(
		ItemComponent->GetItemManifest().GetItemType(),
		ItemComponent->IsItemRarityEnabled(),
		ItemComponent->GetItemRarityTag()) };
	Result.Item = FoundItem;
	
	if (Result.TotalRoomToFill == 0)
	{
		// No room to add.
		OnNoRoomInInventory.Broadcast();
		return;
	}
	
	if (Result.Item.IsValid() && Result.bStackable)
	{
		// Add stacks to an existing item.
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent, Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		// Create a new inventory entry.
		Server_AddNewItem(ItemComponent, Result.bStackable ? Result.TotalRoomToFill : 0);
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
	
	// Update visibility and input mode.
	Inventory->SetVisibility(Visibility);
	bInventoryMenuOpen = bIsOpen;
	
	if (!OwningController.IsValid()) return;
	
	bIsOpen ? OwningController->SetInputMode(FInputModeGameAndUI()) 
			: OwningController->SetInputMode(FInputModeGameOnly());
	
	OwningController->SetShowMouseCursor(bIsOpen);
}

void UINV_InventoryComponent::Server_AddNewItem_Implementation(UINV_ItemComponent* ItemComponent, int32 StackCount)
{
	if (!IsValid(ItemComponent)) return;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	
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
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

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
