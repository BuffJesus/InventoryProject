// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryBase.generated.h"

class UINV_ItemComponent;
struct FINV_SlotAvailabilityResult;
/**
 * 
 */
UCLASS(Abstract)
class INVENTORY_API UINV_InventoryBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE virtual FINV_SlotAvailabilityResult HasRoomForItem(UINV_ItemComponent* ItemComponent) const
	{
		return FINV_SlotAvailabilityResult();
	}
};
