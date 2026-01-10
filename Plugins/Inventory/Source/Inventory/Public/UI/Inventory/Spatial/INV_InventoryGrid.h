// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryGrid.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category="INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
};
