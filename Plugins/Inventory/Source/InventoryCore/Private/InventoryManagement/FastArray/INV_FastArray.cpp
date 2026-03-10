// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"

namespace
{
FIntPoint ResolveItemFootprint(const UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement)
{
	if (!IsValid(Item))
	{
		return FIntPoint::ZeroValue;
	}

	const FINV_GridFragment* GridFragment = Item->GetCachedGridFragment();
	const FIntPoint BaseFootprint = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	return Placement.ResolveFootprint(BaseFootprint);
}

bool MarkPlacementFootprint(
	TArray<bool>& Occupancy,
	const FIntPoint& GridSize,
	const FINV_InventoryItemPlacement& Placement,
	const FIntPoint& Footprint,
	const bool bMarkOccupied)
{
	if (!Placement.IsValid() || GridSize.X <= 0 || GridSize.Y <= 0 || Footprint.X <= 0 || Footprint.Y <= 0)
	{
		return false;
	}

	if (Placement.Anchor.X + Footprint.X > GridSize.X || Placement.Anchor.Y + Footprint.Y > GridSize.Y)
	{
		return false;
	}

	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const int32 TileIndex = (Placement.Anchor.Y + Y) * GridSize.X + (Placement.Anchor.X + X);
			if (!Occupancy.IsValidIndex(TileIndex))
			{
				return false;
			}

			if (bMarkOccupied && Occupancy[TileIndex])
			{
				return false;
			}
		}
	}

	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const int32 TileIndex = (Placement.Anchor.Y + Y) * GridSize.X + (Placement.Anchor.X + X);
			Occupancy[TileIndex] = bMarkOccupied;
		}
	}

	return true;
}
}

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

bool FINV_InventoryFastArray::ContainsItem(const UINV_InventoryItem* Item) const
{
	return FindEntry(Item) != nullptr;
}

bool FINV_InventoryFastArray::GetItemPlacement(const UINV_InventoryItem* Item, FINV_InventoryItemPlacement& OutPlacement) const
{
	const FINV_InventoryEntry* Entry = FindEntry(Item);
	if (!Entry)
	{
		return false;
	}

	OutPlacement = Entry->Placement;
	return Entry->Placement.IsValid();
}

bool FINV_InventoryFastArray::SetItemPlacement(UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement)
{
	FINV_InventoryEntry* Entry = FindEntry(Item);
	if (!Entry)
	{
		return false;
	}

	Entry->Placement = Placement;
	MarkItemDirty(*Entry);
	return true;
}

bool FINV_InventoryFastArray::ClearItemPlacement(UINV_InventoryItem* Item)
{
	FINV_InventoryEntry* Entry = FindEntry(Item);
	if (!Entry)
	{
		return false;
	}

	Entry->Placement.Reset();
	MarkItemDirty(*Entry);
	return true;
}

bool FINV_InventoryFastArray::CanPlaceItemAt(
	const UINV_InventoryItem* Item,
	const FINV_InventoryItemPlacement& Placement,
	const FIntPoint& GridSize,
	TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
	const UINV_InventoryItem* IgnoredItem) const
{
	if (!IsValid(Item) || !Placement.IsValid() || GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return false;
	}

	TArray<bool> Occupancy;
	Occupancy.Init(false, GridSize.X * GridSize.Y);

	for (const FINV_InventoryEntry& Entry : Entries)
	{
		UINV_InventoryItem* ExistingItem = Entry.Item.Get();
		if (!IsValid(ExistingItem) || ExistingItem == IgnoredItem || !ShouldIncludeItem(ExistingItem))
		{
			continue;
		}

		const FIntPoint ExistingFootprint = ResolveItemFootprint(ExistingItem, Entry.Placement);
		if (!MarkPlacementFootprint(Occupancy, GridSize, Entry.Placement, ExistingFootprint, true))
		{
			return false;
		}
	}

	const FIntPoint CandidateFootprint = ResolveItemFootprint(Item, Placement);
	return MarkPlacementFootprint(Occupancy, GridSize, Placement, CandidateFootprint, true);
}

bool FINV_InventoryFastArray::TryFindFirstAvailablePlacement(
	const UINV_InventoryItem* Item,
	const FIntPoint& GridSize,
	FINV_InventoryItemPlacement& OutPlacement,
	TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
	const UINV_InventoryItem* IgnoredItem) const
{
	OutPlacement.Reset();

	if (!IsValid(Item) || GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return false;
	}

	TArray<bool> Occupancy;
	Occupancy.Init(false, GridSize.X * GridSize.Y);

	for (const FINV_InventoryEntry& Entry : Entries)
	{
		UINV_InventoryItem* ExistingItem = Entry.Item.Get();
		if (!IsValid(ExistingItem) || ExistingItem == IgnoredItem || !ShouldIncludeItem(ExistingItem))
		{
			continue;
		}

		const FIntPoint ExistingFootprint = ResolveItemFootprint(ExistingItem, Entry.Placement);
		if (!MarkPlacementFootprint(Occupancy, GridSize, Entry.Placement, ExistingFootprint, true))
		{
			return false;
		}
	}

	const FIntPoint CandidateFootprint = ResolveItemFootprint(Item, FINV_InventoryItemPlacement{});
	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			FINV_InventoryItemPlacement CandidatePlacement;
			CandidatePlacement.Anchor = FIntPoint(X, Y);
			if (!MarkPlacementFootprint(Occupancy, GridSize, CandidatePlacement, CandidateFootprint, true))
			{
				continue;
			}

			OutPlacement = CandidatePlacement;
			return true;
		}
	}

	return false;
}

bool FINV_InventoryFastArray::TryAutoPlaceItem(
	UINV_InventoryItem* Item,
	const FIntPoint& GridSize,
	TFunctionRef<bool(const UINV_InventoryItem*)> ShouldIncludeItem,
	const UINV_InventoryItem* IgnoredItem)
{
	FINV_InventoryItemPlacement Placement;
	if (!TryFindFirstAvailablePlacement(Item, GridSize, Placement, ShouldIncludeItem, IgnoredItem))
	{
		return false;
	}

	return SetItemPlacement(Item, Placement);
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

void FINV_InventoryFastArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		if (!Entries.IsValidIndex(Index))
		{
			continue;
		}

		if (Callbacks.OnItemChanged)
		{
			Callbacks.OnItemChanged(Entries[Index].Item);
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

FINV_InventoryEntry* FINV_InventoryFastArray::FindEntry(UINV_InventoryItem* Item)
{
	return Entries.FindByPredicate([Item](const FINV_InventoryEntry& Entry)
	{
		return Entry.Item == Item;
	});
}

const FINV_InventoryEntry* FINV_InventoryFastArray::FindEntry(const UINV_InventoryItem* Item) const
{
	return Entries.FindByPredicate([Item](const FINV_InventoryEntry& Entry)
	{
		return Entry.Item == Item;
	});
}
