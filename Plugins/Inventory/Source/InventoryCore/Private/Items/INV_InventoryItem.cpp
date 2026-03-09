// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_InventoryItem.h"

#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Internationalization/Text.h"
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
	RebuildPresentationSnapshot();
	BroadcastItemChanged();
}

FINV_ItemManifest& UINV_InventoryItem::GetItemManifestMutable()
{
	ResetFragmentCache();
	return ItemManifest.GetMutable<FINV_ItemManifest>();
}

void UINV_InventoryItem::SetTotalStackCount(const int32 Count)
{
	if (TotalStackCount == Count)
	{
		return;
	}

	TotalStackCount = Count;
	bPresentationSnapshotDirty = true;
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
	bPresentationSnapshotDirty = true;
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
	CachedEquipmentFragment = Manifest.GetFragmentOfTypeWithTag<FINV_EquipmentFragment>(FragmentTags::EquipmentFragment);
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

const FINV_EquipmentFragment* UINV_InventoryItem::GetCachedEquipmentFragment() const
{
	if (!CachedEquipmentFragment)
	{
		const_cast<UINV_InventoryItem*>(this)->BuildFragmentCache();
	}
	return CachedEquipmentFragment;
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

const FINV_ItemPresentationSnapshot& UINV_InventoryItem::GetPresentationSnapshot() const
{
	if (bPresentationSnapshotDirty)
	{
		const_cast<UINV_InventoryItem*>(this)->RebuildPresentationSnapshot();
	}
	return CachedPresentationSnapshot;
}

void UINV_InventoryItem::RebuildPresentationSnapshot()
{
	CachedPresentationSnapshot.Reset();

	const FINV_ItemManifest& Manifest = GetItemManifest();
	CachedPresentationSnapshot.ItemType = GetCachedItemType();
	CachedPresentationSnapshot.ItemRarityTag = bUseItemRarity ? ItemRarityTag : FGameplayTag::EmptyTag;
	CachedPresentationSnapshot.StackCount = TotalStackCount;
	CachedPresentationSnapshot.bStackable = GetCachedStackableFragment() != nullptr;
	CachedPresentationSnapshot.bConsumable = GetCachedConsumableFragment() != nullptr;
	CachedPresentationSnapshot.bEquippable = GetCachedEquipmentFragment() != nullptr;

	if (const FINV_ImageFragment* ImageFragment = GetCachedImageFragment())
	{
		CachedPresentationSnapshot.Icon = ImageFragment->GetIcon();
		CachedPresentationSnapshot.IconDimensions = ImageFragment->GetIconDimensions();
	}

	if (const FINV_GridFragment* GridFragment = GetCachedGridFragment())
	{
		CachedPresentationSnapshot.GridSize = GridFragment->GetGridSize();
	}

	Manifest.ForEachFragment<FINV_TextFragment>([this](const FINV_TextFragment& Fragment)
	{
		FINV_ItemTextLine& TextLine = CachedPresentationSnapshot.TextLines.AddDefaulted_GetRef();
		TextLine.FragmentTag = Fragment.GetFragmentTag();
		TextLine.Text = Fragment.GetText();

		if (CachedPresentationSnapshot.DisplayName.IsEmpty()
			&& Fragment.GetFragmentTag().MatchesTagExact(FragmentTags::ItemNameFragment))
		{
			CachedPresentationSnapshot.DisplayName = Fragment.GetText();
		}
	});

	Manifest.ForEachFragment<FINV_LabeledNumberFragment>([this](const FINV_LabeledNumberFragment& Fragment)
	{
		FINV_ItemStatLine& StatLine = CachedPresentationSnapshot.StatLines.AddDefaulted_GetRef();
		StatLine.FragmentTag = Fragment.GetFragmentTag();
		StatLine.Label = Fragment.GetLabelText();
		StatLine.bCollapseLabel = Fragment.GetCollapseLabel();
		StatLine.bCollapseValue = Fragment.GetCollapseValue();

		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = Fragment.GetMinFractionalDigits();
		Options.MaximumFractionalDigits = Fragment.GetMaxFractionalDigits();
		StatLine.Value = FText::AsNumber(Fragment.GetValue(), &Options);
	});

	Manifest.ForEachFragment<FINV_ConsumableFragment>([this](const FINV_ConsumableFragment& Fragment)
	{
		for (const TInstancedStruct<FINV_ConsumeModifier>& Modifier : Fragment.GetConsumeModifiers())
		{
			const FINV_ConsumeModifier& ConsumeModifier = Modifier.Get();
			FINV_ItemStatLine& StatLine = CachedPresentationSnapshot.StatLines.AddDefaulted_GetRef();
			StatLine.FragmentTag = ConsumeModifier.GetFragmentTag();
			StatLine.Label = ConsumeModifier.GetLabelText();
			StatLine.bCollapseLabel = ConsumeModifier.GetCollapseLabel();
			StatLine.bCollapseValue = ConsumeModifier.GetCollapseValue();

			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = ConsumeModifier.GetMinFractionalDigits();
			Options.MaximumFractionalDigits = ConsumeModifier.GetMaxFractionalDigits();
			StatLine.Value = FText::AsNumber(ConsumeModifier.GetValue(), &Options);
		}
	});

	Manifest.ForEachFragment<FINV_EquipmentFragment>([this](const FINV_EquipmentFragment& Fragment)
	{
		for (const TInstancedStruct<FINV_EquipModifier>& Modifier : Fragment.GetEquipModifiers())
		{
			const FINV_EquipModifier& EquipModifier = Modifier.Get();
			FINV_ItemStatLine& StatLine = CachedPresentationSnapshot.StatLines.AddDefaulted_GetRef();
			StatLine.FragmentTag = EquipModifier.GetFragmentTag();
			StatLine.Label = EquipModifier.GetLabelText();
			StatLine.bCollapseLabel = EquipModifier.GetCollapseLabel();
			StatLine.bCollapseValue = EquipModifier.GetCollapseValue();

			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = EquipModifier.GetMinFractionalDigits();
			Options.MaximumFractionalDigits = EquipModifier.GetMaxFractionalDigits();
			StatLine.Value = FText::AsNumber(EquipModifier.GetValue(), &Options);
		}
	});

	if (CachedPresentationSnapshot.DisplayName.IsEmpty() && CachedPresentationSnapshot.TextLines.Num() > 0)
	{
		CachedPresentationSnapshot.DisplayName = CachedPresentationSnapshot.TextLines[0].Text;
	}

	bPresentationSnapshotDirty = false;
}

void UINV_InventoryItem::OnRep_ItemManifest()
{
	ResetFragmentCache();
	BuildFragmentCache();
	RebuildPresentationSnapshot();
	BroadcastItemChanged();
}

void UINV_InventoryItem::OnRep_TotalStackCount()
{
	bPresentationSnapshotDirty = true;
	BroadcastItemChanged();
}

void UINV_InventoryItem::OnRep_ItemRarityOptions()
{
	bPresentationSnapshotDirty = true;
	BroadcastItemChanged();
}

void UINV_InventoryItem::ResetFragmentCache()
{
	CachedGridFragment = nullptr;
	CachedImageFragment = nullptr;
	CachedStackableFragment = nullptr;
	CachedConsumableFragment = nullptr;
	CachedEquipmentFragment = nullptr;
	CachedItemType = FGameplayTag::EmptyTag;
	bPresentationSnapshotDirty = true;
}

void UINV_InventoryItem::BroadcastItemChanged()
{
	ItemChangedEvent.Broadcast(this);
}
