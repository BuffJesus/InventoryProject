// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Utils/INV_InventoryStatics.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"

UINV_InventoryComponent* UINV_InventoryStatics::GetInventoryComponent(const APlayerController* PlayerController)
{
	// Find the inventory component on the controller.
	if (!IsValid(PlayerController)) return nullptr;
	UINV_InventoryComponent* InventoryComponent { PlayerController->FindComponentByClass<UINV_InventoryComponent>() };
	return InventoryComponent;
}

EINV_ItemCategory UINV_InventoryStatics::GetItemCategoryFromItemComp(UINV_ItemComponent* ItemComponent)
{
	// Return the category from the manifest.
	if (!IsValid(ItemComponent)) return EINV_ItemCategory::None;
	return ItemComponent->GetItemManifest().GetItemCategory();
}

bool UINV_InventoryStatics::TryGetInventoryBase(APlayerController* PC, UINV_InventoryBase*& OutInventoryBase)
{
	UINV_InventoryComponent* IC { GetInventoryComponent(PC) };
	if (!IsValid(IC)) return false;

	OutInventoryBase = IC->GetInventoryMenu();
	return IsValid(OutInventoryBase);
}

void UINV_InventoryStatics::ItemHovered(APlayerController* PC, UINV_InventoryItem* Item)
{
	UINV_InventoryBase* InventoryBase;
	if (!TryGetInventoryBase(PC, InventoryBase)) return;

	if (InventoryBase->HasHoverItem()) return;
	
	InventoryBase->OnItemHovered(Item);
}

void UINV_InventoryStatics::ItemUnhovered(APlayerController* PC)
{
	UINV_InventoryBase* InventoryBase;
	if (!TryGetInventoryBase(PC, InventoryBase)) return;
	
	InventoryBase->OnItemUnhovered();
}

void UINV_InventoryStatics::ItemInspected(APlayerController* PC, UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	UINV_InventoryBase* InventoryBase;
	if (!TryGetInventoryBase(PC, InventoryBase)) return;
	if (!IsValid(Item)) return;

	InventoryBase->OnItemInspected(Item, OpenPosition);
}
