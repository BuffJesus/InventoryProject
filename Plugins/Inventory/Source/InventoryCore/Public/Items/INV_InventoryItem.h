// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/INV_ItemPresentationSnapshot.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_InventoryItem.generated.h"

struct FGameplayTag;
struct FINV_ItemManifest;
struct FINV_GridFragment;
struct FINV_ImageFragment;
struct FINV_StackableFragment;
struct FINV_ConsumableFragment;
struct FINV_EquipmentFragment;
class UINV_InventoryItem;

DECLARE_MULTICAST_DELEGATE_OneParam(FINV_InventoryItemChangedNative, UINV_InventoryItem*);

UCLASS()
class INVENTORYCORE_API UINV_InventoryItem : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	
	// Store the item manifest inside this object.
	void SetItemManifest(const FINV_ItemManifest& Manifest);
	// Access the stored manifest.
	const FINV_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FINV_ItemManifest>(); }
	// Mutable manifest access for stack updates, consume effects, and drop flow.
	FINV_ItemManifest& GetItemManifestMutable();
	// True if this item has a stackable fragment.
	bool IsStackable() const;
	// True if this item has a consumable fragment.
	bool IsConsumable() const;
	// Total stack count across all slots.
	FORCEINLINE int32 GetTotalStackCount() const { return TotalStackCount; }
	void SetTotalStackCount(int32 Count);
	// Whether this item exposes rarity styling data.
	FORCEINLINE bool IsItemRarityEnabled() const { return bUseItemRarity; }
	// Item rarity tag used by UI.
	FORCEINLINE const FGameplayTag& GetItemRarityTag() const { return ItemRarityTag; }
	// Cached gameplay item type for hot-path comparisons.
	const FGameplayTag& GetCachedItemType() const;
	// Sets item rarity style data.
	void SetItemRarityOptions(bool bEnabled, const FGameplayTag& InItemRarityTag);
	// True when item matches the provided type/rarity criteria.
	static bool MatchesTypeAndRarity(
		const UINV_InventoryItem* Item,
		const FGameplayTag& ItemType,
		bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag);
	// True when both items can be stacked together.
	static bool AreItemsStackCompatible(
		const UINV_InventoryItem* SourceItem,
		const UINV_InventoryItem* TargetItem);

	// Cached fragment accessors for performance (commonly accessed fragments)
	const FINV_GridFragment* GetCachedGridFragment() const;
	const FINV_ImageFragment* GetCachedImageFragment() const;
	const FINV_StackableFragment* GetCachedStackableFragment() const;
	const FINV_ConsumableFragment* GetCachedConsumableFragment() const;
	const FINV_EquipmentFragment* GetCachedEquipmentFragment() const;
	const FINV_ItemPresentationSnapshot& GetPresentationSnapshot() const;
	// Call this after setting the manifest to build the cache
	void BuildFragmentCache();
	void RebuildPresentationSnapshot();
	FINV_InventoryItemChangedNative& OnItemChanged() { return ItemChangedEvent; }
	
private:
	UFUNCTION() void OnRep_ItemManifest();
	UFUNCTION() void OnRep_TotalStackCount();
	UFUNCTION() void OnRep_ItemRarityOptions();
	void ResetFragmentCache();
	void BroadcastItemChanged();

	// Instanced manifest (fragments + tags).
	UPROPERTY(VisibleAnywhere, Category = "INV|Inventory", meta = (BaseStruct = "/Script/InventoryCore.INV_ItemManifest"), ReplicatedUsing = OnRep_ItemManifest)
	FInstancedStruct ItemManifest;
	
	// Total stacks stored for this item.
	UPROPERTY(ReplicatedUsing = OnRep_TotalStackCount) int32 TotalStackCount { 0 };

	// Enables per-item rarity styling support.
	UPROPERTY(ReplicatedUsing = OnRep_ItemRarityOptions) bool bUseItemRarity { false };

	// Rarity tag used for UI styling.
	UPROPERTY(ReplicatedUsing = OnRep_ItemRarityOptions) FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };

	// Fragment cache for frequently accessed fragments (not replicated, rebuilt on clients)
	mutable const FINV_GridFragment* CachedGridFragment { nullptr };
	mutable const FINV_ImageFragment* CachedImageFragment { nullptr };
	mutable const FINV_StackableFragment* CachedStackableFragment { nullptr };
	mutable const FINV_ConsumableFragment* CachedConsumableFragment { nullptr };
	mutable const FINV_EquipmentFragment* CachedEquipmentFragment { nullptr };
	mutable FGameplayTag CachedItemType { FGameplayTag::EmptyTag };
	mutable FINV_ItemPresentationSnapshot CachedPresentationSnapshot;
	mutable bool bPresentationSnapshotDirty { true };
	FINV_InventoryItemChangedNative ItemChangedEvent;
};

template <typename FragmentType>
const FragmentType* GetFragment(const UINV_InventoryItem* Item, const FGameplayTag& Tag)
{
	// Helper to fetch a fragment by tag from an item.
	if (!IsValid(Item)) return nullptr;
	
	const FINV_ItemManifest& Manifest { Item->GetItemManifest() };
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}
