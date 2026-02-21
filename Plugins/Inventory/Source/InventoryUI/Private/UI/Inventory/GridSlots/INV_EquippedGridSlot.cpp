// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"
#include "Components/Image.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Utils/INV_InventoryStatics.h"

bool UINV_EquippedGridSlot::CanUpdateHoverTexture() const
{
	if (!GetAvailability()) return false;
	UINV_HoverItem* HoverItem { UINV_InventoryStatics::GetHoverItem(GetOwningPlayer()) };
	if (!IsValid(HoverItem)) return false;
	return HoverItem->GetItemType().MatchesTag(EquipmentTypeTag);
}

void UINV_EquippedGridSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!CanUpdateHoverTexture()) return;
	SetOccupiedTexture();
	if (IsValid(Image_GrayedOutIcon))
	{
		Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UINV_EquippedGridSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (!CanUpdateHoverTexture()) return;
	SetUnoccupiedTexture();
	if (IsValid(Image_GrayedOutIcon))
	{
		Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

FReply UINV_EquippedGridSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	EquippedGridSlotClicked.Broadcast(this, EquipmentTypeTag);
	return FReply::Handled();
}

UINV_EquippedSlottedItem* UINV_EquippedGridSlot::OnItemEquipped(UINV_InventoryItem* Item,
	const FGameplayTag& EquipmentTag, float TileSize)
{
	// Check the equipment type tag
	if (!EquipmentTypeTag.MatchesTagExact(EquipmentTypeTag)) return nullptr;
	
	// Get the grid dimensions
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(Item, FragmentTags::GridFragment) };
	if (!GridFragment) return nullptr;
	const FIntPoint GridDimensions = GridFragment->GetGridSize();
	
	// Calculate the draw size for the equipped slotted item
	const float IconTileWidth = TileSize * GridFragment->GetGridPadding() * 2;
	const FVector2D DrawSize = GridDimensions * IconTileWidth;
	
	// Create the equipped slotted item widget
	EquippedSlottedItem = CreateWidget<UINV_EquippedSlottedItem>(GetOwningPlayer(), EquippedSlottedItemClass);
	
	// Set the slotted item's inventory item
	EquippedSlottedItem->SetInventoryItem(Item);
	
	// Set the slotted item's equipment type tag
	EquippedSlottedItem->SetEquipmentTypeTag(EquipmentTag);
	
	// Hide the stack count widget on the slotted item
	EquippedSlottedItem->UpdateStackCount(0);
	
	// Set the inventory item on this class (the equipped grid slot)
	SetInventoryItem(Item);
	
	// Set the image brush on the equipped slotted item
	// Add the slotted item as the child to this widget's overlay
	// Return the equipped slotted item widget
	return nullptr;
}
