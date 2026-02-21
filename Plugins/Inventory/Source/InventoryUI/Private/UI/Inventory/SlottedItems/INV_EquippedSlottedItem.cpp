// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"
#include "Components/Image.h"

FReply UINV_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnEquippedSlottedItemClicked.Broadcast(this, MouseEvent);
	return FReply::Handled();
}

void UINV_EquippedSlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	OnEquippedSlottedItemHovered.Broadcast(this);
}

void UINV_EquippedSlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	OnEquippedSlottedItemUnhovered.Broadcast(this);
}

void UINV_EquippedSlottedItem::SetHighlightEnabled(bool bEnabled) const
{
	UImage* IconImage = GetImageIcon();
	if (!IsValid(IconImage)) return;
	IconImage->SetColorAndOpacity(bEnabled ? HighlightTint : FLinearColor::White);
}
