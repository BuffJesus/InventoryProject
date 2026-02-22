// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_GridSlot.h"
#include "INV_EquippedGridSlot.generated.h"

class UOverlay;
class UINV_EquippedSlottedItem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquippedGridSlotClicked, UINV_EquippedGridSlot*, GridSlot,
                                             const FGameplayTag&, EquipmentTypeTag);

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_EquippedGridSlot : public UINV_GridSlot
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	bool SetEquippedItemImageBrush(UINV_InventoryItem* Item, const FVector2D& DrawSize);
	void AddEquippedItemToOverlay();
	void SetSlotOccupiedVisual();
	void SetSlotEmptyVisual();
	void SetEquippedSlottedItem(UINV_EquippedSlottedItem* Item) { EquippedSlottedItem = Item; };
	UINV_EquippedSlottedItem* GetEquippedSlottedItem() const { return EquippedSlottedItem; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }

	UINV_EquippedSlottedItem* OnItemEquipped(UINV_InventoryItem* Item, const FGameplayTag& EquipmentTag, float TileSize);
	
	FEquippedGridSlotClicked EquippedGridSlotClicked;
	
private:
	bool CanUpdateHoverTexture() const;
	
	UPROPERTY(EditAnywhere, Category="INV|Tags", meta = (Categories = "GameItems.Equipment"))
	FGameplayTag EquipmentTypeTag;
	
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_GrayedOutIcon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UOverlay> Overlay_Root;
	UPROPERTY() TObjectPtr<UINV_EquippedSlottedItem> EquippedSlottedItem;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	TSubclassOf<UINV_EquippedSlottedItem> EquippedSlottedItemClass;

	// Extra inset (in pixels) applied to the equipped icon to better match slot art padding.
	UPROPERTY(EditAnywhere, Category="INV|Grid", meta=(ClampMin="0.0", UIMin="0.0"))
	float EquippedIconPadding { 8.f };
};
