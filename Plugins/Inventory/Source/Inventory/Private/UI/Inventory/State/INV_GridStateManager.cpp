// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/State/INV_GridStateManager.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"

void UINV_GridStateManager::HighlightSlots(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const int32 GridWidth,
	const int32 Index,
	const FIntPoint& Dimensions,
	const bool bMouseWithinCanvas)
{
	if (!bMouseWithinCanvas) return;

	FINV_GridIteration::ForEach2D(GridSlots, Index, Dimensions, GridWidth,
		[](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
}

void UINV_GridStateManager::UnHighlightSlots(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const int32 GridWidth,
	const int32 Index,
	const FIntPoint& Dimensions)
{
	FINV_GridIteration::ForEach2D(GridSlots, Index, Dimensions, GridWidth,
		[](UINV_GridSlot* GridSlot)
	{
		ApplyStateBasedOnAvailability(GridSlot);
	});
}

void UINV_GridStateManager::ChangeSlotState(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const int32 GridWidth,
	const int32 Index,
	const FIntPoint& Dimensions,
	const EINV_GridSlotState State)
{
	FINV_GridIteration::ForEach2D(GridSlots, Index, Dimensions, GridWidth,
		[State](UINV_GridSlot* GridSlot)
	{
		switch (State)
		{
			case EINV_GridSlotState::Occupied:
				GridSlot->SetOccupiedTexture();
				break;
			case EINV_GridSlotState::Unoccupied:
				GridSlot->SetUnoccupiedTexture();
				break;
			case EINV_GridSlotState::GrayedOut:
				GridSlot->SetGrayedOutTexture();
				break;
			case EINV_GridSlotState::Selected:
				GridSlot->SetSelectedTexture();
				break;
		}
	});
}

TArray<int32> UINV_GridStateManager::HighlightBlockingItems(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const int32 GridWidth,
	const TArray<int32>& BlockingUpperLeftIndices,
	TFunction<const FINV_GridFragment*(int32)> GetItemFragment)
{
	TArray<int32> GrayedOutIndices;

	for (const int32 UpperLeftIndex : BlockingUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(UpperLeftIndex)) continue;

		const FINV_GridFragment* GridFragment = GetItemFragment(UpperLeftIndex);
		if (!GridFragment) continue;

		GrayedOutIndices.Add(UpperLeftIndex);

		FINV_GridIteration::ForEach2D(GridSlots, UpperLeftIndex, GridFragment->GetGridSize(), GridWidth,
			[](UINV_GridSlot* GridSlot)
		{
			GridSlot->SetGrayedOutTexture();
		});
	}

	return GrayedOutIndices;
}

void UINV_GridStateManager::UnHighlightBlockingItems(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const int32 GridWidth,
	const TArray<int32>& GrayedOutIndices,
	TFunction<const FINV_GridFragment*(int32)> GetItemFragment)
{
	for (const int32 UpperLeftIndex : GrayedOutIndices)
	{
		if (!GridSlots.IsValidIndex(UpperLeftIndex)) continue;

		const FINV_GridFragment* GridFragment = GetItemFragment(UpperLeftIndex);
		if (!GridFragment) continue;

		FINV_GridIteration::ForEach2D(GridSlots, UpperLeftIndex, GridFragment->GetGridSize(), GridWidth,
			[](UINV_GridSlot* GridSlot)
		{
			ApplyStateBasedOnAvailability(GridSlot);
		});
	}
}

void UINV_GridStateManager::RefreshAllSlotVisuals(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots)
{
	for (UINV_GridSlot* GridSlot : GridSlots)
	{
		if (!IsValid(GridSlot)) continue;
		ApplyStateBasedOnAvailability(GridSlot);
	}
}

void UINV_GridStateManager::ApplyStateBasedOnAvailability(UINV_GridSlot* GridSlot)
{
	if (!IsValid(GridSlot)) return;

	if (GridSlot->GetAvailability())
	{
		GridSlot->SetUnoccupiedTexture();
	}
	else
	{
		GridSlot->SetOccupiedTexture();
	}
}
