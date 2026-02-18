// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Factory/INV_GridWidgetFactory.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Utils/INV_WidgetUtils.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

UINV_SlottedItem* UINV_GridWidgetFactory::CreateSlottedItem(
	UINV_InventoryItem* Item,
	const bool bStackable,
	const int32 StackAmount,
	const FINV_GridFragment* GridFragment,
	const FINV_ImageFragment* ImageFragment,
	const int32 Index,
	const FINV_GridWidgetFactoryConfig& Config,
	UUserWidget* Outer,
	TSubclassOf<UINV_SlottedItem> SlottedItemClass)
{
	if (!IsValid(Outer) || !SlottedItemClass) return nullptr;
	if (!GridFragment || !ImageFragment) return nullptr;

	APlayerController* PC = Outer->GetOwningPlayer();
	if (!IsValid(PC)) return nullptr;

	UINV_SlottedItem* SlottedItem = CreateWidget<UINV_SlottedItem>(PC, SlottedItemClass);
	if (!IsValid(SlottedItem)) return nullptr;

	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImageBrush(GridFragment, ImageFragment, SlottedItem, Config.TileSize);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);

	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);

	return SlottedItem;
}

UINV_GridSlot* UINV_GridWidgetFactory::CreateGridSlot(
	const int32 Index,
	const FINV_GridWidgetFactoryConfig& Config,
	UUserWidget* Outer,
	TSubclassOf<UINV_GridSlot> GridSlotClass)
{
	if (!IsValid(Outer) || !GridSlotClass) return nullptr;

	UINV_GridSlot* GridSlot = CreateWidget<UINV_GridSlot>(Outer, GridSlotClass);
	if (!IsValid(GridSlot)) return nullptr;

	GridSlot->SetTileIndex(Index);

	return GridSlot;
}

FVector2D UINV_GridWidgetFactory::CalculateDrawSize(
	const FINV_GridFragment* GridFragment,
	const float TileSize)
{
	if (!GridFragment) return FVector2D::ZeroVector;

	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	const FVector2D IconSize = GridFragment->GetGridSize() * IconTileWidth;
	return IconSize;
}

FVector2D UINV_GridWidgetFactory::CalculateWidgetPosition(
	const int32 Index,
	const FINV_GridFragment* GridFragment,
	const FINV_GridWidgetFactoryConfig& Config)
{
	FVector2D DrawPos = UINV_WidgetUtils::GetPositionFromIndex(Index, Config.GridWidth) * Config.TileSize;

	if (GridFragment)
	{
		DrawPos += FVector2D(GridFragment->GetGridPadding());
	}

	return DrawPos;
}

void UINV_GridWidgetFactory::AddSlottedItemToCanvas(
	UINV_SlottedItem* SlottedItem,
	const int32 Index,
	const FINV_GridFragment* GridFragment,
	const FINV_GridWidgetFactoryConfig& Config)
{
	if (!IsValid(SlottedItem) || !IsValid(Config.CanvasPanel)) return;
	if (!GridFragment) return;

	Config.CanvasPanel->AddChild(SlottedItem);

	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	if (!IsValid(CanvasSlot)) return;

	CanvasSlot->SetSize(CalculateDrawSize(GridFragment, Config.TileSize));
	CanvasSlot->SetPosition(CalculateWidgetPosition(Index, GridFragment, Config));
}

void UINV_GridWidgetFactory::AddGridSlotToCanvas(
	UINV_GridSlot* GridSlot,
	const int32 Index,
	const FINV_GridWidgetFactoryConfig& Config)
{
	if (!IsValid(GridSlot) || !IsValid(Config.CanvasPanel)) return;

	Config.CanvasPanel->AddChild(GridSlot);

	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
	if (!IsValid(CanvasSlot)) return;

	CanvasSlot->SetSize(FVector2D(Config.TileSize));
	CanvasSlot->SetPosition(CalculateWidgetPosition(Index, nullptr, Config));
}

void UINV_GridWidgetFactory::SetSlottedItemImageBrush(
	const FINV_GridFragment* GridFragment,
	const FINV_ImageFragment* ImageFragment,
	UINV_SlottedItem* SlottedItem,
	const float TileSize)
{
	if (!GridFragment || !ImageFragment || !IsValid(SlottedItem)) return;

	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = CalculateDrawSize(GridFragment, TileSize);
	SlottedItem->SetImageBrush(Brush);
}
