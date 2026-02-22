// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Hover/INV_HoverItemManager.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "UI/Utils/INV_WidgetUtils.h"

UINV_HoverItem* FINV_HoverItemManager::AssignHoverItem(
	UINV_HoverItem* HoverItem,
	TSubclassOf<UINV_HoverItem> HoverItemClass,
	UINV_InventoryItem* InventoryItem,
	const FINV_GridFragment* GridFragment,
	const FINV_ImageFragment* ImageFragment,
	float TileSize,
	APlayerController* OwningPlayer,
	UUserWidget* ContextWidget,
	const TOptional<FVector2D>& InitialViewportCenter)
{
	if (!IsValid(InventoryItem) || !GridFragment || !ImageFragment)
	{
		return HoverItem;
	}

	// Create hover item if needed
	if (!IsValid(HoverItem))
	{
		if (!HoverItemClass || !IsValid(OwningPlayer))
		{
			return nullptr;
		}
		HoverItem = CreateWidget<UINV_HoverItem>(OwningPlayer, HoverItemClass);
	}

	if (!IsValid(HoverItem))
	{
		return nullptr;
	}

	// Calculate draw size
	const FIntPoint GridSize = GridFragment->GetGridSize();
	const FVector2D DrawSize = FVector2D(GridSize.X * TileSize, GridSize.Y * TileSize);

	// Configure icon brush
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = DrawSize * UWidgetLayoutLibrary::GetViewportScale(ContextWidget);

	// Set hover item properties
	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetGridDimensions(GridSize);
	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());
	HoverItem->SetDesiredSizeInViewport(IconBrush.ImageSize);
	HoverItem->SetCachedSize(IconBrush.ImageSize);
	if (!HoverItem->IsInViewport())
	{
		HoverItem->AddToViewport();
	}

	const FVector2D AnchorPosition = InitialViewportCenter.IsSet()
		? InitialViewportCenter.GetValue()
		: UWidgetLayoutLibrary::GetMousePositionOnViewport(ContextWidget);

	// Prevent first-frame flash at (0,0): place hover item immediately before first tick.
	const FVector2D InitialPosition = UINV_WidgetUtils::GetCenteredClampedWidgetPosition(
		UWidgetLayoutLibrary::GetViewportSize(ContextWidget),
		IconBrush.ImageSize,
		AnchorPosition);
	HoverItem->SetPositionInViewport(InitialPosition, false);
	HoverItem->SetVisibility(ESlateVisibility::HitTestInvisible);

	return HoverItem;
}

void FINV_HoverItemManager::ConfigureHoverItemProperties(
	UINV_HoverItem* HoverItem,
	const UINV_GridSlot* GridSlot,
	const UINV_InventoryItem* InventoryItem,
	int32 PreviousGridIndex)
{
	if (!IsValid(HoverItem) || !IsValid(GridSlot) || !IsValid(InventoryItem))
	{
		return;
	}

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlot->GetStackCount() : 0);
}

void FINV_HoverItemManager::ClearHoverItem(TObjectPtr<UINV_HoverItem>& HoverItem)
{
	if (!IsValid(HoverItem))
	{
		return;
	}

	HoverItem->Clear();
	HoverItem->RemoveFromParent();
	HoverItem = nullptr;
}

UUserWidget* FINV_HoverItemManager::GetOrCreateCursorWidget(
	TObjectPtr<UUserWidget>& CachedWidget,
	const TSubclassOf<UUserWidget>& WidgetClass,
	APlayerController* OwningPlayer)
{
	if (!IsValid(OwningPlayer) || WidgetClass == nullptr)
	{
		return nullptr;
	}

	if (!IsValid(CachedWidget))
	{
		CachedWidget = CreateWidget<UUserWidget>(OwningPlayer, WidgetClass);
	}

	return CachedWidget;
}

void FINV_HoverItemManager::SetCursorWidget(
	APlayerController* PlayerController,
	UUserWidget* CursorWidget)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->SetMouseCursorWidget(EMouseCursor::Default, CursorWidget);
}
