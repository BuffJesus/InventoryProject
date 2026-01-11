// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryGrid.generated.h"

class UINV_InventoryComponent;
class UCanvasPanel;
class UINV_GridSlot;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	virtual void NativeOnInitialized() override;
	
	UFUNCTION()
	void AddItem(UINV_InventoryItem* Item);
	
private:
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	
	void ConstructGrid();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category="INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> CanvasPanel;
	UPROPERTY() TArray<TObjectPtr<UINV_GridSlot>> GridSlots;
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_GridSlot> SlotClass;
	UPROPERTY(EditAnywhere, Category = "INV|Grid") FIntPoint GridSize { 8, 4 };
	UPROPERTY(EditAnywhere, Category = "INV|Grid") float TileSize { 54.0f };
	
	bool MatchesCategory(const UINV_InventoryItem* Item) const;
};
