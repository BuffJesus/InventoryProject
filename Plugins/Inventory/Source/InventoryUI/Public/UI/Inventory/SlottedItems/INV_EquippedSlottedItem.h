// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_SlottedItem.h"
#include "INV_EquippedSlottedItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedSlottedItemClicked, class UINV_EquippedSlottedItem*, SlottedItem);

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_EquippedSlottedItem : public UINV_SlottedItem
{
	GENERATED_BODY()
	
public:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	void SetEquipmentTypeTag(FGameplayTag Tag) { EquipmentTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquipmentTypeTag; }
	
	FEquippedSlottedItemClicked OnEquippedSlottedItemClicked;
	
private:
	UPROPERTY() FGameplayTag EquipmentTypeTag;
};
