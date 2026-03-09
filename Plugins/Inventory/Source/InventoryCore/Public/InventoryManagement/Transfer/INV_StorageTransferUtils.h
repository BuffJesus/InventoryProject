// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UINV_InventoryItem;

struct INVENTORYCORE_API FINV_StorageTransferUtils
{
	static bool CanInventoryFitItem(
		const TArray<UINV_InventoryItem*>& ExistingItems,
		const FIntPoint& GridSize,
		const UINV_InventoryItem* CandidateItem,
		const UINV_InventoryItem* IgnoredItem = nullptr);

	static UINV_InventoryItem* FindFirstMergeTarget(
		const TArray<UINV_InventoryItem*>& ExistingItems,
		const UINV_InventoryItem* CandidateItem);

	static UINV_InventoryItem* FindSwapCandidate(
		const TArray<UINV_InventoryItem*>& SourceItems,
		const FIntPoint& SourceGridSize,
		const TArray<UINV_InventoryItem*>& DestinationItems,
		const FIntPoint& DestinationGridSize,
		const UINV_InventoryItem* MovingItem);

private:
	static bool TryPlaceExistingItems(
		const TArray<UINV_InventoryItem*>& ExistingItems,
		const FIntPoint& GridSize,
		TArray<bool>& OutOccupancy,
		const UINV_InventoryItem* IgnoredItem = nullptr);

	static bool TryPlaceItemOnOccupancy(
		TArray<bool>& Occupancy,
		const FIntPoint& GridSize,
		const UINV_InventoryItem* Item);

	static bool CanFitAtIndex(
		const TArray<bool>& Occupancy,
		const FIntPoint& GridSize,
		int32 StartIndex,
		const FIntPoint& Dimensions);

	static void MarkFootprint(
		TArray<bool>& Occupancy,
		const FIntPoint& GridSize,
		int32 StartIndex,
		const FIntPoint& Dimensions,
		bool bOccupied);
};
