// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"
#include "INV_SpatialInventory.generated.h"

class UINV_ItemDescription;
class UCanvasPanel;
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
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FINV_SlotAvailabilityResult HasRoomForItem(UINV_ItemComponent* ItemComponent) const override;
	virtual void OnItemHovered(UINV_InventoryItem* Item) override;
	virtual void OnItemUnhovered() override;
	virtual void OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition) override;
	virtual bool HasHoverItem() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
private:	
	UINV_ItemDescription* GetItemDescription();
	void OpenItemDescription(UINV_InventoryItem* Item, const FVector2D& OpenPosition);
	void CloseDescriptionIfCursorExited();
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidgetSwitcher> Switcher;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Equippable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Consumable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InventoryGrid> Grid_Craftable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Equippable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Consumable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Craftable;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> CanvasPanel;
	
	UFUNCTION() void ShowEquippableGrid();
	UFUNCTION() void ShowConsumableGrid();
	UFUNCTION() void ShowCraftableGrid();
	
	void DisableButton(UButton* Button);
	void SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button);
	void SetItemDescriptionSizeAndPosition(UINV_ItemDescription* Description, UCanvasPanel* Canvas, const FVector2D& OpenPosition) const;
	
	TWeakObjectPtr<UINV_InventoryGrid> ActiveGrid;
	
	UPROPERTY(EditAnywhere, Category="INV|Spatial")
	TSubclassOf<UINV_ItemDescription> ItemDescriptionClass;
	
	UPROPERTY() TObjectPtr<UINV_ItemDescription> ItemDescription;

	// Prevent immediate close on the same frame the description is opened.
	bool bSkipDescriptionCloseThisTick { false };
};


