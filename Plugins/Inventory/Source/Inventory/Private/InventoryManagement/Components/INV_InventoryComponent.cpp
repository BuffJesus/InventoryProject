// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"

UINV_InventoryComponent::UINV_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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
	bInventoryMenuOpen ? HandleInventoryMenu(ESlateVisibility::Collapsed, false)
		: HandleInventoryMenu(ESlateVisibility::Visible, true);
}

void UINV_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
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

void UINV_InventoryComponent::TryAddItem(UINV_ItemComponent* ItemComponent)
{
	FINV_SlotAvailabilityResult Result { Inventory->HasRoomForItem(ItemComponent) };
	
	if (Result.TotalRoomToFill == 0)
	{
		OnNoRoomInInventory.Broadcast();
		return;
	}
	
	if (Result.Item.IsValid() && Result.bStackable)
	{
		// Add stacks to item that already exists
		Server_AddStacksToItem(ItemComponent, Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		// Item does not yet exist in inventory. Create new entry and update all pertinent stats
		Server_AddNewItem(ItemComponent, Result.bStackable ? Result.TotalRoomToFill : 0);
	}
}

void UINV_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("OwningController cannot be null"));
	if (!OwningController->IsLocalController()) return;
	
	Inventory = CreateWidget<UINV_InventoryBase>(OwningController.Get(), InventoryClass);
	checkf(Inventory, TEXT("Inventory cannot be null"));
	Inventory->AddToViewport();
	// Inventory->SetVisibility(ESlateVisibility::Collapsed);
}

void UINV_InventoryComponent::HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen)
{
	if (!IsValid(Inventory)) return;
	
	Inventory->SetVisibility(Visibility);
	bInventoryMenuOpen = bIsOpen;
	
	if (!OwningController.IsValid()) return;
	
	bIsOpen ? OwningController->SetInputMode(FInputModeGameAndUI()) 
			: OwningController->SetInputMode(FInputModeGameOnly());
	
	OwningController->SetShowMouseCursor(bIsOpen);
}

void UINV_InventoryComponent::Server_AddNewItem_Implementation(UINV_ItemComponent* ItemComponent, int32 StackCount)
{
	UINV_InventoryItem* NewItem { InventoryFastArray.AddEntry(ItemComponent) };
	
	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}
	
	// TODO: Tell item component to destroy owning actor
}

void UINV_InventoryComponent::Server_AddStacksToItem_Implementation(UINV_ItemComponent* ItemComponent, int32 StackCount,
	int32 Remainder)
{
	
}
