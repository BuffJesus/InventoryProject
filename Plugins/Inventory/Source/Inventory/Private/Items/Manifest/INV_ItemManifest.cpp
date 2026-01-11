// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Manifest/INV_ItemManifest.h"
#include "Items/INV_InventoryItem.h"

UINV_InventoryItem* FINV_ItemManifest::CreateItem(UObject* NewOuter)
{
	UINV_InventoryItem* Item { NewObject<UINV_InventoryItem>(NewOuter, UINV_InventoryItem::StaticClass()) };
	Item->SetItemManifest(*this);
	
	return Item;
}
