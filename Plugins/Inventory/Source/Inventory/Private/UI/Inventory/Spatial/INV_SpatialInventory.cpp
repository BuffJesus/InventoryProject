// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_SpatialInventory.h"

#include "Inventory.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "Items/INV_ItemComponent.h"
#include "UI/Inventory/Spatial/INV_InventoryGrid.h"

void UINV_SpatialInventory::ShowEquippableGrid()
{
	SetActiveGrid(Grid_Equippable, Button_Equippable);
}

void UINV_SpatialInventory::ShowConsumableGrid()
{
	SetActiveGrid(Grid_Consumable, Button_Consumable);
}

void UINV_SpatialInventory::ShowCraftableGrid()
{
	SetActiveGrid(Grid_Craftable, Button_Craftable);
}

void UINV_SpatialInventory::DisableButton(UButton* Button)
{
	Button_Equippable->SetIsEnabled(true);
	Button_Consumable->SetIsEnabled(true);
	Button_Craftable->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UINV_SpatialInventory::SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button)
{
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}

void UINV_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::ShowEquippableGrid);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::ShowConsumableGrid);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::ShowCraftableGrid);
	
	ShowEquippableGrid();
}

FINV_SlotAvailabilityResult UINV_SpatialInventory::HasRoomForItem(UINV_ItemComponent* ItemComponent) const
{
	switch (UINV_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{
		case EINV_ItemCategory::Equippable: return Grid_Equippable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Consumable: return Grid_Consumable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Craftable: return Grid_Craftable->HasRoomForItem(ItemComponent);
		default:
		UE_LOG(LogInventory, Error, TEXT("Invalid item category for inventory slot availability check"));
		return FINV_SlotAvailabilityResult();
	}
}
