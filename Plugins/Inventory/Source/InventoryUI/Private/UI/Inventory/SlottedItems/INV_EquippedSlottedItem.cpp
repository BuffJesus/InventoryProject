// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"

FReply UINV_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnEquippedSlottedItemClicked.Broadcast(this, MouseEvent);
	return FReply::Handled();
}
