// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UI/Base/INV_InventoryBase.h"
#include "INV_SpatialInventory.generated.h"

class UINV_EquippedGridSlot;
class UINV_EquippedSlottedItem;
class UTextBlock;
struct FGameplayTag;
struct FKey;
class UINV_ItemDescription;
class UCanvasPanel;
class UButton;
class UWidgetSwitcher;
class UINV_InventoryGrid;
class UINV_InventoryComponent;
/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_SpatialInventory : public UINV_InventoryBase
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FINV_SlotAvailabilityResult HasRoomForItem(UINV_ItemComponent* ItemComponent) const override;
	virtual void OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition) override;
	virtual bool HasHoverItem() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual UINV_HoverItem* GetHoverItem() const override;
	virtual float GetTileSize() const override;
	virtual void ReturnActiveHoverItemToSource() override;
	bool TryEquipHoveredItemFromController();
	
private:	
	UINV_ItemDescription* GetItemDescription();
	void OpenItemDescription(UINV_InventoryItem* Item, const FVector2D& OpenPosition);
	void CloseDescriptionIfCursorExited();
	
	UPROPERTY() TArray<TObjectPtr<UINV_EquippedGridSlot>> EquippedGridSlots;
	
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
	UFUNCTION() void EquippedGridSlotClicked(UINV_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag);
	UFUNCTION() void EquippedSlottedItemClicked(UINV_EquippedSlottedItem* EquippedSlottedItem, const FPointerEvent& MouseEvent);
	
	void DisableButton(UButton* Button) const;
	void SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button);
	void ForEachInventoryGrid(TFunctionRef<void(UINV_InventoryGrid* Grid)> Visitor) const;
	void ForEachCategoryButton(TFunctionRef<void(UButton* Button)> Visitor) const;
	void SwitchCategoryByDirection(int32 Direction);
	bool IsGridNavigationInput(const FKey& PressedKey) const;
	void EnsureControllerHintWidget();
	void SetItemDescriptionSizeAndPosition(UINV_ItemDescription* Description, UCanvasPanel* Canvas, const FVector2D& OpenPosition) const;
	float ResolveInventoryTileSize() const;
	bool TryHandleEquippedItemInspect(UINV_EquippedSlottedItem* EquippedSlottedItem, const FPointerEvent& MouseEvent);
	void SwapEquippedItemWithHover(UINV_EquippedSlottedItem* EquippedSlottedItem);
	UINV_InventoryComponent* ResolveInventoryComponent() const;
	void BroadcastEquipState(UINV_InventoryComponent* InventoryComponent, UINV_InventoryItem* ItemToEquip, UINV_InventoryItem* ItemToUnequip) const;
	void BroadcastSlotClickedDelegates(UINV_InventoryItem* ItemToEquip, UINV_InventoryItem* ItemToUnequip);
	void BindEquippedSlottedItemDelegates(UINV_EquippedSlottedItem* EquippedSlottedItem);
	void UnbindEquippedSlottedItemDelegates(UINV_EquippedSlottedItem* EquippedSlottedItem);
	UINV_EquippedGridSlot* FindSlotWithEquippedItem(UINV_InventoryItem* EquippedItem) const;
	UINV_InventoryGrid* FindGridWithHoverItem() const;
	
	TWeakObjectPtr<UINV_InventoryGrid> ActiveGrid;
	
	UPROPERTY(EditAnywhere, Category="INV|Spatial")
	TSubclassOf<UINV_ItemDescription> ItemDescriptionClass;
	
	UPROPERTY() TObjectPtr<UINV_ItemDescription> ItemDescription;
	UPROPERTY() TObjectPtr<UTextBlock> ControllerHintsText;

	UPROPERTY(EditAnywhere, Category="INV|Controller")
	FText ControllerHintsLabel { FText::FromString(TEXT("A Select   B Back   X Options   LB/RB Category")) };

	// Prevent immediate close on the same frame the description is opened.
	bool bSkipDescriptionCloseThisTick { false };
	
	bool CanEquipHoverItem(UINV_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const;
};


