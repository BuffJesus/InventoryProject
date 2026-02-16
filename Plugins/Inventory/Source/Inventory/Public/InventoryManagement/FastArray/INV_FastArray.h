// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "INV_FastArray.generated.h"

struct FGameplayTag;
class UINV_ItemComponent;
class UINV_InventoryComponent;
class UINV_InventoryItem;

// A single replicated inventory entry.
USTRUCT(BlueprintType)
struct FINV_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FINV_InventoryEntry(){}
	
private:
	friend struct FINV_InventoryFastArray;
	friend UINV_InventoryComponent;
	// The actual item object for this entry.
	UPROPERTY() TObjectPtr<UINV_InventoryItem> Item { nullptr };
};

// Replicated list of inventory items.
USTRUCT(BlueprintType)
struct FINV_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()
	FINV_InventoryFastArray() : Owner(nullptr) {}
	FINV_InventoryFastArray(UActorComponent* InOwner) : Owner(InOwner) {}
	
	// Returns all valid inventory item objects currently held in entries.
	TArray<UINV_InventoryItem*> GetAllItems() const;
	
	// FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
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
	// Finds first valid item entry whose item type matches ItemType exactly.
	UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType);

private:
	friend UINV_InventoryComponent;
	// FastArray entries replicated to clients.
	UPROPERTY() TArray<FINV_InventoryEntry> Entries;
	// Owning component used for callbacks.
	UPROPERTY(NotReplicated) TObjectPtr<UActorComponent> Owner { nullptr };
};

template<>
struct TStructOpsTypeTraits<FINV_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FINV_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};

