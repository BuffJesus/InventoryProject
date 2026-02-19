// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Placement/INV_GridPlacementEngine.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

FINV_SlotAvailabilityResult FINV_GridPlacementEngine::HasRoomForItem(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const FINV_ItemManifest& Manifest,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridPlacementEngine_HasRoomForItem);
	// Walk the grid and compute how much space we can fill.
	FINV_SlotAvailabilityResult Result;

	// Determine if item is stackable
	const FINV_StackableFragment* StackableFragment { Manifest.GetFragmentOfType<FINV_StackableFragment>() };
	Result.bStackable = StackableFragment != nullptr;

	// Determine how many stacks to add
	const int32 MaxStackSize { StackableFragment ? StackableFragment->GetMaxStackSize() : 1 };
	int32 AmountToFill { StackableFragment ? StackableFragment->GetStackCount() : 1 };

	TSet<int32> CheckedIndices;

	// For stackable items, top off existing matching stacks before searching for empty slots.
	if (Result.bStackable)
	{
		for (int32 GridIndex = 0; GridIndex < GridSlots.Num(); ++GridIndex)
		{
			const TObjectPtr<UINV_GridSlot> GridSlot = GridSlots[GridIndex];
			if (!IsValid(GridSlot)) continue;
			if (!HasValidItem(GridSlot)) continue;

			const UINV_InventoryItem* ExistingItem = GridSlot->GetInventoryItem().Get();
			if (!IsValid(ExistingItem)) continue;
			if (!IsStackCompatible(ExistingItem, Manifest.GetItemType(), bUseItemRarity, ItemRarityTag)) continue;

			// Get the upper-left index to avoid checking the same stack multiple times
			const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex();
			if (CheckedIndices.Contains(UpperLeftIndex))
			{
				// Already checked this stack
				continue;
			}

			const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlots, GridSlot);
			if (AmountToFillInSlot <= 0) continue;

			CheckedIndices.Add(UpperLeftIndex);

			Result.TotalRoomToFill += AmountToFillInSlot;
			Result.SlotAvailabilities.Emplace(
				FINV_SlotAvailability{
					UpperLeftIndex,
					Result.bStackable ? AmountToFillInSlot : 0,
					true
				}
			);

			AmountToFill -= AmountToFillInSlot;
			Result.Remainder = AmountToFill;

			if (AmountToFill == 0) return Result;
		}
	}

	// For each GridSlot, check if we can place a new item
	const FIntPoint ItemDimensions = GetItemDimensions(Manifest);
	for (const auto& GridSlot : GridSlots)
	{
	    // if no more to fill, break from loop
	    if (AmountToFill == 0) break;

	    // is index claimed?
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetTileIndex())) continue;

		// is the item inside grid bounds?
		if (!IsInGridBounds(GridSlot->GetTileIndex(), ItemDimensions, GridSize)) continue;

	    // can item fit (i.e., no obstructions)?
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlots, GridSize, GridSlot, ItemDimensions, CheckedIndices, TentativelyClaimed,
			Manifest.GetItemType(), bUseItemRarity, ItemRarityTag, MaxStackSize))
		{
			continue;
		}

	    // how much to fill?
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlots, GridSlot);
		if (AmountToFillInSlot == 0) continue;

		CheckedIndices.Append(TentativelyClaimed);

	    // update amount left to fill
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FINV_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetTileIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}
		);

		AmountToFill -= AmountToFillInSlot;
		Result.Remainder = AmountToFill;

		if (AmountToFill == 0) return Result;
	}
	return Result;
}

FINV_SpaceQueryResult FINV_GridPlacementEngine::CheckHoverPosition(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const FIntPoint& Position,
	const FIntPoint& Dimensions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridPlacementEngine_CheckHoverPosition);
	FINV_SpaceQueryResult Result;

	// Convert position to index
	const int32 StartIndex = Position.Y * GridSize.X + Position.X;

	// in grid bounds?
	if (!IsInGridBounds(StartIndex, Dimensions, GridSize)) return Result;

	Result.bHasSpace = true;

	// if more than one of the indices is occupied with the same item, need to check if all have same upper left index
	TSet<int32> OccupiedUpperLeftIndices;
	FINV_GridIteration::ForEach2D(GridSlots, StartIndex, Dimensions, GridSize.X,
		[&](const UINV_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});

	// if yes, only one item in the way? (can we swap?)
	if (OccupiedUpperLeftIndices.Num() == 1) // single item at position, valid for swapping/combining
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}

	Result.BlockingUpperLeftIndices = OccupiedUpperLeftIndices.Array();

	return Result;
}

FIntPoint FINV_GridPlacementEngine::CalculateStartingCoordinate(
	const FIntPoint& CursorCoord,
	const FIntPoint& ItemDimensions,
	const EINV_TileQuadrant CursorQuadrant)
{
	const int32 HasEvenWidth = ItemDimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = ItemDimensions.Y % 2 == 0 ? 1 : 0;

	FIntPoint StartingCoord;
	switch (CursorQuadrant)
	{
	case EINV_TileQuadrant::TopLeft:
			StartingCoord.X = CursorCoord.X - FMath::FloorToInt(ItemDimensions.X * 0.5f);
			StartingCoord.Y = CursorCoord.Y - FMath::FloorToInt(ItemDimensions.Y * 0.5f);
		break;
		case EINV_TileQuadrant::TopRight:
			StartingCoord.X = CursorCoord.X - FMath::FloorToInt(ItemDimensions.X * 0.5f) + HasEvenWidth;
			StartingCoord.Y = CursorCoord.Y - FMath::FloorToInt(ItemDimensions.Y * 0.5f);
		break;
		case EINV_TileQuadrant::BottomLeft:
			StartingCoord.X = CursorCoord.X - FMath::FloorToInt(ItemDimensions.X * 0.5f);
			StartingCoord.Y = CursorCoord.Y - FMath::FloorToInt(ItemDimensions.Y * 0.5f) + HasEvenHeight;
		break;
		case EINV_TileQuadrant::BottomRight:
			StartingCoord.X = CursorCoord.X - FMath::FloorToInt(ItemDimensions.X * 0.5f) + HasEvenWidth;
			StartingCoord.Y = CursorCoord.Y - FMath::FloorToInt(ItemDimensions.Y * 0.5f) + HasEvenHeight;
		break;
	default:
		return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

bool FINV_GridPlacementEngine::IsInGridBounds(
	const int32 StartIndex,
	const FIntPoint& ItemDimensions,
	const FIntPoint& GridSize)
{
	if (StartIndex < 0 || StartIndex >= GridSize.X * GridSize.Y) return false;
	const int32 EndColumn = (StartIndex % GridSize.X) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / GridSize.X) + ItemDimensions.Y;
	return EndColumn <= GridSize.X && EndRow <= GridSize.Y;
}

bool FINV_GridPlacementEngine::IsStackCompatible(
	const UINV_InventoryItem* ExistingItem,
	const FGameplayTag& ItemType,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	if (!IsValid(ExistingItem)) return false;
	if (!ExistingItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType)) return false;
	if (ExistingItem->IsItemRarityEnabled() != bUseItemRarity) return false;
	if (!bUseItemRarity) return true;
	return ExistingItem->GetItemRarityTag().MatchesTagExact(ItemRarityTag);
}

FIntPoint FINV_GridPlacementEngine::GetItemDimensions(const FINV_ItemManifest& Manifest)
{
	const FINV_GridFragment* GridFragment { Manifest.GetFragmentOfType<FINV_GridFragment>() };
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
}

int32 FINV_GridPlacementEngine::DetermineFillAmountForSlot(
	const bool bStackable,
	const int32 MaxStackSize,
	const int32 AmountToFill,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const UINV_GridSlot* GridSlot)
{
	// Get the actual stack count (handles multi-tile items by checking upper-left slot)
	const int32 CurrentStackCount = GetStackAmount(GridSlots, GridSlot);

	// calculate room in the slot
	const int32 RoomInSlot = MaxStackSize - CurrentStackCount;

	// if stackable, need min between amount to fill and room in slot
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 FINV_GridPlacementEngine::GetStackAmount(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const UINV_GridSlot* GridSlot)
{
	int32 CurrentSlotStackCount { GridSlot->GetStackCount() };
	// if we are at a slot that doesn't hold stack count, must get actual stack count
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		if (GridSlots.IsValidIndex(UpperLeftIndex))
		{
			const UINV_GridSlot* UpperLeftGridSlot { GridSlots[UpperLeftIndex] };
			CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
		}
	}
	return CurrentSlotStackCount;
}

bool FINV_GridPlacementEngine::HasRoomAtIndex(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const UINV_GridSlot* GridSlot,
	const FIntPoint& Dimensions,
	const TSet<int32>& CheckedIndices,
	TSet<int32>& OutTentativelyClaimed,
	const FGameplayTag& ItemType,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag,
	const int32 MaxStackSize)
{
	// is there room at index (i.e., other items in the way)?
	bool bHasRoomAtIndex = true;
	FINV_GridIteration::ForEach2D(GridSlots, GridSlot->GetTileIndex(), Dimensions, GridSize.X,
		[&](const UINV_GridSlot* SubGridSlot)
		{
			if (CheckSlotConstraints(GridSlots, GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed,
				ItemType, bUseItemRarity, ItemRarityTag, MaxStackSize))
			{
				OutTentativelyClaimed.Add(SubGridSlot->GetTileIndex());
			}
			else
			{
				bHasRoomAtIndex = false;
			}
		});

	return bHasRoomAtIndex;
}

bool FINV_GridPlacementEngine::CheckSlotConstraints(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const UINV_GridSlot* GridSlot,
	const UINV_GridSlot* SubGridSlot,
	const TSet<int32>& CheckedIndices,
	TSet<int32>& OutTentativelyClaimed,
	const FGameplayTag& ItemType,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag,
	const int32 MaxStackSize)
{
	// index claimed?
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetTileIndex())) return false;

	// has valid item?
	if (!HasValidItem(SubGridSlot)) return true;

	// is this grid slot upper left slot?
	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;

	// if yes, is stackable?
	const UINV_InventoryItem* SubItem { SubGridSlot->GetInventoryItem().Get() };
	if (!SubItem->IsStackable()) return false;

	// is item same type as the item trying to add?
	if (!IsStackCompatible(SubItem, ItemType, bUseItemRarity, ItemRarityTag)) return false;

	// if yes, is slot at max stack size already?
	if (GridSlot->GetStackCount() >= MaxStackSize) return false;

	return true;
}

bool FINV_GridPlacementEngine::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index)
{
	return CheckedIndices.Contains(Index);
}

bool FINV_GridPlacementEngine::HasValidItem(const UINV_GridSlot* GridSlot)
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool FINV_GridPlacementEngine::IsUpperLeftSlot(const UINV_GridSlot* GridSlot, const UINV_GridSlot* SubGridSlot)
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetTileIndex();
}
