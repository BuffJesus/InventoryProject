// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/INV_ItemDefinition.h"
#include "Items/INV_ItemInstanceState.h"
#include "Items/INV_ItemPlacementState.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "INV_FastArray.generated.h"

struct FGameplayTag;
class AActor;
class UINV_ItemComponent;
class UINV_InventoryItem;
enum class EINV_ItemCategory : uint8;

struct FINV_FastArrayCallbacks
{
	// Runtime item construction and matching hooks.
	TFunction<UINV_InventoryItem*(UINV_ItemComponent*, AActor*)> CreateItemFromPickup;
	TFunction<bool(const UINV_InventoryItem*, const FGameplayTag&, bool, const FGameplayTag&)> MatchesItemByTypeAndRarity;
	TFunction<UINV_InventoryItem*(UObject*, const FGuid&, const FINV_ItemDefinitionHandle&, const FINV_ItemInstanceState&, const FINV_ItemPlacementState&, bool, const FGameplayTag&)> CreateItemWrapperFromReplication;

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
	UPROPERTY() FGuid ItemInstanceId;
	UPROPERTY() FINV_ItemDefinitionHandle DefinitionHandle;
	UPROPERTY() FINV_ItemInstanceState InstanceState;
	UPROPERTY() FINV_ItemPlacementState PlacementState;
	UPROPERTY() bool bUseItemRarity { false };
	UPROPERTY() FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };
	UPROPERTY(Transient) TObjectPtr<UINV_InventoryItem> Item { nullptr };
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
	UINV_InventoryItem* AddEntry(UINV_ItemComponent* ItemComponent, const FINV_ItemPlacementState& PlacementState);
	// Adds an already-created runtime inventory item.
	UINV_InventoryItem* AddEntry(UINV_InventoryItem* Item, const FINV_ItemPlacementState& PlacementState);
	// Removes the matching item entry from the replicated list.
	void RemoveEntry(UINV_InventoryItem* Item);
	// Finds first valid item entry whose item type/rarity matches.
	UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType, bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag);
	void SyncItem(UINV_InventoryItem* Item);

private:
	FINV_InventoryEntry* FindEntry(UINV_InventoryItem* Item);
	const FINV_InventoryEntry* FindEntry(UINV_InventoryItem* Item) const;
	UINV_InventoryItem* EnsureItemWrapper(FINV_InventoryEntry& Entry);
	void NotifyItemAdded(UINV_InventoryItem* Item) const;
	void NotifyItemRemoved(UINV_InventoryItem* Item) const;
	void NotifyItemChanged(UINV_InventoryItem* Item) const;

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

