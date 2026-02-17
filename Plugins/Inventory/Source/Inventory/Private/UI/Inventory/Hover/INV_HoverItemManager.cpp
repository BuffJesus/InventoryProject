// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Hover/INV_HoverItemManager.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

UINV_HoverItem* UINV_HoverItemManager::AssignHoverItem(
	UINV_HoverItem* HoverItem,
	TSubclassOf<UINV_HoverItem> HoverItemClass,
	UINV_InventoryItem* InventoryItem,
	const FINV_GridFragment* GridFragment,
	const FINV_ImageFragment* ImageFragment,
	float TileSize,
	APlayerController* OwningPlayer,
	UUserWidget* ContextWidget)
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
	HoverItem->AddToViewport();

	return HoverItem;
}

void UINV_HoverItemManager::ConfigureHoverItemProperties(
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

void UINV_HoverItemManager::ClearHoverItem(TObjectPtr<UINV_HoverItem>& HoverItem)
{
	if (!IsValid(HoverItem))
	{
		return;
	}

	HoverItem->Clear();
	HoverItem->RemoveFromParent();
	HoverItem = nullptr;
}

UUserWidget* UINV_HoverItemManager::GetOrCreateCursorWidget(
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

void UINV_HoverItemManager::SetCursorWidget(
	APlayerController* PlayerController,
	UUserWidget* CursorWidget)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->SetMouseCursorWidget(EMouseCursor::Default, CursorWidget);
}
