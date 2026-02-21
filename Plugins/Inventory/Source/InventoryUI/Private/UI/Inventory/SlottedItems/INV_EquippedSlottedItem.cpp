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
	SetHighlightEnabled(true);
	OnEquippedSlottedItemHovered.Broadcast(this);
}

void UINV_EquippedSlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	SetHighlightEnabled(false);
	OnEquippedSlottedItemUnhovered.Broadcast(this);
}

void UINV_EquippedSlottedItem::SetHighlightEnabled(bool bEnabled) const
{
	// Primary effect: widget-level scale/opacity so highlight works even when Image_Icon is not bound in BP.
	const_cast<UINV_EquippedSlottedItem*>(this)->SetRenderScale(bEnabled ? FVector2D(1.06f, 1.06f) : FVector2D(1.0f, 1.0f));
	const_cast<UINV_EquippedSlottedItem*>(this)->SetRenderOpacity(bEnabled ? 1.0f : 0.95f);

	// Secondary effect: icon tint when the inherited image is available.
	UImage* IconImage = GetImageIcon();
	if (IsValid(IconImage))
	{
		IconImage->SetColorAndOpacity(bEnabled ? HighlightTint : FLinearColor::White);
	}
}
