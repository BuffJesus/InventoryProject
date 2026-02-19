// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_GridWidgetFactory.generated.h"

class UINV_SlottedItem;
class UINV_GridSlot;
class UINV_InventoryItem;
class UCanvasPanel;
class UUserWidget;
struct FINV_GridFragment;
struct FINV_ImageFragment;

/**
 * Configuration data for widget factory
 */
USTRUCT(BlueprintType)
struct FINV_GridWidgetFactoryConfig
{
	GENERATED_BODY()

	/** Size of each tile in the grid */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TileSize { 64.0f };

	/** Width of the grid in tiles */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 GridWidth { 1 };

	/** Canvas panel to add widgets to */
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UCanvasPanel> CanvasPanel { nullptr };
};

/**
 * Factory for creating and configuring inventory grid widgets.
 * Handles slotted items, grid slots, and their visual properties.
 */
class INVENTORY_API FINV_GridWidgetFactory
{
public:
	/**
	 * Create a slotted item widget with all properties configured.
	 *
	 * @param Item The inventory item to display
	 * @param bStackable Whether this item is stackable
	 * @param StackAmount Current stack count
	 * @param GridFragment Grid size/padding data
	 * @param ImageFragment Icon/image data
	 * @param Index Grid index position
	 * @param Config Factory configuration
	 * @param Outer Widget outer (typically the grid widget)
	 * @param SlottedItemClass Class to instantiate
	 * @return Configured slotted item widget
	 */
	static UINV_SlottedItem* CreateSlottedItem(
		UINV_InventoryItem* Item,
		bool bStackable,
		int32 StackAmount,
		const FINV_GridFragment* GridFragment,
		const FINV_ImageFragment* ImageFragment,
		int32 Index,
		const FINV_GridWidgetFactoryConfig& Config,
		UUserWidget* Outer,
		TSubclassOf<UINV_SlottedItem> SlottedItemClass);

	/**
	 * Create a grid slot widget.
	 *
	 * @param Index Grid index position
	 * @param Config Factory configuration
	 * @param Outer Widget outer (typically the grid widget)
	 * @param GridSlotClass Class to instantiate
	 * @return Configured grid slot widget
	 */
	static UINV_GridSlot* CreateGridSlot(
		int32 Index,
		const FINV_GridWidgetFactoryConfig& Config,
		UUserWidget* Outer,
		TSubclassOf<UINV_GridSlot> GridSlotClass);

	/**
	 * Calculate the draw size for an item based on its grid dimensions.
	 *
	 * @param GridFragment Grid size/padding data
	 * @param TileSize Size of each tile
	 * @return Size in slate units
	 */
	static FVector2D CalculateDrawSize(
		const FINV_GridFragment* GridFragment,
		float TileSize);

	/**
	 * Calculate the position for a widget at a grid index.
	 *
	 * @param Index Grid index position
	 * @param GridFragment Grid size/padding data (optional, for padding offset)
	 * @param Config Factory configuration
	 * @return Position in slate units
	 */
	static FVector2D CalculateWidgetPosition(
		int32 Index,
		const FINV_GridFragment* GridFragment,
		const FINV_GridWidgetFactoryConfig& Config);

	/**
	 * Add a slotted item widget to the canvas at the specified position.
	 *
	 * @param SlottedItem Widget to add
	 * @param Index Grid index position
	 * @param GridFragment Grid size/padding data
	 * @param Config Factory configuration
	 */
	static void AddSlottedItemToCanvas(
		UINV_SlottedItem* SlottedItem,
		int32 Index,
		const FINV_GridFragment* GridFragment,
		const FINV_GridWidgetFactoryConfig& Config);

	/**
	 * Add a grid slot widget to the canvas at the specified position.
	 *
	 * @param GridSlot Widget to add
	 * @param Index Grid index position
	 * @param Config Factory configuration
	 */
	static void AddGridSlotToCanvas(
		UINV_GridSlot* GridSlot,
		int32 Index,
		const FINV_GridWidgetFactoryConfig& Config);

private:
	/**
	 * Configure the image brush for a slotted item.
	 */
	static void SetSlottedItemImageBrush(
		const FINV_GridFragment* GridFragment,
		const FINV_ImageFragment* ImageFragment,
		UINV_SlottedItem* SlottedItem,
		float TileSize);
};
