// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Items/INV_ItemComponent.h"
#include "Items/INV_InventoryItem.h"

TArray<UINV_InventoryItem*> FINV_InventoryFastArray::GetAllItems() const
{
	// Return a list of valid items.
	TArray<UINV_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FINV_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// Notify about items that are being removed.
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	if (!IsValid(IC)) return;
	for (int32 Index : RemovedIndices)
	{
		if (!Entries.IsValidIndex(Index)) continue;
		if (UINV_InventoryItem* RemovedItem = Entries[Index].Item)
		{
			IC->OnItemRemoved.Broadcast(RemovedItem);
			if (IC->IsUsingRegisteredSubObjectList() && IC->IsReadyForReplication())
			{
				IC->RemoveReplicatedSubObject(RemovedItem);
			}
		}
	}
}

void FINV_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// Notify about items that were added.
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	if (!IsValid(IC)) return;
	for (int32 Index : AddedIndices)
	{
		IC->OnItemAdded.Broadcast(Entries[Index].Item);
	}
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_ItemComponent* ItemComponent)
{
	// Create a new item object from the pickup manifest.
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor { Owner->GetOwner() };
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	checkf(IsValid(IC), TEXT("ItemComponent must be valid when adding an item to the inventory fast array"));

	FINV_InventoryEntry& NewEntry { Entries.AddDefaulted_GetRef() };
	NewEntry.Item = ItemComponent->GetItemManifestMutable().CreateItem(OwningActor);
	NewEntry.Item->SetItemRarityOptions(ItemComponent->IsItemRarityEnabled(), ItemComponent->GetItemRarityTag());

	IC->AddRepSubObj(NewEntry.Item);
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
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

const UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag) const
{
	const auto* FoundItem { Entries.FindByPredicate([ItemType = ItemType, bUseItemRarity, ItemRarityTag](const FINV_InventoryEntry& Entry)
	{
		if (!IsValid(Entry.Item)) return false;
		if (!Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType)) return false;
		if (Entry.Item->IsItemRarityEnabled() != bUseItemRarity) return false;
		if (!bUseItemRarity) return true;
		return Entry.Item->GetItemRarityTag().MatchesTagExact(ItemRarityTag);
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
