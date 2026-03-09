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
	ResetFragmentCache();
	BuildFragmentCache();
	BroadcastItemChanged();
}

void UINV_InventoryItem::SetTotalStackCount(const int32 Count)
{
	if (TotalStackCount == Count)
	{
		return;
	}

	TotalStackCount = Count;
	BroadcastItemChanged();
}

void UINV_InventoryItem::SetItemRarityOptions(bool bEnabled, const FGameplayTag& InItemRarityTag)
{
	const bool bNewUseItemRarity = bEnabled;
	const FGameplayTag NewItemRarityTag = bNewUseItemRarity ? InItemRarityTag : FGameplayTag::EmptyTag;
	if (bUseItemRarity == bNewUseItemRarity && ItemRarityTag.MatchesTagExact(NewItemRarityTag))
	{
		return;
	}

	bUseItemRarity = bNewUseItemRarity;
	ItemRarityTag = NewItemRarityTag;
	BroadcastItemChanged();
}

bool UINV_InventoryItem::MatchesTypeAndRarity(
	const UINV_InventoryItem* Item,
	const FGameplayTag& ItemType,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	if (!IsValid(Item)) return false;
	if (!Item->GetCachedItemType().MatchesTagExact(ItemType)) return false;
	if (Item->IsItemRarityEnabled() != bUseItemRarity) return false;
	if (!bUseItemRarity) return true;
	return Item->GetItemRarityTag().MatchesTagExact(ItemRarityTag);
}

bool UINV_InventoryItem::AreItemsStackCompatible(
	const UINV_InventoryItem* SourceItem,
	const UINV_InventoryItem* TargetItem)
{
	if (!IsValid(SourceItem) || !IsValid(TargetItem))
	{
		return false;
	}

	if (!SourceItem->IsStackable() || !TargetItem->IsStackable())
	{
		return false;
	}

	return MatchesTypeAndRarity(
		TargetItem,
		SourceItem->GetCachedItemType(),
		SourceItem->IsItemRarityEnabled(),
		SourceItem->GetItemRarityTag());
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
	CachedItemType = Manifest.GetItemType();
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

const FGameplayTag& UINV_InventoryItem::GetCachedItemType() const
{
	if (!CachedItemType.IsValid())
	{
		// Lazy cache build if not already cached
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedItemType;
}

void UINV_InventoryItem::OnRep_ItemManifest()
{
	ResetFragmentCache();
	BuildFragmentCache();
	BroadcastItemChanged();
}

void UINV_InventoryItem::OnRep_TotalStackCount()
{
	BroadcastItemChanged();
}

void UINV_InventoryItem::OnRep_ItemRarityOptions()
{
	BroadcastItemChanged();
}

void UINV_InventoryItem::ResetFragmentCache()
{
	CachedGridFragment = nullptr;
	CachedImageFragment = nullptr;
	CachedStackableFragment = nullptr;
	CachedConsumableFragment = nullptr;
	CachedItemType = FGameplayTag::EmptyTag;
}

void UINV_InventoryItem::BroadcastItemChanged()
{
	ItemChangedEvent.Broadcast(this);
}
