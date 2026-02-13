// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "INV_SlottedItem.generated.h"

class UINV_InventoryItem;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSlottedItemClicked, int32, GridIndex, const FPointerEvent&, MouseEvent);

/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_SlottedItem : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Handles click on the item image.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	bool IsStackable() const { return bIsStackable; }
	void SetIsStackable(bool bStackable) { bIsStackable = bStackable; }
	UImage* GetImageIcon() const { return Image_Icon; }
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	int32 GetGridIndex() const { return GridIndex; }
	void SetGridDimensions(const FIntPoint Dimensions) { GridDimensions = Dimensions; }
	FIntPoint GetGridDimensions() const { return GridDimensions; }
	UINV_InventoryItem* GetInventoryItem() const { return InventoryItem.Get(); }
	void SetInventoryItem(UINV_InventoryItem* Item) { InventoryItem = Item; }
	void SetImageBrush(const FSlateBrush& Brush) const { Image_Icon->SetBrush(Brush); }
	// Update the visible stack count.
	void UpdateStackCount(int32 StackCount);
	
	FSlottedItemClicked OnSlottedItemClicked;
	
private:
	// Icon image in the widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	// Stack count text in the widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StackCount;
	
	// Grid index this item is anchored to.
	int32 GridIndex;
	// Size in grid tiles.
	FIntPoint GridDimensions;
	// Backing inventory item.
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	// True if stacks are shown.
	bool bIsStackable { false };
};
