// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryGrid.generated.h"

class UINV_SlottedItem;
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
	FVector2D GetDrawSize(const FINV_GridFragment* GridFragment) const;
	
	void SetSlottedItemImageBrush(const FINV_GridFragment* GridFragment, const FINV_ImageFragment* ImageFragment,
								  const UINV_SlottedItem* SlottedItem) const;
	
	UINV_SlottedItem* CreateSlottedItem(UINV_InventoryItem* Item, 
										const bool bStackable, 
										int32 StackAmount, 
										const FINV_GridFragment* GridFragment,
										const FINV_ImageFragment* ImageFragment,
										const int32 Index);
	
	void AddItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	void AddSlottedItemToCanvas(const int32 Index, const FINV_GridFragment* GridFragment, UINV_SlottedItem* SlottedItem);
	void UpdateGridSlots(UINV_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount);
	
	void ConstructGrid();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category="INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> CanvasPanel;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_SlottedItem> SlottedItemClass;
	
	UPROPERTY() TMap<int32, TObjectPtr<UINV_SlottedItem>> SlottedItems;
	
	UPROPERTY() TArray<TObjectPtr<UINV_GridSlot>> GridSlots;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_GridSlot> SlotClass;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") FIntPoint GridSize { 8, 4 };
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") float TileSize { 54.0f };
	
	bool MatchesCategory(const UINV_InventoryItem* Item) const;
};
