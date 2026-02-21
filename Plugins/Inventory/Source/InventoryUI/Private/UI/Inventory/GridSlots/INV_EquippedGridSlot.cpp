// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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

bool UINV_EquippedGridSlot::SetEquippedItemImageBrush(UINV_InventoryItem* Item, const FVector2D DrawSize, UINV_EquippedSlottedItem*& Value1)
{
	const FINV_ImageFragment* ImageFragment { GetFragment<FINV_ImageFragment>(Item, FragmentTags::IconFragment) };
	if (!ImageFragment)
	{
		Value1 = nullptr;
		return true;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = DrawSize;
	
	EquippedSlottedItem->SetImageBrush(Brush);
	return false;
}

void UINV_EquippedGridSlot::AddEquippedItemToOverlay(const FVector2D DrawSize)
{
	Overlay_Root->AddChildToOverlay(EquippedSlottedItem);
	FGeometry OverlayGeometry = Overlay_Root->GetCachedGeometry();
	auto OverlayPos = OverlayGeometry.Position;
	auto OverlaySize = OverlayGeometry.Size;
	
	const float LeftPadding = OverlaySize.X / 2.f - DrawSize.X / 2.f;
	const float TopPadding = OverlaySize.Y / 2.f - DrawSize.Y / 2.f;
	
	UOverlaySlot* OverlaySlot { UWidgetLayoutLibrary::SlotAsOverlaySlot(EquippedSlottedItem) };
	OverlaySlot->SetPadding(FMargin(LeftPadding, TopPadding));
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
	UINV_EquippedSlottedItem* Value1;
	if (SetEquippedItemImageBrush(Item, DrawSize, Value1)) return Value1;

	// Add the slotted item as the child to this widget's overlay
	AddEquippedItemToOverlay(DrawSize);
	
	// Return the equipped slotted item widget
	return EquippedSlottedItem;
}
