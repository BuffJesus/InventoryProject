// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Types/INV_GridTypes.h"
#include "INV_InventoryGrid.generated.h"

class UINV_HoverItem;
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	
	bool HasRoomAtIndex(const UINV_GridSlot* GridSlot, 
		const FIntPoint& Dimensions,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize);
	
	FIntPoint GetItemDimensions(const FINV_ItemManifest& Manifest) const;
	
	bool CheckSlotConstraints(const UINV_GridSlot* GridSlot, 
		const UINV_GridSlot* SubGridSlot, 
		const TSet<int32>& CheckedIndices, 
		TSet<int32> OutTentativelyClaimed, 
		const FGameplayTag& ItemType,
		const int32 MaxStackSize) const;
	
	bool IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const;
	bool HasValidItem(const UINV_GridSlot* GridSlot) const;
	bool IsUpperLeftSlot(const UINV_GridSlot* GridSlot, const UINV_GridSlot* SubGridSlot) const;
	bool DoesItemTypeMatch(const UINV_InventoryItem* SubItem, const FGameplayTag& ItemType) const;
	bool IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const;
	bool MatchesCategory(const UINV_InventoryItem* Item) const;
	bool IsRightClick(const FPointerEvent& MouseEvent) const;
	bool IsLeftClick(const FPointerEvent& MouseEvent) const;
	
	int32 DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize, 
		const int32 AmountToFill, const UINV_GridSlot* GridSlot) const;
	
	int32 GetStackAmount(const UINV_GridSlot* GridSlot) const;
	
	void ConstructGrid();
	void Pickup(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	void AssignHoverItem(UINV_InventoryItem* InventoryItem);
	void AssignHoverItem(UINV_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex);
	void RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex);
	void UpdateTileParams(const FVector2D& CanvasPos, const FVector2D& MousePos);
	void OnTileParamsUpdated(const FINV_TileParams& Params);

	FIntPoint CalculateHoverCoordinates(const FVector2D& CanvasPos, const FVector2D& MousePos) const;
	FIntPoint CalculateStartingCoordinate(const FIntPoint& Coord, const FIntPoint& Dimensions, const EINV_TileQuadrant Quadrant) const;
	EINV_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPos, const FVector2D& MousePos) const;
	FINV_SpaceQueryResult CheckHoverPosition(const FIntPoint& Pos, const FIntPoint& Dimensions);
	
	UFUNCTION() void AddStacks(const FINV_SlotAvailabilityResult& Result);
	
	UFUNCTION() void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category="INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> CanvasPanel;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_SlottedItem> SlottedItemClass;
	
	UPROPERTY() TMap<int32, TObjectPtr<UINV_SlottedItem>> SlottedItems;
	
	UPROPERTY() TArray<TObjectPtr<UINV_GridSlot>> GridSlots;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_GridSlot> SlotClass;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") FIntPoint GridSize { 8, 4 };
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") float TileSize { 54.0f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_HoverItem> HoverItemClass;
	UPROPERTY() TObjectPtr<UINV_HoverItem> HoverItem;
	
	FINV_TileParams TileParams;
	FINV_TileParams LastTileParams;
	
	FINV_SpaceQueryResult CurrentQueryResult;
	
	// Index where an item would be placed if we click on grid at valid location
	int32 ItemDropIndex { INDEX_NONE };
};
