#pragma once

#include "CoreMinimal.h"

class UINV_EquippedGridSlot;
class UINV_EquippedSlottedItem;
class UINV_InventoryItem;
struct FPointerEvent;
struct FGameplayTag;

enum class EINV_EquippedClickAction : uint8
{
	None,
	Inspect,
	Swap
};

struct FINV_EquippedSlotCallbacks
{
	TFunction<void(UINV_EquippedSlottedItem*)> BindEquippedSlottedItemDelegates;
	TFunction<void(UINV_EquippedSlottedItem*)> UnbindEquippedSlottedItemDelegates;
};

class INVENTORYUI_API FINV_EquippedSlotService
{
public:
	static EINV_EquippedClickAction ResolveClickAction(
		const FPointerEvent& MouseEvent,
		bool bHasValidSlottedItem,
		bool bHasStackableHover);

	static void ClearSlot(UINV_EquippedGridSlot* EquippedGridSlot);

	static void RemoveSlottedItem(
		UINV_EquippedSlottedItem* EquippedSlottedItem,
		const FINV_EquippedSlotCallbacks& Callbacks);

	static void EquipItemInSlot(
		UINV_EquippedGridSlot* EquippedGridSlot,
		UINV_InventoryItem* ItemToEquip,
		const FGameplayTag& EquipmentTypeTag,
		float TileSize,
		const FINV_EquippedSlotCallbacks& Callbacks);
};
