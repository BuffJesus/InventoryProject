// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Popup/INV_GridPopupManager.h"
#include "UI/Popup/INV_ItemPopUp.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/Utils/INV_WidgetUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

UINV_ItemPopUp* FINV_GridPopupManager::CreateItemPopup(
	TObjectPtr<UINV_ItemPopUp>& ItemPopUp,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	int32 GridIndex,
	TSubclassOf<UINV_ItemPopUp> ItemPopUpClass,
	UCanvasPanel* OwningCanvasPanel,
	UUserWidget* OwningWidget,
	APlayerController* PlayerController)
{
	if (!GridSlots.IsValidIndex(GridIndex)) return nullptr;

	UINV_InventoryItem* RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return nullptr;
	if (!ItemPopUpClass) return nullptr;
	if (!IsValid(OwningCanvasPanel)) return nullptr;
	if (!IsValid(OwningWidget)) return nullptr;

	// Reuse existing popup when possible to avoid repeated widget creation.
	if (!IsValid(ItemPopUp))
	{
		ItemPopUp = CreateWidget<UINV_ItemPopUp>(OwningWidget, ItemPopUpClass);
	}
	if (!IsValid(ItemPopUp)) return nullptr;

	if (ItemPopUp->GetParent() != OwningCanvasPanel)
	{
		ItemPopUp->RemoveFromParent();
		OwningCanvasPanel->AddChild(ItemPopUp);
	}

	ItemPopUp->SetGridIndex(GridIndex);
	ItemPopUp->SetVisibility(ESlateVisibility::Visible);
	ItemPopUp->ResetMenuState();

	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp);
	if (!IsValid(CanvasSlot)) return nullptr;

	FVector2D PopUpSize = ItemPopUp->GetBoxSize();
	if (PopUpSize.IsNearlyZero())
	{
		PopUpSize = ItemPopUp->GetDesiredSize();
	}
	if (PopUpSize.IsNearlyZero())
	{
		PopUpSize = FVector2D(220.f, 140.f);
	}
	CanvasSlot->SetSize(PopUpSize);

	const UCanvasPanelSlot* GridSlotCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlots[GridIndex]);
	FVector2D AnchorPosition = FVector2D::ZeroVector;
	const FGeometry CanvasGeometry = OwningCanvasPanel->GetCachedGeometry();
	if (!CanvasGeometry.GetLocalSize().IsNearlyZero())
	{
		const FVector2D CursorAbsPosition = FSlateApplication::Get().GetCursorPos();
		AnchorPosition = CanvasGeometry.AbsoluteToLocal(CursorAbsPosition);
	}
	else
	{
		const FVector2D MouseViewportPos = UWidgetLayoutLibrary::GetMousePositionOnViewport(PlayerController);
		const FVector2D CanvasViewportPos = UINV_WidgetUtils::GetWidgetPosition(OwningCanvasPanel);
		AnchorPosition = MouseViewportPos - CanvasViewportPos;
	}

	// Fallback to slot center if mouse position is unavailable.
	if ((!FMath::IsFinite(AnchorPosition.X) || !FMath::IsFinite(AnchorPosition.Y)) && IsValid(GridSlotCanvasSlot))
	{
		AnchorPosition = GridSlotCanvasSlot->GetPosition() + (GridSlotCanvasSlot->GetSize() * 0.5f);
	}

	FVector2D GridMin(FLT_MAX, FLT_MAX);
	FVector2D GridMax(-FLT_MAX, -FLT_MAX);
	for (const TObjectPtr<UINV_GridSlot>& GridSlot : GridSlots)
	{
		const UCanvasPanelSlot* IterGridSlotCanvasSlot = IsValid(GridSlot)
			? UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot)
			: nullptr;
		if (!IsValid(IterGridSlotCanvasSlot)) continue;
		const FVector2D SlotPos = IterGridSlotCanvasSlot->GetPosition();
		const FVector2D SlotMax = SlotPos + IterGridSlotCanvasSlot->GetSize();
		GridMin.X = FMath::Min(GridMin.X, SlotPos.X);
		GridMin.Y = FMath::Min(GridMin.Y, SlotPos.Y);
		GridMax.X = FMath::Max(GridMax.X, SlotMax.X);
		GridMax.Y = FMath::Max(GridMax.Y, SlotMax.Y);
	}
	if (!FMath::IsFinite(GridMin.X) || !FMath::IsFinite(GridMin.Y) || !FMath::IsFinite(GridMax.X) || !FMath::IsFinite(GridMax.Y))
	{
		GridMin = FVector2D::ZeroVector;
		GridMax = UINV_WidgetUtils::GetWidgetSize(OwningCanvasPanel);
	}
	const FVector2D GridBoundsSize = FVector2D(
		FMath::Max(0.f, GridMax.X - GridMin.X),
		FMath::Max(0.f, GridMax.Y - GridMin.Y));

	const FVector2D ClampedPosition = GridMin + UINV_WidgetUtils::GetCenteredClampedWidgetPosition(
		GridBoundsSize,
		PopUpSize,
		AnchorPosition - GridMin);
	CanvasSlot->SetPosition(ClampedPosition);

	// Configure split button visibility
	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}

	// Configure consume button visibility
	if (!RightClickedItem->IsConsumable())
	{
		ItemPopUp->CollapseConsumeButton();
	}

	return ItemPopUp;
}

bool FINV_GridPopupManager::ClosePopupIfClickedOutside(
	TObjectPtr<UINV_ItemPopUp>& ItemPopUp,
	APlayerController* PlayerController)
{
	if (!IsValid(ItemPopUp)) return false;
	if (ItemPopUp->GetVisibility() != ESlateVisibility::Visible) return false;
	if (!IsValid(PlayerController)) return false;

	const bool bClickedThisFrame =
		PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton) ||
		PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton) ||
		PlayerController->WasInputKeyJustPressed(EKeys::MiddleMouseButton);

	if (!bClickedThisFrame) return false;

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	if (ItemPopUp->GetCachedGeometry().IsUnderLocation(CursorPosition)) return false;

	ItemPopUp->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void FINV_GridPopupManager::CloseActiveItemPopup(TObjectPtr<UINV_ItemPopUp>& ItemPopUp)
{
	if (!IsValid(ItemPopUp)) return;
	ItemPopUp->SetVisibility(ESlateVisibility::Collapsed);
}
