// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/FastArray/INV_FastArray.h"

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Items/INV_InventoryItem.h"
#include "UObject/UObjectGlobals.h"

TArray<UINV_InventoryItem*> FINV_InventoryFastArray::GetAllItems() const
{
	TArray<UINV_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const FINV_InventoryEntry& Entry : Entries)
	{
		if (Entry.Item == nullptr)
		{
			continue;
		}

		Results.Add(Entry.Item);
	}
	return Results;
}

FINV_InventoryEntry* FINV_InventoryFastArray::FindEntry(UINV_InventoryItem* Item)
{
	return Entries.FindByPredicate([Item](const FINV_InventoryEntry& Entry)
	{
		return Entry.Item == Item;
	});
}

const FINV_InventoryEntry* FINV_InventoryFastArray::FindEntry(UINV_InventoryItem* Item) const
{
	return Entries.FindByPredicate([Item](const FINV_InventoryEntry& Entry)
	{
		return Entry.Item == Item;
	});
}

UINV_InventoryItem* FINV_InventoryFastArray::EnsureItemWrapper(FINV_InventoryEntry& Entry)
{
	if (IsValid(Entry.Item))
	{
		Entry.Item->InitializeFromReplicatedData(
			Entry.ItemInstanceId,
			Entry.DefinitionHandle,
			Entry.InstanceState,
			Entry.PlacementState,
			Entry.bUseItemRarity,
			Entry.ItemRarityTag);
		return Entry.Item;
	}

	if (!Callbacks.CreateItemWrapperFromReplication)
	{
		return nullptr;
	}

	UObject* ItemOuter = Owner ? static_cast<UObject*>(Owner) : GetTransientPackage();
	Entry.Item = Callbacks.CreateItemWrapperFromReplication(
		ItemOuter,
		Entry.ItemInstanceId,
		Entry.DefinitionHandle,
		Entry.InstanceState,
		Entry.PlacementState,
		Entry.bUseItemRarity,
		Entry.ItemRarityTag);
	return Entry.Item;
}

void FINV_InventoryFastArray::NotifyItemAdded(UINV_InventoryItem* Item) const
{
	if (Callbacks.OnItemAdded)
	{
		Callbacks.OnItemAdded(Item);
	}
}

void FINV_InventoryFastArray::NotifyItemRemoved(UINV_InventoryItem* Item) const
{
	if (Callbacks.OnItemRemoved)
	{
		Callbacks.OnItemRemoved(Item);
	}
}

void FINV_InventoryFastArray::NotifyItemChanged(UINV_InventoryItem* Item) const
{
	if (Callbacks.OnItemChanged)
	{
		Callbacks.OnItemChanged(Item);
	}
}

void FINV_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		if (!Entries.IsValidIndex(Index))
		{
			continue;
		}

		if (UINV_InventoryItem* RemovedItem = Entries[Index].Item)
		{
			NotifyItemRemoved(RemovedItem);
		}
	}
}

void FINV_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		if (!Entries.IsValidIndex(Index))
		{
			continue;
		}

		if (UINV_InventoryItem* AddedItem = EnsureItemWrapper(Entries[Index]))
		{
			NotifyItemAdded(AddedItem);
		}
	}
}

void FINV_InventoryFastArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		if (!Entries.IsValidIndex(Index))
		{
			continue;
		}

		if (UINV_InventoryItem* ChangedItem = EnsureItemWrapper(Entries[Index]))
		{
			NotifyItemChanged(ChangedItem);
		}
	}
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_ItemComponent* ItemComponent, const FINV_ItemPlacementState& PlacementState)
{
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor = Owner->GetOwner();
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	checkf(ItemComponent != nullptr, TEXT("ItemComponent must be valid when adding an item to the inventory fast array"));
	checkf(Callbacks.CreateItemFromPickup, TEXT("CreateItemFromPickup callback must be configured"));

	FINV_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Callbacks.CreateItemFromPickup(ItemComponent, OwningActor);
	checkf(NewEntry.Item != nullptr, TEXT("CreateItemFromPickup callback returned null"));

	NewEntry.ItemInstanceId = FGuid::NewGuid();
	NewEntry.DefinitionHandle = NewEntry.Item->GetItemDefinitionHandle();
	NewEntry.InstanceState = NewEntry.Item->GetItemInstanceState();
	NewEntry.PlacementState = PlacementState;
	NewEntry.bUseItemRarity = NewEntry.Item->IsItemRarityEnabled();
	NewEntry.ItemRarityTag = NewEntry.Item->GetItemRarityTag();

	NewEntry.Item->InitializeFromReplicatedData(
		NewEntry.ItemInstanceId,
		NewEntry.DefinitionHandle,
		NewEntry.InstanceState,
		NewEntry.PlacementState,
		NewEntry.bUseItemRarity,
		NewEntry.ItemRarityTag);

	MarkItemDirty(NewEntry);
	return NewEntry.Item;
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_InventoryItem* Item, const FINV_ItemPlacementState& PlacementState)
{
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor = Owner->GetOwner();
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	checkf(Item != nullptr, TEXT("Item must be valid when adding an item to the inventory fast array"));

	FINV_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;
	NewEntry.ItemInstanceId = Item->GetItemInstanceId().IsValid() ? Item->GetItemInstanceId() : FGuid::NewGuid();
	NewEntry.DefinitionHandle = Item->GetItemDefinitionHandle();
	NewEntry.InstanceState = Item->GetItemInstanceState();
	NewEntry.PlacementState = PlacementState;
	NewEntry.bUseItemRarity = Item->IsItemRarityEnabled();
	NewEntry.ItemRarityTag = Item->GetItemRarityTag();

	NewEntry.Item->InitializeFromReplicatedData(
		NewEntry.ItemInstanceId,
		NewEntry.DefinitionHandle,
		NewEntry.InstanceState,
		NewEntry.PlacementState,
		NewEntry.bUseItemRarity,
		NewEntry.ItemRarityTag);

	MarkItemDirty(NewEntry);
	return Item;
}

void FINV_InventoryFastArray::SyncItem(UINV_InventoryItem* Item)
{
	FINV_InventoryEntry* Entry = FindEntry(Item);
	if (Entry == nullptr || !IsValid(Item))
	{
		return;
	}

	Entry->DefinitionHandle = Item->GetItemDefinitionHandle();
	Entry->ItemInstanceId = Item->GetItemInstanceId();
	Entry->InstanceState = Item->GetItemInstanceState();
	Entry->PlacementState = Item->GetPlacementState();
	Entry->bUseItemRarity = Item->IsItemRarityEnabled();
	Entry->ItemRarityTag = Item->GetItemRarityTag();
	MarkItemDirty(*Entry);
	NotifyItemChanged(Item);
}

void FINV_InventoryFastArray::RemoveEntry(UINV_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FINV_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			NotifyItemRemoved(Entry.Item.Get());
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
			return;
		}
	}
}

const UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag) const
{
	if (!Callbacks.MatchesItemByTypeAndRarity)
	{
		return nullptr;
	}

	const FINV_InventoryEntry* FoundItem = Entries.FindByPredicate([this, ItemType, bUseItemRarity, ItemRarityTag](const FINV_InventoryEntry& Entry)
	{
		return Entry.Item != nullptr && Callbacks.MatchesItemByTypeAndRarity(Entry.Item, ItemType, bUseItemRarity, ItemRarityTag);
	});
	return FoundItem ? FoundItem->Item : nullptr;
}

UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	return const_cast<UINV_InventoryItem*>(
		const_cast<const FINV_InventoryFastArray*>(this)->FindFirstItemByType(ItemType, bUseItemRarity, ItemRarityTag)
	);
}
