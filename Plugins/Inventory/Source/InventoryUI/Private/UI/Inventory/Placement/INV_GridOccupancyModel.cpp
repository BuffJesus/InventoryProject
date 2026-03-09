// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Placement/INV_GridOccupancyModel.h"

#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"

void FINV_GridOccupancyModel::Initialize(const FIntPoint& InGridSize)
{
	GridSize = InGridSize;
	AnchorByTile.Init(INDEX_NONE, FMath::Max(0, GridSize.X * GridSize.Y));
	ItemByAnchor.Reset();
	AnchorByItem.Reset();
	DimensionsByAnchor.Reset();
	StackCountByAnchor.Reset();
}

void FINV_GridOccupancyModel::Reset()
{
	Initialize(GridSize);
}

void FINV_GridOccupancyModel::RebuildFromGridSlots(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots, const FIntPoint& InGridSize)
{
	Initialize(InGridSize);

	for (const TObjectPtr<UINV_GridSlot>& GridSlot : GridSlots)
	{
		if (!IsValid(GridSlot))
		{
			continue;
		}

		UINV_InventoryItem* Item = GridSlot->GetInventoryItem().Get();
		const int32 AnchorIndex = GridSlot->GetUpperLeftIndex();
		if (!IsValid(Item) || AnchorIndex == INDEX_NONE || GridSlot->GetTileIndex() != AnchorIndex)
		{
			continue;
		}

		const FINV_GridFragment* GridFragment = Item->GetCachedGridFragment();
		const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
		SetItemAtAnchor(Item, AnchorIndex, Dimensions, GridSlot->GetStackCount());
	}
}

void FINV_GridOccupancyModel::SetItemAtAnchor(UINV_InventoryItem* Item, const int32 AnchorIndex, const FIntPoint& Dimensions, const int32 StackCount)
{
	if (!IsValid(Item) || !IsTileIndexValid(AnchorIndex))
	{
		return;
	}

	ClearAnchor(AnchorIndex);

	ItemByAnchor.Add(AnchorIndex, Item);
	AnchorByItem.Add(Item, AnchorIndex);
	DimensionsByAnchor.Add(AnchorIndex, Dimensions);
	StackCountByAnchor.Add(AnchorIndex, StackCount);
	MarkFootprint(AnchorIndex, Dimensions, true);
}

void FINV_GridOccupancyModel::ClearAnchor(const int32 AnchorIndex)
{
	const FIntPoint* ExistingDimensions = DimensionsByAnchor.Find(AnchorIndex);
	const FIntPoint Dimensions = ExistingDimensions ? *ExistingDimensions : FIntPoint(1, 1);
	MarkFootprint(AnchorIndex, Dimensions, false);

	if (const TWeakObjectPtr<UINV_InventoryItem>* ExistingItem = ItemByAnchor.Find(AnchorIndex))
	{
		if (ExistingItem->IsValid())
		{
			AnchorByItem.Remove(ExistingItem->Get());
		}
	}

	ItemByAnchor.Remove(AnchorIndex);
	DimensionsByAnchor.Remove(AnchorIndex);
	StackCountByAnchor.Remove(AnchorIndex);
}

void FINV_GridOccupancyModel::UpdateStackCountForAnchor(const int32 AnchorIndex, const int32 StackCount)
{
	if (AnchorIndex == INDEX_NONE)
	{
		return;
	}

	StackCountByAnchor.Add(AnchorIndex, StackCount);
}

int32 FINV_GridOccupancyModel::FindAnchorForItem(const UINV_InventoryItem* Item) const
{
	if (!IsValid(Item))
	{
		return INDEX_NONE;
	}

	if (const int32* FoundAnchor = AnchorByItem.Find(Item))
	{
		return *FoundAnchor;
	}

	return INDEX_NONE;
}

UINV_InventoryItem* FINV_GridOccupancyModel::GetItemAtAnchor(const int32 AnchorIndex) const
{
	const TWeakObjectPtr<UINV_InventoryItem>* FoundItem = ItemByAnchor.Find(AnchorIndex);
	return FoundItem ? FoundItem->Get() : nullptr;
}

UINV_InventoryItem* FINV_GridOccupancyModel::GetItemAtTile(const int32 TileIndex) const
{
	return GetItemAtAnchor(GetAnchorAtTile(TileIndex));
}

int32 FINV_GridOccupancyModel::GetAnchorAtTile(const int32 TileIndex) const
{
	if (!AnchorByTile.IsValidIndex(TileIndex))
	{
		return INDEX_NONE;
	}

	return AnchorByTile[TileIndex];
}

int32 FINV_GridOccupancyModel::GetStackCountForAnchor(const int32 AnchorIndex) const
{
	if (const int32* FoundStackCount = StackCountByAnchor.Find(AnchorIndex))
	{
		return *FoundStackCount;
	}

	return 0;
}

int32 FINV_GridOccupancyModel::GetStackCountAtTile(const int32 TileIndex) const
{
	return GetStackCountForAnchor(GetAnchorAtTile(TileIndex));
}

const FIntPoint* FINV_GridOccupancyModel::GetDimensionsForAnchor(const int32 AnchorIndex) const
{
	return DimensionsByAnchor.Find(AnchorIndex);
}

void FINV_GridOccupancyModel::GetSortedAnchors(TArray<int32>& OutAnchors) const
{
	ItemByAnchor.GetKeys(OutAnchors);
	OutAnchors.Sort();
}

bool FINV_GridOccupancyModel::IsTileOccupied(const int32 TileIndex) const
{
	return GetAnchorAtTile(TileIndex) != INDEX_NONE;
}

bool FINV_GridOccupancyModel::IsTileIndexValid(const int32 TileIndex) const
{
	return AnchorByTile.IsValidIndex(TileIndex);
}

void FINV_GridOccupancyModel::MarkFootprint(const int32 AnchorIndex, const FIntPoint& Dimensions, const bool bOccupied)
{
	if (!IsTileIndexValid(AnchorIndex))
	{
		return;
	}

	const int32 StartX = AnchorIndex % GridSize.X;
	const int32 StartY = AnchorIndex / GridSize.X;
	for (int32 Y = 0; Y < Dimensions.Y; ++Y)
	{
		for (int32 X = 0; X < Dimensions.X; ++X)
		{
			const int32 TileX = StartX + X;
			const int32 TileY = StartY + Y;
			if (TileX < 0 || TileY < 0 || TileX >= GridSize.X || TileY >= GridSize.Y)
			{
				continue;
			}

			const int32 TileIndex = TileY * GridSize.X + TileX;
			AnchorByTile[TileIndex] = bOccupied ? AnchorIndex : INDEX_NONE;
		}
	}
}
