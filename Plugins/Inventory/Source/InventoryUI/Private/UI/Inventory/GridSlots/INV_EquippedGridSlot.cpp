// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"

#include "Components/Image.h"
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
