// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_SlottedItem.h"
#include "INV_EquippedSlottedItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquippedSlottedItemClicked, class UINV_EquippedSlottedItem*, SlottedItem, const FPointerEvent&, MouseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedSlottedItemHovered, class UINV_EquippedSlottedItem*, SlottedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedSlottedItemUnhovered, class UINV_EquippedSlottedItem*, SlottedItem);

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_EquippedSlottedItem : public UINV_SlottedItem
{
	GENERATED_BODY()
	
public:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	void SetHighlightEnabled(bool bEnabled) const;
	
	void SetEquipmentTypeTag(FGameplayTag Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }
	
	FEquippedSlottedItemClicked OnEquippedSlottedItemClicked;
	FEquippedSlottedItemHovered OnEquippedSlottedItemHovered;
	FEquippedSlottedItemUnhovered OnEquippedSlottedItemUnhovered;
	
private:
	UPROPERTY() FGameplayTag EquipmentTypeTag;

	// Tint used while hovering an equipped item.
	UPROPERTY(EditAnywhere, Category="INV|Visual")
	FLinearColor HighlightTint { 1.15f, 1.15f, 1.15f, 1.0f };
};
