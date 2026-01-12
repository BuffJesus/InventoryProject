// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryGrid.generated.h"

struct FINV_ItemManifest;
class UINV_ItemComponent;
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
	FINV_SlotAvailabilityResult HasRoomForItem(const UINV_ItemComponent* ItemComponent);
	virtual void NativeOnInitialized() override;
	
	UFUNCTION()
	void AddItem(UINV_InventoryItem* Item);
	
private:
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	
	FINV_SlotAvailabilityResult HasRoomForItem(const UINV_InventoryItem* Item);
	FINV_SlotAvailabilityResult HasRoomForItem(const FINV_ItemManifest& Manifest);
	void AddItemToIndices(const FINV_SlotAvailabilityResult& Result, UINV_InventoryItem* NewItem);
	
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
