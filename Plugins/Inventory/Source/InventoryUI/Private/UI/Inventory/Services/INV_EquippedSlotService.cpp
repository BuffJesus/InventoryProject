#include "UI/Inventory/Services/INV_EquippedSlotService.h"

#include "InputCoreTypes.h"
#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"

EINV_EquippedClickAction FINV_EquippedSlotService::ResolveClickAction(
	const FPointerEvent& MouseEvent,
	const bool bHasValidSlottedItem,
	const bool bHasStackableHover)
{
	if (!bHasValidSlottedItem)
	{
		return EINV_EquippedClickAction::None;
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return EINV_EquippedClickAction::Inspect;
	}

	if (bHasStackableHover)
	{
		return EINV_EquippedClickAction::None;
	}

	return EINV_EquippedClickAction::Swap;
}

void FINV_EquippedSlotService::ClearSlot(UINV_EquippedGridSlot* EquippedGridSlot)
{
	if (!IsValid(EquippedGridSlot)) return;
	EquippedGridSlot->SetEquippedSlottedItem(nullptr);
	EquippedGridSlot->SetInventoryItem(nullptr);
	EquippedGridSlot->SetSlotEmptyVisual();
}

void FINV_EquippedSlotService::RemoveSlottedItem(
	UINV_EquippedSlottedItem* EquippedSlottedItem,
	const FINV_EquippedSlotCallbacks& Callbacks)
{
	if (!IsValid(EquippedSlottedItem)) return;

	EquippedSlottedItem->SetHighlightEnabled(false);
	if (Callbacks.UnbindEquippedSlottedItemDelegates)
	{
		Callbacks.UnbindEquippedSlottedItemDelegates(EquippedSlottedItem);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void FINV_EquippedSlotService::EquipItemInSlot(
	UINV_EquippedGridSlot* EquippedGridSlot,
	UINV_InventoryItem* ItemToEquip,
	const FGameplayTag& EquipmentTypeTag,
	const float TileSize,
	const FINV_EquippedSlotCallbacks& Callbacks)
{
	if (!IsValid(EquippedGridSlot)) return;
	if (!IsValid(ItemToEquip) || TileSize <= 0.f)
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
		return;
	}

	UINV_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(ItemToEquip, EquipmentTypeTag, TileSize);
	if (!IsValid(SlottedItem)) return;

	if (Callbacks.BindEquippedSlottedItemDelegates)
	{
		Callbacks.BindEquippedSlottedItemDelegates(SlottedItem);
	}
	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}
