// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/INV_InventoryItem.h"
#include "INV_SlottedItem.generated.h"

class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSlottedItemClicked, int32, GridIndex, const FPointerEvent&, MouseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSlottedItemHovered, int32, GridIndex, const FPointerEvent&, MouseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSlottedItemUnhovered, int32, GridIndex, const FPointerEvent&, MouseEvent);

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_SlottedItem : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Handles click on the item image.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	
	bool IsStackable() const { return bIsStackable; }
	void SetIsStackable(bool bStackable) { bIsStackable = bStackable; }
	UImage* GetImageIcon() const { return Image_Icon; }
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	int32 GetGridIndex() const { return GridIndex; }
	void SetGridDimensions(const FIntPoint Dimensions) { GridDimensions = Dimensions; }
	FIntPoint GetGridDimensions() const { return GridDimensions; }
	UINV_InventoryItem* GetInventoryItem() const { return InventoryItem.Get(); }
	void SetInventoryItem(UINV_InventoryItem* Item) { InventoryItem = Item; }
	void SetImageBrush(const FSlateBrush& Brush) const;
	// Update the visible stack count.
	void UpdateStackCount(int32 StackCount);
	
	FSlottedItemClicked OnSlottedItemClicked;
	FSlottedItemHovered OnSlottedItemHovered;
	FSlottedItemUnhovered OnSlottedItemUnhovered;
	
private:
	// Icon image in the widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	// Stack count text in the widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StackCount;
	
	// Grid index this item is anchored to.
	int32 GridIndex { INDEX_NONE };
	// Size in grid tiles.
	FIntPoint GridDimensions;
	// Backing inventory item.
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	// True if stacks are shown.
	bool bIsStackable { false };
};
