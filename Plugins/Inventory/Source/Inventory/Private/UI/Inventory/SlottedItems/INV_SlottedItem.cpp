// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "Components/TextBlock.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"

FReply UINV_SlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Forward click to grid logic.
	OnSlottedItemClicked.Broadcast(GridIndex, MouseEvent);
	return FReply::Handled();
}

void UINV_SlottedItem::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	OnSlottedItemHovered.Broadcast(GridIndex, MouseEvent);
	UINV_InventoryStatics::ItemHovered(GetOwningPlayer(), InventoryItem.Get());
}

void UINV_SlottedItem::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	OnSlottedItemUnhovered.Broadcast(GridIndex, MouseEvent);
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
}

void UINV_SlottedItem::UpdateStackCount(int32 StackCount)
{
	// Show or hide stack count text.
	if (StackCount > 0)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		Text_StackCount->SetText(FText::AsNumber(StackCount));
	}
	else
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}
