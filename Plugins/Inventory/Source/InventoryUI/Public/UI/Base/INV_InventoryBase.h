// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryBase.generated.h"

class UINV_HoverItem;
class UINV_ItemComponent;
class UINV_InventoryItem;
class UINV_InventoryGrid;
struct FINV_SlotAvailabilityResult;
/**
 * 
 */
UCLASS(Abstract)
class INVENTORYUI_API UINV_InventoryBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Base hook for asking if an item can fit.
	virtual FINV_SlotAvailabilityResult HasRoomForItem(UINV_ItemComponent* ItemComponent) const
	{
		return FINV_SlotAvailabilityResult();
	}
	
	virtual void OnItemHovered(UINV_InventoryItem* Item) {}
	virtual void OnItemUnhovered() {}
	virtual void OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition) {}
	virtual void ReturnActiveHoverItemToSource() {}
	virtual void ClearActiveHoverItem() {}
	virtual bool HasHoverItem() const { return false; }
	virtual UINV_HoverItem* GetHoverItem() const { return nullptr; }
	virtual UINV_InventoryGrid* GetHoverSourceGrid() const { return nullptr; }
	virtual float GetTileSize() const { return 0.0f; }
};
