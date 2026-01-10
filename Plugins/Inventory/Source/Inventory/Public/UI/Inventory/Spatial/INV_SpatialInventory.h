// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"
#include "INV_SpatialInventory.generated.h"

class UButton;
class UWidgetSwitcher;
class UINV_InventoryGrid;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_SpatialInventory : public UINV_InventoryBase
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual FINV_SlotAvailabilityResult HasRoomForItem(UINV_ItemComponent* ItemComponent) const override;
	
private:	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidgetSwitcher> Switcher;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Equippable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Consumable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Craftable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Equippable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Consumable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Craftable;
	
	UFUNCTION() void ShowEquippableGrid();
	UFUNCTION() void ShowConsumableGrid();
	UFUNCTION() void ShowCraftableGrid();
	
	void DisableButton(UButton* Button);
	void SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button);
};


