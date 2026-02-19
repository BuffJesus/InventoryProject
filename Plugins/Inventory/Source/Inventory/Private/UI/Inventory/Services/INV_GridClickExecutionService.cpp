#include "UI/Inventory/Services/INV_GridClickExecutionService.h"

void FINV_GridClickExecutionService::ExecuteSlottedItemClick(
	const FINV_GridClickResult& ActionResult,
	const FINV_StackDetails& StackDetails,
	UINV_InventoryItem* ClickedInventoryItem,
	const int32 GridIndex,
	const FINV_GridClickExecutionCallbacks& Callbacks)
{
	switch (ActionResult.Action)
	{
	case EINV_ClickAction::Pickup:
		if (Callbacks.Pickup)
		{
			Callbacks.Pickup(ClickedInventoryItem, GridIndex);
		}
		break;

	case EINV_ClickAction::CreatePopup:
		if (Callbacks.CreatePopup)
		{
			Callbacks.CreatePopup(GridIndex);
		}
		break;

	case EINV_ClickAction::SwapStackCounts:
		if (Callbacks.SwapStackCounts)
		{
			Callbacks.SwapStackCounts(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
		}
		break;

	case EINV_ClickAction::ConsumeHoverStacks:
		if (Callbacks.ConsumeHoverStacks)
		{
			Callbacks.ConsumeHoverStacks(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
		}
		break;

	case EINV_ClickAction::FillStack:
		if (Callbacks.FillStack)
		{
			Callbacks.FillStack(ActionResult.AuxiliaryValue, StackDetails.HoveredStackCount - ActionResult.AuxiliaryValue, GridIndex);
		}
		break;

	case EINV_ClickAction::SwapItems:
		if (Callbacks.SwapItems)
		{
			Callbacks.SwapItems(ClickedInventoryItem, GridIndex);
		}
		break;

	case EINV_ClickAction::PlaceItem:
	case EINV_ClickAction::None:
	default:
		break;
	}
}

void FINV_GridClickExecutionService::ExecuteGridSlotClick(
	const FINV_GridClickResult& ActionResult,
	const FINV_GridClickExecutionCallbacks& Callbacks)
{
	switch (ActionResult.Action)
	{
	case EINV_ClickAction::SwapItems:
		if (Callbacks.RouteToSlottedClick)
		{
			Callbacks.RouteToSlottedClick(ActionResult.TargetIndex);
		}
		break;

	case EINV_ClickAction::PlaceItem:
		if (Callbacks.PlaceItem)
		{
			Callbacks.PlaceItem(ActionResult.TargetIndex);
		}
		break;

	case EINV_ClickAction::Pickup:
	case EINV_ClickAction::CreatePopup:
	case EINV_ClickAction::SwapStackCounts:
	case EINV_ClickAction::ConsumeHoverStacks:
	case EINV_ClickAction::FillStack:
	case EINV_ClickAction::None:
	default:
		break;
	}
}
