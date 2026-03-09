// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Placement/INV_GridPlacementEngine.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"
#include "UI/Inventory/Placement/INV_GridOccupancyModel.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

FINV_SlotAvailabilityResult FINV_GridPlacementEngine::HasRoomForItem(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const FINV_ItemManifest& Manifest,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	FINV_GridOccupancyModel OccupancyModel;
	OccupancyModel.RebuildFromGridSlots(GridSlots, GridSize);
	return HasRoomForItem(OccupancyModel, Manifest, bUseItemRarity, ItemRarityTag);
}

FINV_SlotAvailabilityResult FINV_GridPlacementEngine::HasRoomForItem(
	const FINV_GridOccupancyModel& OccupancyModel,
	const FINV_ItemManifest& Manifest,
	const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridPlacementEngine_HasRoomForItem);
	FINV_SlotAvailabilityResult Result;

	const FINV_StackableFragment* StackableFragment { Manifest.GetFragmentOfType<FINV_StackableFragment>() };
	Result.bStackable = StackableFragment != nullptr;
	const int32 MaxStackSize { StackableFragment ? StackableFragment->GetMaxStackSize() : 1 };
	int32 AmountToFill { StackableFragment ? StackableFragment->GetStackCount() : 1 };
	TSet<int32> CheckedAnchors;
	const FIntPoint GridSize = OccupancyModel.GetGridSize();

	if (Result.bStackable)
	{
		TArray<int32> SortedAnchors;
		OccupancyModel.GetSortedAnchors(SortedAnchors);
		for (const int32 AnchorIndex : SortedAnchors)
		{
			const UINV_InventoryItem* ExistingItem = OccupancyModel.GetItemAtAnchor(AnchorIndex);
			if (!IsValid(ExistingItem)) continue;
			if (!IsStackCompatible(ExistingItem, Manifest.GetItemType(), bUseItemRarity, ItemRarityTag)) continue;
			if (CheckedAnchors.Contains(AnchorIndex)) continue;

			const int32 CurrentStackCount = OccupancyModel.GetStackCountForAnchor(AnchorIndex);
			const int32 AmountToFillInSlot = Result.bStackable ? FMath::Min(AmountToFill, MaxStackSize - CurrentStackCount) : 1;
			if (AmountToFillInSlot <= 0) continue;

			CheckedAnchors.Add(AnchorIndex);

			Result.TotalRoomToFill += AmountToFillInSlot;
			Result.SlotAvailabilities.Emplace(
				FINV_SlotAvailability{
					AnchorIndex,
					Result.bStackable ? AmountToFillInSlot : 0,
					true
				}
			);

			AmountToFill -= AmountToFillInSlot;
			Result.Remainder = AmountToFill;

			if (AmountToFill == 0) return Result;
		}
	}

	const FIntPoint ItemDimensions = GetItemDimensions(Manifest);
	for (int32 TileIndex = 0; TileIndex < GridSize.X * GridSize.Y; ++TileIndex)
	{
		if (AmountToFill == 0) break;
		if (CheckedAnchors.Contains(TileIndex)) continue;
		if (!IsInGridBounds(TileIndex, ItemDimensions, GridSize)) continue;

		bool bHasRoomAtIndex = true;
		int32 StackableAnchorAtIndex = INDEX_NONE;
		const int32 StartX = TileIndex % GridSize.X;
		const int32 StartY = TileIndex / GridSize.X;
		for (int32 Y = 0; Y < ItemDimensions.Y && bHasRoomAtIndex; ++Y)
		{
			for (int32 X = 0; X < ItemDimensions.X; ++X)
			{
				const int32 SubIndex = (StartY + Y) * GridSize.X + (StartX + X);
				const int32 OccupiedAnchor = OccupancyModel.GetAnchorAtTile(SubIndex);
				if (OccupiedAnchor == INDEX_NONE)
				{
					continue;
				}

				if (CheckedAnchors.Contains(OccupiedAnchor))
				{
					bHasRoomAtIndex = false;
					break;
				}

				if (OccupiedAnchor != SubIndex)
				{
					bHasRoomAtIndex = false;
					break;
				}

				const UINV_InventoryItem* ExistingItem = OccupancyModel.GetItemAtAnchor(OccupiedAnchor);
				if (!IsValid(ExistingItem) || !ExistingItem->IsStackable())
				{
					bHasRoomAtIndex = false;
					break;
				}

				if (!IsStackCompatible(ExistingItem, Manifest.GetItemType(), bUseItemRarity, ItemRarityTag))
				{
					bHasRoomAtIndex = false;
					break;
				}

				if (OccupancyModel.GetStackCountForAnchor(OccupiedAnchor) >= MaxStackSize)
				{
					bHasRoomAtIndex = false;
					break;
				}

				if (StackableAnchorAtIndex != INDEX_NONE && StackableAnchorAtIndex != OccupiedAnchor)
				{
					bHasRoomAtIndex = false;
					break;
				}

				StackableAnchorAtIndex = OccupiedAnchor;
			}
		}
		if (!bHasRoomAtIndex) continue;

		const int32 TargetIndex = StackableAnchorAtIndex != INDEX_NONE ? StackableAnchorAtIndex : TileIndex;
		const int32 CurrentStackCount = StackableAnchorAtIndex != INDEX_NONE ? OccupancyModel.GetStackCountForAnchor(StackableAnchorAtIndex) : 0;
		const int32 AmountToFillInSlot = Result.bStackable ? FMath::Min(AmountToFill, MaxStackSize - CurrentStackCount) : 1;
		if (AmountToFillInSlot == 0) continue;

		CheckedAnchors.Add(TargetIndex);

		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FINV_SlotAvailability{
				TargetIndex,
				Result.bStackable ? AmountToFillInSlot : 0,
				StackableAnchorAtIndex != INDEX_NONE
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
	FINV_GridOccupancyModel OccupancyModel;
	OccupancyModel.RebuildFromGridSlots(GridSlots, GridSize);
	return CheckHoverPosition(OccupancyModel, Position, Dimensions);
}

FINV_SpaceQueryResult FINV_GridPlacementEngine::CheckHoverPosition(
	const FINV_GridOccupancyModel& OccupancyModel,
	const FIntPoint& Position,
	const FIntPoint& Dimensions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridPlacementEngine_CheckHoverPosition);
	FINV_SpaceQueryResult Result;
	const FIntPoint& GridSize = OccupancyModel.GetGridSize();

	const int32 StartIndex = Position.Y * GridSize.X + Position.X;
	if (!IsInGridBounds(StartIndex, Dimensions, GridSize)) return Result;

	Result.bHasSpace = true;
	TSet<int32> OccupiedUpperLeftIndices;

	const int32 StartX = StartIndex % GridSize.X;
	const int32 StartY = StartIndex / GridSize.X;
	for (int32 Y = 0; Y < Dimensions.Y; ++Y)
	{
		for (int32 X = 0; X < Dimensions.X; ++X)
		{
			const int32 TileIndex = (StartY + Y) * GridSize.X + (StartX + X);
			const int32 AnchorIndex = OccupancyModel.GetAnchorAtTile(TileIndex);
			if (AnchorIndex == INDEX_NONE)
			{
				continue;
			}

			OccupiedUpperLeftIndices.Add(AnchorIndex);
			Result.bHasSpace = false;
		}
	}

	if (OccupiedUpperLeftIndices.Num() == 1) // single item at position, valid for swapping/combining
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateIterator();
		if (UINV_InventoryItem* BlockingItem = OccupancyModel.GetItemAtAnchor(Index); IsValid(BlockingItem))
		{
			Result.ValidItem = BlockingItem;
			Result.UpperLeftIndex = Index;
		}
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
	return UINV_InventoryItem::MatchesTypeAndRarity(ExistingItem, ItemType, bUseItemRarity, ItemRarityTag);
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
			if (!IsValid(SubGridSlot))
			{
				bHasRoomAtIndex = false;
				return;
			}
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
