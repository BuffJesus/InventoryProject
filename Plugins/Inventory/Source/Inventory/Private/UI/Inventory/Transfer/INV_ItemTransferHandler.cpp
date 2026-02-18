// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Transfer/INV_ItemTransferHandler.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "InventoryManagement/GridPlacement/INV_GridPlacementEngine.h"

FINV_SwapResult FINV_ItemTransferHandler::PlanSwapOperation(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const FIntPoint& HoverItemDimensions,
	const int32 TargetDropIndex,
	const int32 ClickedIndex)
{
	FINV_SwapResult Result;

	if (!GridSlots.IsValidIndex(TargetDropIndex) || !GridSlots.IsValidIndex(ClickedIndex))
	{
		return Result;
	}

	if (!FINV_GridPlacementEngine::IsInGridBounds(TargetDropIndex, HoverItemDimensions, GridSize))
	{
		return Result;
	}

	// Gather all unique overlapped items under the hovered footprint
	TSet<int32> OverlappedUpperLeftIndices;
	FINV_GridIteration::ForEach2D(GridSlots, TargetDropIndex, HoverItemDimensions, GridSize.X,
		[&](const UINV_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OverlappedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
		}
	});

	// Only allow swap when the clicked item is one of the overlapped blockers
	if (!OverlappedUpperLeftIndices.Contains(ClickedIndex))
	{
		return Result;
	}

	struct FDisplacedItemPlan
	{
		UINV_InventoryItem* Item { nullptr };
		int32 SourceIndex { INDEX_NONE };
		int32 TargetIndex { INDEX_NONE };
		int32 StackCount { 0 };
		bool bStackable { false };
		FIntPoint Dimensions { 1, 1 };
	};

	TArray<FDisplacedItemPlan> DisplacedItems;
	DisplacedItems.Reserve(OverlappedUpperLeftIndices.Num());

	// Build candidate list for all overlapped blockers except the clicked one
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (OverlappedIndex == ClickedIndex) continue;
		if (!GridSlots.IsValidIndex(OverlappedIndex)) return Result;

		UINV_GridSlot* SourceSlot = GridSlots[OverlappedIndex];
		UINV_InventoryItem* SourceItem = SourceSlot->GetInventoryItem().Get();
		if (!IsValid(SourceItem)) return Result;

		const FINV_GridFragment* SourceGridFragment = SourceItem->GetItemManifest().GetFragmentOfType<FINV_GridFragment>();
		if (!SourceGridFragment) return Result;

		DisplacedItems.Add(FDisplacedItemPlan{
			SourceItem,
			OverlappedIndex,
			INDEX_NONE,
			SourceSlot->GetStackCount(),
			SourceItem->IsStackable(),
			SourceGridFragment->GetGridSize()
		});
	}

	// Build occupancy simulation
	TArray<bool> SimulatedOccupied = BuildOccupancyMap(GridSlots);

	// Free all overlapped blockers, then reserve target footprint for hovered item
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(OverlappedIndex)) return Result;
		UINV_InventoryItem* SourceItem = GridSlots[OverlappedIndex]->GetInventoryItem().Get();
		if (!IsValid(SourceItem)) return Result;

		const FINV_GridFragment* SourceGridFragment = SourceItem->GetItemManifest().GetFragmentOfType<FINV_GridFragment>();
		if (!SourceGridFragment) return Result;

		MarkFootprint(SimulatedOccupied, GridSlots, GridSize, OverlappedIndex, SourceGridFragment->GetGridSize(), false);
	}
	MarkFootprint(SimulatedOccupied, GridSlots, GridSize, TargetDropIndex, HoverItemDimensions, true);

	// Plan destinations for all displaced blockers using first-fit
	const int32 MaxSearchIterations = GridSlots.Num();
	for (FDisplacedItemPlan& Plan : DisplacedItems)
	{
		int32 IterationCount = 0;
		bool bFoundPlacement = false;

		for (int32 CandidateIndex = 0; CandidateIndex < GridSlots.Num(); ++CandidateIndex)
		{
			if (++IterationCount > MaxSearchIterations)
			{
				// Safety limit reached
				return Result;
			}

			if (!CanFitAtIndex(SimulatedOccupied, GridSlots, GridSize, CandidateIndex, Plan.Dimensions))
			{
				continue;
			}

			Plan.TargetIndex = CandidateIndex;
			MarkFootprint(SimulatedOccupied, GridSlots, GridSize, CandidateIndex, Plan.Dimensions, true);
			bFoundPlacement = true;
			break;
		}

		if (!bFoundPlacement)
		{
			// Not enough room to relocate all blockers
			return Result;
		}
	}

	// Build successful result
	Result.bSuccess = true;
	Result.PlacementIndex = TargetDropIndex;

	// Add all overlapped items to removal list
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(OverlappedIndex)) continue;
		UINV_InventoryItem* Item = GridSlots[OverlappedIndex]->GetInventoryItem().Get();
		if (IsValid(Item))
		{
			Result.ItemsToRemove.Add(TPair<UINV_InventoryItem*, int32>(Item, OverlappedIndex));
		}
	}

	// Add displaced items to relocation list
	for (const FDisplacedItemPlan& Plan : DisplacedItems)
	{
		Result.ItemsToRelocate.Add(TPair<UINV_InventoryItem*, int32>(Plan.Item, Plan.TargetIndex));
		Result.RelocationStackCounts.Add(Plan.StackCount);
		Result.RelocationStackableFlags.Add(Plan.bStackable);
	}

	return Result;
}

void FINV_ItemTransferHandler::CalculateStackTransfer(
	const int32 SourceStackCount,
	const int32 TargetStackCount,
	const int32 MaxStackSize,
	int32& OutAmountToTransfer,
	int32& OutSourceRemaining,
	int32& OutTargetFinal)
{
	const int32 TargetAvailableSpace = FMath::Max(0, MaxStackSize - TargetStackCount);
	OutAmountToTransfer = FMath::Min(SourceStackCount, TargetAvailableSpace);
	OutSourceRemaining = SourceStackCount - OutAmountToTransfer;
	OutTargetFinal = TargetStackCount + OutAmountToTransfer;
}

bool FINV_ItemTransferHandler::AreItemsStackCompatible(
	const UINV_InventoryItem* SourceItem,
	const UINV_InventoryItem* TargetItem)
{
	if (!IsValid(SourceItem) || !IsValid(TargetItem))
	{
		return false;
	}

	if (!SourceItem->IsStackable() || !TargetItem->IsStackable())
	{
		return false;
	}

	// Items must have same type
	const FINV_ItemManifest& SourceManifest = SourceItem->GetItemManifest();
	const FINV_ItemManifest& TargetManifest = TargetItem->GetItemManifest();

	if (SourceManifest.GetItemType() != TargetManifest.GetItemType())
	{
		return false;
	}

	// If rarity is enabled, items must have same rarity
	if (SourceItem->IsItemRarityEnabled() && TargetItem->IsItemRarityEnabled())
	{
		if (SourceItem->GetItemRarityTag() != TargetItem->GetItemRarityTag())
		{
			return false;
		}
	}

	return true;
}

TArray<bool> FINV_ItemTransferHandler::BuildOccupancyMap(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots)
{
	TArray<bool> OccupancyMap;
	OccupancyMap.Init(false, GridSlots.Num());

	for (int32 Index = 0; Index < GridSlots.Num(); ++Index)
	{
		OccupancyMap[Index] = GridSlots[Index]->GetInventoryItem().IsValid();
	}

	return OccupancyMap;
}

void FINV_ItemTransferHandler::MarkFootprint(
	TArray<bool>& OccupancyMap,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const int32 StartIndex,
	const FIntPoint& Dimensions,
	const bool bOccupied)
{
	FINV_GridIteration::ForEach2D(GridSlots, StartIndex, Dimensions, GridSize.X,
		[&](const UINV_GridSlot* GridSlotRef)
	{
		OccupancyMap[GridSlotRef->GetTileIndex()] = bOccupied;
	});
}

bool FINV_ItemTransferHandler::CanFitAtIndex(
	const TArray<bool>& OccupancyMap,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const int32 StartIndex,
	const FIntPoint& Dimensions)
{
	if (!FINV_GridPlacementEngine::IsInGridBounds(StartIndex, Dimensions, GridSize))
	{
		return false;
	}

	bool bCanFit = true;
	FINV_GridIteration::ForEach2D(GridSlots, StartIndex, Dimensions, GridSize.X,
		[&](const UINV_GridSlot* GridSlotRef)
	{
		if (OccupancyMap[GridSlotRef->GetTileIndex()])
		{
			bCanFit = false;
		}
	});

	return bCanFit;
}

bool FINV_ItemTransferHandler::FindFirstFitPlacement(
	const TArray<bool>& OccupancyMap,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const FIntPoint& Dimensions,
	int32& OutIndex)
{
	for (int32 CandidateIndex = 0; CandidateIndex < GridSlots.Num(); ++CandidateIndex)
	{
		if (CanFitAtIndex(OccupancyMap, GridSlots, GridSize, CandidateIndex, Dimensions))
		{
			OutIndex = CandidateIndex;
			return true;
		}
	}

	OutIndex = INDEX_NONE;
	return false;
}
