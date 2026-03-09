// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/Transfer/INV_StorageTransferUtils.h"

#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"

bool FINV_StorageTransferUtils::CanInventoryFitItem(
	const TArray<UINV_InventoryItem*>& ExistingItems,
	const FIntPoint& GridSize,
	const UINV_InventoryItem* CandidateItem,
	const UINV_InventoryItem* IgnoredItem)
{
	if (!IsValid(CandidateItem))
	{
		return false;
	}

	TArray<bool> Occupancy;
	if (!TryPlaceExistingItems(ExistingItems, GridSize, Occupancy, IgnoredItem))
	{
		return false;
	}

	return TryPlaceItemOnOccupancy(Occupancy, GridSize, CandidateItem);
}

UINV_InventoryItem* FINV_StorageTransferUtils::FindFirstMergeTarget(
	const TArray<UINV_InventoryItem*>& ExistingItems,
	const UINV_InventoryItem* CandidateItem)
{
	if (!IsValid(CandidateItem) || !CandidateItem->IsStackable())
	{
		return nullptr;
	}

	const FINV_StackableFragment* CandidateStackFragment = CandidateItem->GetCachedStackableFragment();
	if (!CandidateStackFragment)
	{
		return nullptr;
	}

	const int32 MaxStackSize = CandidateStackFragment->GetMaxStackSize();
	for (UINV_InventoryItem* ExistingItem : ExistingItems)
	{
		if (!IsValid(ExistingItem))
		{
			continue;
		}

		if (!UINV_InventoryItem::AreItemsStackCompatible(CandidateItem, ExistingItem))
		{
			continue;
		}

		if (ExistingItem->GetTotalStackCount() >= MaxStackSize)
		{
			continue;
		}

		return ExistingItem;
	}

	return nullptr;
}

UINV_InventoryItem* FINV_StorageTransferUtils::FindSwapCandidate(
	const TArray<UINV_InventoryItem*>& SourceItems,
	const FIntPoint& SourceGridSize,
	const TArray<UINV_InventoryItem*>& DestinationItems,
	const FIntPoint& DestinationGridSize,
	const UINV_InventoryItem* MovingItem)
{
	if (!IsValid(MovingItem))
	{
		return nullptr;
	}

	for (UINV_InventoryItem* DestinationItem : DestinationItems)
	{
		if (!IsValid(DestinationItem) || DestinationItem == MovingItem)
		{
			continue;
		}

		if (!CanInventoryFitItem(DestinationItems, DestinationGridSize, MovingItem, DestinationItem))
		{
			continue;
		}

		if (!CanInventoryFitItem(SourceItems, SourceGridSize, DestinationItem, MovingItem))
		{
			continue;
		}

		return DestinationItem;
	}

	return nullptr;
}

bool FINV_StorageTransferUtils::TryPlaceExistingItems(
	const TArray<UINV_InventoryItem*>& ExistingItems,
	const FIntPoint& GridSize,
	TArray<bool>& OutOccupancy,
	const UINV_InventoryItem* IgnoredItem)
{
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return false;
	}

	OutOccupancy.Init(false, GridSize.X * GridSize.Y);
	for (UINV_InventoryItem* ExistingItem : ExistingItems)
	{
		if (!IsValid(ExistingItem) || ExistingItem == IgnoredItem)
		{
			continue;
		}

		if (!TryPlaceItemOnOccupancy(OutOccupancy, GridSize, ExistingItem))
		{
			return false;
		}
	}

	return true;
}

bool FINV_StorageTransferUtils::TryPlaceItemOnOccupancy(
	TArray<bool>& Occupancy,
	const FIntPoint& GridSize,
	const UINV_InventoryItem* Item)
{
	if (!IsValid(Item))
	{
		return false;
	}

	const FINV_GridFragment* GridFragment = Item->GetCachedGridFragment();
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	for (int32 CandidateIndex = 0; CandidateIndex < Occupancy.Num(); ++CandidateIndex)
	{
		if (!CanFitAtIndex(Occupancy, GridSize, CandidateIndex, Dimensions))
		{
			continue;
		}

		MarkFootprint(Occupancy, GridSize, CandidateIndex, Dimensions, true);
		return true;
	}

	return false;
}

bool FINV_StorageTransferUtils::CanFitAtIndex(
	const TArray<bool>& Occupancy,
	const FIntPoint& GridSize,
	const int32 StartIndex,
	const FIntPoint& Dimensions)
{
	if (StartIndex < 0 || StartIndex >= Occupancy.Num())
	{
		return false;
	}

	const int32 StartX = StartIndex % GridSize.X;
	const int32 StartY = StartIndex / GridSize.X;
	if (StartX + Dimensions.X > GridSize.X || StartY + Dimensions.Y > GridSize.Y)
	{
		return false;
	}

	for (int32 Y = 0; Y < Dimensions.Y; ++Y)
	{
		for (int32 X = 0; X < Dimensions.X; ++X)
		{
			const int32 TileIndex = (StartY + Y) * GridSize.X + (StartX + X);
			if (Occupancy[TileIndex])
			{
				return false;
			}
		}
	}

	return true;
}

void FINV_StorageTransferUtils::MarkFootprint(
	TArray<bool>& Occupancy,
	const FIntPoint& GridSize,
	const int32 StartIndex,
	const FIntPoint& Dimensions,
	const bool bOccupied)
{
	const int32 StartX = StartIndex % GridSize.X;
	const int32 StartY = StartIndex / GridSize.X;
	for (int32 Y = 0; Y < Dimensions.Y; ++Y)
	{
		for (int32 X = 0; X < Dimensions.X; ++X)
		{
			const int32 TileIndex = (StartY + Y) * GridSize.X + (StartX + X);
			Occupancy[TileIndex] = bOccupied;
		}
	}
}
