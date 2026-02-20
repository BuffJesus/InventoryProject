// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_InventoryItem.h"

#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Net/UnrealNetwork.h"

void UINV_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate manifest data and stack count.
	DOREPLIFETIME(ThisClass, ItemManifest);
	DOREPLIFETIME(ThisClass, TotalStackCount);
	DOREPLIFETIME(ThisClass, bUseItemRarity);
	DOREPLIFETIME(ThisClass, ItemRarityTag);
}

void UINV_InventoryItem::SetItemManifest(const FINV_ItemManifest& Manifest)
{
	ItemManifest = FInstancedStruct::Make<FINV_ItemManifest>(Manifest);
	BuildFragmentCache();
}

void UINV_InventoryItem::SetItemRarityOptions(bool bEnabled, const FGameplayTag& InItemRarityTag)
{
	bUseItemRarity = bEnabled;
	ItemRarityTag = bUseItemRarity ? InItemRarityTag : FGameplayTag::EmptyTag;
}

bool UINV_InventoryItem::IsStackable() const
{
	return GetCachedStackableFragment() != nullptr;
}

bool UINV_InventoryItem::IsConsumable() const
{
	return GetCachedConsumableFragment() != nullptr;
}

void UINV_InventoryItem::BuildFragmentCache()
{
	// Cache frequently accessed fragments to avoid repeated linear searches
	const FINV_ItemManifest& Manifest = GetItemManifest();
	CachedGridFragment = Manifest.GetFragmentOfTypeWithTag<FINV_GridFragment>(FragmentTags::GridFragment);
	CachedImageFragment = Manifest.GetFragmentOfTypeWithTag<FINV_ImageFragment>(FragmentTags::IconFragment);
	CachedStackableFragment = Manifest.GetFragmentOfTypeWithTag<FINV_StackableFragment>(FragmentTags::StackableFragment);
	CachedConsumableFragment = Manifest.GetFragmentOfTypeWithTag<FINV_ConsumableFragment>(FragmentTags::ConsumableFragment);
}

const FINV_GridFragment* UINV_InventoryItem::GetCachedGridFragment() const
{
	if (!CachedGridFragment)
	{
		// Lazy cache build if not already cached
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedGridFragment;
}

const FINV_ImageFragment* UINV_InventoryItem::GetCachedImageFragment() const
{
	if (!CachedImageFragment)
	{
		// Lazy cache build if not already cached
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedImageFragment;
}

const FINV_StackableFragment* UINV_InventoryItem::GetCachedStackableFragment() const
{
	if (!CachedStackableFragment)
	{
		// Lazy cache build if not already cached
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedStackableFragment;
}

const FINV_ConsumableFragment* UINV_InventoryItem::GetCachedConsumableFragment() const
{
	if (!CachedConsumableFragment)
	{
		// Lazy cache build if not already cached
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedConsumableFragment;
}
