// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Items/INV_ItemComponent.h"

UINV_InventoryComponent* UINV_InventoryStatics::GetInventoryComponent(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return nullptr;
	UINV_InventoryComponent* InventoryComponent { PlayerController->FindComponentByClass<UINV_InventoryComponent>() };
	return InventoryComponent;
}

EINV_ItemCategory UINV_InventoryStatics::GetItemCategoryFromItemComp(UINV_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return EINV_ItemCategory::None;
	return ItemComponent->GetItemManifest().GetItemCategory();
}
