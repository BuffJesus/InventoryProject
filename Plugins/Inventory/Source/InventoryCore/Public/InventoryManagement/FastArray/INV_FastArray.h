// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryManagement/Placement/INV_InventoryPlacementTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Templates/Function.h"
#include "INV_FastArray.generated.h"

struct FGameplayTag;
class AActor;
class UINV_ItemComponent;
class UINV_InventoryItem;

struct FINV_FastArrayCallbacks
{
	// Runtime item construction and matching hooks.
	TFunction<UINV_InventoryItem*(UINV_ItemComponent*, AActor*)> CreateItemFromPickup;
	TFunction<bool(const UINV_InventoryItem*, const FGameplayTag&, bool, const FGameplayTag&)> MatchesItemByTypeAndRarity;

	// Item lifecycle notifications.
	TFunction<void(UINV_InventoryItem*)> OnItemAdded;
	TFunction<void(UINV_InventoryItem*)> OnItemRemoved;
	TFunction<void(UINV_InventoryItem*)> OnItemChanged;

	// Replication subobject management hooks.
	TFunction<void(UObject*)> RegisterReplicatedSubObject;
	TFunction<void(UObject*)> UnregisterReplicatedSubObject;
	TFunction<bool()> CanUseReplicationSubObjectList;
};

// A single replicated inventory entry.
USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FINV_InventoryEntry(){}
	
private:
	friend struct FINV_InventoryFastArray;
	// The actual item object for this entry.
	UPROPERTY() TObjectPtr<UINV_InventoryItem> Item { nullptr };
	// Authoritative runtime placement for this item within the owning store.
	UPROPERTY() FINV_InventoryItemPlacement Placement;
};

// Replicated list of inventory items.
USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()
	FINV_InventoryFastArray() : Owner(nullptr) {}
	FINV_InventoryFastArray(UActorComponent* InOwner) : Owner(InOwner) {}

	void SetCallbacks(FINV_FastArrayCallbacks&& InCallbacks) { Callbacks = MoveTemp(InCallbacks); }
	void ClearCallbacks() { Callbacks = FINV_FastArrayCallbacks{}; }
	
	// Returns all valid inventory item objects currently held in entries.
	TArray<UINV_InventoryItem*> GetAllItems() const;
	bool ContainsItem(const UINV_InventoryItem* Item) const;
	bool GetItemPlacement(const UINV_InventoryItem* Item, FINV_InventoryItemPlacement& OutPlacement) const;
	bool SetItemPlacement(UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement);
	bool ClearItemPlacement(UINV_InventoryItem* Item);
	bool CanPlaceItemAt(
		const UINV_InventoryItem* Item,
		const FINV_InventoryItemPlacement& Placement,
		const FIntPoint& GridSize,
		TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
		const UINV_InventoryItem* IgnoredItem = nullptr) const;
	bool TryFindFirstAvailablePlacement(
		const UINV_InventoryItem* Item,
		const FIntPoint& GridSize,
		FINV_InventoryItemPlacement& OutPlacement,
		TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
		const UINV_InventoryItem* IgnoredItem = nullptr) const;
	bool TryAutoPlaceItem(
		UINV_InventoryItem* Item,
		const FIntPoint& GridSize,
		TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
		const UINV_InventoryItem* IgnoredItem = nullptr);
	// Const version of FindFirstItemByType for read-only access
	const UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType, bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag) const;
	
	// FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	// End of contract
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FINV_InventoryEntry, FINV_InventoryFastArray>(Entries, DeltaParams, *this);
	}
	
	// Creates a runtime inventory item from a pickup component manifest and adds it.
	UINV_InventoryItem* AddEntry(UINV_ItemComponent* ItemComponent);
	// Adds an already-created runtime inventory item.
	UINV_InventoryItem* AddEntry(UINV_InventoryItem* Item);
	// Removes the matching item entry from the replicated list.
	void RemoveEntry(UINV_InventoryItem* Item);
	// Finds first valid item entry whose item type/rarity matches.
	UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType, bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag);

private:
	FINV_InventoryEntry* FindEntry(UINV_InventoryItem* Item);
	const FINV_InventoryEntry* FindEntry(const UINV_InventoryItem* Item) const;
	// FastArray entries replicated to clients.
	UPROPERTY() TArray<FINV_InventoryEntry> Entries;
	// Owning component used for callbacks.
	UPROPERTY(NotReplicated) TObjectPtr<UActorComponent> Owner { nullptr };
	// Externalized hooks to avoid coupling with specific owner component types.
	FINV_FastArrayCallbacks Callbacks;
};

template<>
struct TStructOpsTypeTraits<FINV_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FINV_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};

