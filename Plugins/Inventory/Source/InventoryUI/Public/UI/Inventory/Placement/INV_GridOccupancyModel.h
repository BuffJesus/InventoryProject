// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UINV_GridSlot;
class UINV_InventoryItem;

struct INVENTORYUI_API FINV_GridOccupancyModel
{
public:
	void Initialize(const FIntPoint& InGridSize);
	void Reset();
	void RebuildFromGridSlots(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots, const FIntPoint& InGridSize);
	void SetItemAtAnchor(UINV_InventoryItem* Item, int32 AnchorIndex, const FIntPoint& Dimensions, int32 StackCount);
	void ClearAnchor(int32 AnchorIndex);
	void UpdateStackCountForAnchor(int32 AnchorIndex, int32 StackCount);

	int32 FindAnchorForItem(const UINV_InventoryItem* Item) const;
	UINV_InventoryItem* GetItemAtAnchor(int32 AnchorIndex) const;
	UINV_InventoryItem* GetItemAtTile(int32 TileIndex) const;
	int32 GetAnchorAtTile(int32 TileIndex) const;
	int32 GetStackCountForAnchor(int32 AnchorIndex) const;
	int32 GetStackCountAtTile(int32 TileIndex) const;
	const FIntPoint* GetDimensionsForAnchor(int32 AnchorIndex) const;
	void GetSortedAnchors(TArray<int32>& OutAnchors) const;
	bool IsTileOccupied(int32 TileIndex) const;
	const FIntPoint& GetGridSize() const { return GridSize; }

private:
	bool IsTileIndexValid(int32 TileIndex) const;
	void MarkFootprint(int32 AnchorIndex, const FIntPoint& Dimensions, bool bOccupied);

	FIntPoint GridSize { 0, 0 };
	TArray<int32> AnchorByTile;
	TMap<int32, TWeakObjectPtr<UINV_InventoryItem>> ItemByAnchor;
	TMap<const UINV_InventoryItem*, int32> AnchorByItem;
	TMap<int32, FIntPoint> DimensionsByAnchor;
	TMap<int32, int32> StackCountByAnchor;
};
