// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_InventoryItem.h"

#include "Items/Fragments/INV_ItemFragment.h"
#include "Net/UnrealNetwork.h"

void UINV_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate manifest data and stack count.
	DOREPLIFETIME(ThisClass, ItemManifest);
	DOREPLIFETIME(ThisClass, TotalStackCount);
}

void UINV_InventoryItem::SetItemManifest(const FINV_ItemManifest& Manifest)
{
	ItemManifest = FInstancedStruct::Make<FINV_ItemManifest>(Manifest);
}

bool UINV_InventoryItem::IsStackable() const
{
	// Stackable if a stack fragment exists.
	const FINV_StackableFragment* StackableFragment { GetItemManifest().GetFragmentOfType<FINV_StackableFragment>() };
	return StackableFragment != nullptr;
}

bool UINV_InventoryItem::IsConsumable() const
{
	// Consumable if a consumable fragment exists.
	const FINV_ConsumableFragment* ConsumableFragment { GetItemManifest().GetFragmentOfType<FINV_ConsumableFragment>() };
	return ConsumableFragment != nullptr;
}
