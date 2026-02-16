// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_SpatialInventory.h"

#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "UI/ItemDescription/INV_ItemDescription.h"
#include "Framework/Application/SlateApplication.h"

void UINV_SpatialInventory::ShowEquippableGrid()
{
	SetActiveGrid(Grid_Equippable, Button_Equippable);
}

void UINV_SpatialInventory::ShowConsumableGrid()
{
	SetActiveGrid(Grid_Consumable, Button_Consumable);
}

void UINV_SpatialInventory::ShowCraftableGrid()
{
	SetActiveGrid(Grid_Craftable, Button_Craftable);
}

void UINV_SpatialInventory::DisableButton(UButton* Button)
{
	Button_Equippable->SetIsEnabled(true);
	Button_Consumable->SetIsEnabled(true);
	Button_Craftable->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UINV_SpatialInventory::SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button)
{
	if (ActiveGrid.IsValid()) ActiveGrid->HideCursor();
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid()) ActiveGrid->ShowCursor();
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}

void UINV_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::ShowEquippableGrid);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::ShowConsumableGrid);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::ShowCraftableGrid);
	
	Grid_Equippable->SetOwningCanvas(CanvasPanel);
	Grid_Consumable->SetOwningCanvas(CanvasPanel);
	Grid_Craftable->SetOwningCanvas(CanvasPanel);
	
	ShowEquippableGrid();
}

FReply UINV_SpatialInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ActiveGrid->DropItem();
	return FReply::Handled();
}

FINV_SlotAvailabilityResult UINV_SpatialInventory::HasRoomForItem(UINV_ItemComponent* ItemComponent) const
{
	switch (UINV_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{
		case EINV_ItemCategory::Equippable: return Grid_Equippable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Consumable: return Grid_Consumable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Craftable: return Grid_Craftable->HasRoomForItem(ItemComponent);
		default:
		UE_LOG(LogInventory, Error, TEXT("Invalid item category for inventory slot availability check"));
		return FINV_SlotAvailabilityResult();
	}
}

void UINV_SpatialInventory::OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	OpenItemDescription(Item, OpenPosition);
}

bool UINV_SpatialInventory::HasHoverItem() const
{
	if (Grid_Equippable->HasHoverItem()) return true;
	if (Grid_Consumable->HasHoverItem()) return true;
	if (Grid_Craftable->HasHoverItem()) return true;
	return false;
}

void UINV_SpatialInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CloseDescriptionIfCursorExited();
}

void UINV_SpatialInventory::SetItemDescriptionSizeAndPosition(UINV_ItemDescription* Description,
	UCanvasPanel* Canvas, const FVector2D& OpenPosition) const
{
	UCanvasPanelSlot* ItemDescriptionCPS { UWidgetLayoutLibrary::SlotAsCanvasSlot(Description) };
	if (!IsValid(ItemDescriptionCPS)) return;

	// Ensure first-open layout is valid before querying desired size.
	Description->ForceLayoutPrepass();
	Canvas->ForceLayoutPrepass();

	FVector2D ItemDescriptionSize = Description->GetBoxSize();
	if (ItemDescriptionSize.IsNearlyZero())
	{
		ItemDescriptionSize = Description->GetDesiredSize();
		if (ItemDescriptionSize.IsNearlyZero())
		{
			// Last resort to keep first inspect visible if desired size is not ready yet.
			ItemDescriptionSize = FVector2D(200.f, 100.f);
		}
	}
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);

	// Canvas slot positions are canvas-local, so convert mouse viewport position to canvas-local space first.
	const FVector2D CanvasViewportPos = UINV_WidgetUtils::GetWidgetPosition(Canvas);
	const FVector2D LocalOpenPosition = OpenPosition - CanvasViewportPos;
	FVector2D ClampedPos { UINV_WidgetUtils::GetCenteredClampedWidgetPosition(
		UINV_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		LocalOpenPosition) };
	
	ItemDescriptionCPS->SetPosition(ClampedPos);
}

UINV_ItemDescription* UINV_SpatialInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UINV_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
		if (UCanvasPanelSlot* DescriptionSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemDescription))
		{
			DescriptionSlot->SetZOrder(1000);
		}
	}
	return ItemDescription;
}

void UINV_SpatialInventory::OpenItemDescription(UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	if (!IsValid(Item) || !IsValid(CanvasPanel)) return;

	UINV_ItemDescription* DescriptionWidget = GetItemDescription();
	if (!IsValid(DescriptionWidget)) return;

	SetItemDescriptionSizeAndPosition(DescriptionWidget, CanvasPanel, OpenPosition);
	DescriptionWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	bSkipDescriptionCloseThisTick = true;
}

void UINV_SpatialInventory::CloseDescriptionIfCursorExited()
{
	if (!IsValid(ItemDescription)) return;
	if (ItemDescription->GetVisibility() == ESlateVisibility::Collapsed) return;
	if (bSkipDescriptionCloseThisTick)
	{
		bSkipDescriptionCloseThisTick = false;
		return;
	}

	// Avoid evaluating exit logic before Slate has a valid layout for this widget.
	if (ItemDescription->GetCachedGeometry().GetLocalSize().IsNearlyZero())
	{
		return;
	}

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	if (ItemDescription->GetCachedGeometry().IsUnderLocation(CursorPosition)) return;
	ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
}
