// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Stack/INV_GridStackOperations.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

FINV_StackDetails FINV_GridStackOperations::CalculateStackDetails(
	const UINV_GridSlot* GridSlot,
	const UINV_HoverItem* HoverItem,
	const UINV_InventoryItem* ClickedItem)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridStackOperations_CalculateStackDetails);

	if (!IsValid(GridSlot) || !IsValid(HoverItem) || !IsValid(ClickedItem))
	{
		return FINV_StackDetails {};
	}

	const int32 ClickedStackCount = GridSlot->GetStackCount();
	const FINV_StackableFragment* StackableFragment = ClickedItem->GetCachedStackableFragment();
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 0;
	const int32 RoomInClickedSlot = MaxStackSize - ClickedStackCount;
	const int32 HoveredStackCount = HoverItem->GetStackCount();

	return FINV_StackDetails { ClickedStackCount, RoomInClickedSlot, HoveredStackCount, MaxStackSize };
}

bool FINV_GridStackOperations::IsSameStackable(
	const UINV_HoverItem* HoverItem,
	const UINV_InventoryItem* ClickedItem)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridStackOperations_IsSameStackable);

	if (!IsValid(HoverItem) || !IsValid(ClickedItem))
	{
		return false;
	}

	const bool bIsSameItem = ClickedItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = HoverItem->IsStackable();
	return bIsSameItem && bIsStackable;
}

void FINV_GridStackOperations::SwapStackCounts(
	UINV_GridSlot* GridSlot,
	UINV_SlottedItem* SlottedItem,
	UINV_HoverItem* HoverItem,
	int32 ClickedStackCount,
	int32 HoveredStackCount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridStackOperations_SwapStackCounts);

	if (!IsValid(GridSlot) || !IsValid(SlottedItem) || !IsValid(HoverItem))
	{
		return;
	}

	GridSlot->SetStackCount(HoveredStackCount);
	SlottedItem->UpdateStackCount(HoveredStackCount);
	HoverItem->UpdateStackCount(ClickedStackCount);
}

void FINV_GridStackOperations::ConsumeHoverItemStacks(
	UINV_GridSlot* GridSlot,
	UINV_SlottedItem* SlottedItem,
	int32 ClickedStackCount,
	int32 HoveredStackCount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridStackOperations_ConsumeHoverItemStacks);

	if (!IsValid(GridSlot) || !IsValid(SlottedItem))
	{
		return;
	}

	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;

	GridSlot->SetStackCount(NewClickedStackCount);
	SlottedItem->UpdateStackCount(NewClickedStackCount);
}

void FINV_GridStackOperations::FillInStack(
	UINV_GridSlot* GridSlot,
	UINV_SlottedItem* SlottedItem,
	UINV_HoverItem* HoverItem,
	int32 FillAmount,
	int32 Remainder)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridStackOperations_FillInStack);

	if (!IsValid(GridSlot) || !IsValid(SlottedItem) || !IsValid(HoverItem))
	{
		return;
	}

	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;
	GridSlot->SetStackCount(NewStackCount);
	SlottedItem->UpdateStackCount(NewStackCount);
	HoverItem->UpdateStackCount(Remainder);
}
