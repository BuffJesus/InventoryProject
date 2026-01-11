// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "INV_FastArray.generated.h"

class UINV_ItemComponent;
class UINV_InventoryComponent;
class UINV_InventoryItem;

// A single entry in the inventory
USTRUCT(BlueprintType)
struct FINV_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	FINV_InventoryEntry(){}
	
private:
	friend struct FINV_InventoryFastArray;
	friend UINV_InventoryComponent;
	UPROPERTY() TObjectPtr<UINV_InventoryItem> Item { nullptr };
};

// List of inventory items
USTRUCT(BlueprintType)
struct FINV_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()
	FINV_InventoryFastArray() : Owner(nullptr) {}
	FINV_InventoryFastArray(UActorComponent* InOwner) : Owner(InOwner) {}
	
	TArray<UINV_InventoryItem*> GetAllItems() const;
	
	// FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	// End of contract
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FINV_InventoryEntry, FINV_InventoryFastArray>(Entries, DeltaParams, *this);
	}
	
	UINV_InventoryItem* AddEntry(UINV_ItemComponent* ItemComponent);
	UINV_InventoryItem* AddEntry(UINV_InventoryItem* Item);
	void RemoveEntry(UINV_InventoryItem* Item);
	
private:
	friend UINV_InventoryComponent;
	UPROPERTY() TArray<FINV_InventoryEntry> Entries;
	UPROPERTY(NotReplicated) TObjectPtr<UActorComponent> Owner { nullptr };
};

template<>
struct TStructOpsTypeTraits<FINV_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FINV_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};

