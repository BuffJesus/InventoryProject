// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Items/INV_InventoryItem.h"

TArray<UINV_InventoryItem*> FINV_InventoryFastArray::GetAllItems() const
{
	// Return a list of valid items.
	TArray<UINV_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (Entry.Item == nullptr) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FINV_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// Notify about items that are being removed.
	for (int32 Index : RemovedIndices)
	{
		if (!Entries.IsValidIndex(Index)) continue;
		if (UINV_InventoryItem* RemovedItem = Entries[Index].Item)
		{
			if (Callbacks.OnItemRemoved)
			{
				Callbacks.OnItemRemoved(RemovedItem);
			}
			if (Callbacks.UnregisterReplicatedSubObject && Callbacks.CanUseReplicationSubObjectList && Callbacks.CanUseReplicationSubObjectList())
			{
				Callbacks.UnregisterReplicatedSubObject(static_cast<UObject*>(RemovedItem));
			}
		}
	}
}

void FINV_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// Notify about items that were added.
	for (int32 Index : AddedIndices)
	{
		if (Callbacks.OnItemAdded)
		{
			Callbacks.OnItemAdded(Entries[Index].Item);
		}
	}
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_ItemComponent* ItemComponent)
{
	// Create a new item object from the pickup manifest.
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor { Owner->GetOwner() };
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	checkf(ItemComponent != nullptr, TEXT("ItemComponent must be valid when adding an item to the inventory fast array"));
	checkf(Callbacks.CreateItemFromPickup, TEXT("CreateItemFromPickup callback must be configured"));

	FINV_InventoryEntry& NewEntry { Entries.AddDefaulted_GetRef() };
	NewEntry.Item = Callbacks.CreateItemFromPickup(ItemComponent, OwningActor);
	checkf(NewEntry.Item != nullptr, TEXT("CreateItemFromPickup callback returned null"));

	if (Callbacks.RegisterReplicatedSubObject && Callbacks.CanUseReplicationSubObjectList && Callbacks.CanUseReplicationSubObjectList())
	{
		Callbacks.RegisterReplicatedSubObject(static_cast<UObject*>(NewEntry.Item.Get()));
	}
	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_InventoryItem* Item)
{
	// Insert an existing item object.
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor { Owner->GetOwner() };
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	
	FINV_InventoryEntry& NewEntry { Entries.AddDefaulted_GetRef() };
	NewEntry.Item = Item;

	if (Callbacks.RegisterReplicatedSubObject && Callbacks.CanUseReplicationSubObjectList && Callbacks.CanUseReplicationSubObjectList())
	{
		Callbacks.RegisterReplicatedSubObject(static_cast<UObject*>(NewEntry.Item.Get()));
	}
	
	MarkItemDirty(NewEntry);
	return Item;
}

void FINV_InventoryFastArray::RemoveEntry(UINV_InventoryItem* Item)
{
	// Remove an entry and mark the array dirty.
	for (auto EntryIt { Entries.CreateIterator() }; EntryIt; ++EntryIt)
	{
		FINV_InventoryEntry& Entry { *EntryIt };
		if (Entry.Item == Item)
		{
			if (Callbacks.UnregisterReplicatedSubObject && Callbacks.CanUseReplicationSubObjectList && Callbacks.CanUseReplicationSubObjectList())
			{
				Callbacks.UnregisterReplicatedSubObject(static_cast<UObject*>(Entry.Item.Get()));
			}
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
			return;
		}
	}
}

const UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag) const
{
	if (!Callbacks.MatchesItemByTypeAndRarity) return nullptr;

	const auto* FoundItem { Entries.FindByPredicate([this, ItemType = ItemType, bUseItemRarity, ItemRarityTag](const FINV_InventoryEntry& Entry)
	{
		if (Entry.Item == nullptr) return false;
		return Callbacks.MatchesItemByTypeAndRarity(Entry.Item, ItemType, bUseItemRarity, ItemRarityTag);
	}) };
	return FoundItem ? FoundItem->Item : nullptr;
}

UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	// Delegate to const version and cast away const for mutable access
	return const_cast<UINV_InventoryItem*>(
		const_cast<const FINV_InventoryFastArray*>(this)->FindFirstItemByType(ItemType, bUseItemRarity, ItemRarityTag)
	);
}
