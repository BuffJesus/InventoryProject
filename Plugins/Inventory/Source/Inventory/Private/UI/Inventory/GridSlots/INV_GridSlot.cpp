// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Components/Image.h"

void UINV_GridSlot::NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseEnter(MyGeometry, MouseEvent);
	
	// Forward hover event with index.
	GridSlotHovered.Broadcast(TileIndex, MouseEvent);
}

void UINV_GridSlot::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
	
	// Forward unhover event with index.
	GridSlotUnhovered.Broadcast(TileIndex, MouseEvent);
}

FReply UINV_GridSlot::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Forward click event with index.
	GridSlotClicked.Broadcast(TileIndex, MouseEvent);
	return FReply::Handled();
}

void UINV_GridSlot::SetItemPopUp(UINV_ItemPopUp* PopUp)
{
	ItemPopUp = PopUp;
	if (IsValid(ItemPopUp.Get()))
	{
		ItemPopUp->SetGridIndex(GetTileIndex());
		ItemPopUp->OnNativeDestruct.AddUObject(this, &ThisClass::OnItemPopUpDestruct);
	}
}

void UINV_GridSlot::SetUnoccupiedTexture()
{
	// Visual state for empty slot.
	GridSlotState = EINV_GridSlotState::Unoccupied;
	Image_GridSlot->SetBrush(Brush_Unoccupied);
}

void UINV_GridSlot::SetOccupiedTexture()
{
	// Visual state for occupied slot.
	GridSlotState = EINV_GridSlotState::Occupied;
	Image_GridSlot->SetBrush(Brush_Occupied);
}

void UINV_GridSlot::SetSelectedTexture()
{
	// Visual state for selected slot.
	GridSlotState = EINV_GridSlotState::Selected;
	Image_GridSlot->SetBrush(Brush_Selected);
}

void UINV_GridSlot::SetGrayedOutTexture()
{
	// Visual state for blocked slot.
	GridSlotState = EINV_GridSlotState::GrayedOut;
	Image_GridSlot->SetBrush(Brush_GrayedOut);
}

void UINV_GridSlot::OnItemPopUpDestruct(UUserWidget* Menu)
{
	ItemPopUp.Reset();
}
