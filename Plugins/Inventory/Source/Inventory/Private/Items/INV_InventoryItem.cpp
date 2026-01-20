// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_InventoryItem.h"

#include "Items/Fragments/INV_ItemFragment.h"
#include "Net/UnrealNetwork.h"

void UINV_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, ItemManifest);
}

bool UINV_InventoryItem::IsStackable() const
{
	const FINV_StackableFragment* StackableFragment { GetItemManifest().GetFragmentOfType<FINV_StackableFragment>() };
	return StackableFragment != nullptr;
}
