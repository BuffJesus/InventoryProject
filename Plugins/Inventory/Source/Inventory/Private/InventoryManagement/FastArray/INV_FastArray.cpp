// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "Items/INV_ItemComponent.h"
#include "Items/INV_InventoryItem.h"
#include "Tests/ToolMenusTestUtilities.h"

TArray<UINV_InventoryItem*> FINV_InventoryFastArray::GetAllItems() const
{
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
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	if (!IsValid(IC)) return;
	for (int32 Index : RemovedIndices)
	{
		IC->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FINV_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	if (!IsValid(IC)) return;
	for (int32 Index : AddedIndices)
	{
		IC->OnItemAdded.Broadcast(Entries[Index].Item);
	}
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_ItemComponent* ItemComponent)
{
	checkf(Owner, TEXT("Owner cannot be null when adding an item to the inventory fast array"));
	AActor* OwningActor { Owner->GetOwner() };
	checkf(OwningActor->HasAuthority(), TEXT("Only the owning actor can add items to the inventory fast array"));
	UINV_InventoryComponent* IC { Cast<UINV_InventoryComponent>(Owner) };
	checkf(IsValid(IC), TEXT("ItemComponent must be valid when adding an item to the inventory fast array"));
	
	FINV_InventoryEntry& NewEntry { Entries.AddDefaulted_GetRef() };
	NewEntry.Item = ItemComponent->GetItemManifest().CreateItem(OwningActor);
	
	IC->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);
	
	return NewEntry.Item;
}

UINV_InventoryItem* FINV_InventoryFastArray::AddEntry(UINV_InventoryItem* Item)
{
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

UINV_InventoryItem* FINV_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType)
{
	auto* FoundItem { Entries.FindByPredicate([ItemType = ItemType](const FINV_InventoryEntry& Entry)
	{
		return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
	}) };
	return FoundItem ? FoundItem->Item : nullptr;
}
