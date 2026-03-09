// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Delegates/Delegate.h"
#include "Types/INV_GridTypes.h"

class UINV_ItemPopUp;
enum class EINV_GridSlotState : uint8;
class UINV_HoverItem;
class UINV_SlottedItem;
class UINV_InventoryItem;
struct FGameplayTag;
struct FINV_ItemManifest;
struct FINV_GridFragment;
struct FINV_ImageFragment;
struct FINV_GridWidgetFactoryConfig;
class UINV_ItemComponent;
struct FINV_GridClickExecutionCallbacks;
struct FINV_GridSwapCallbacks;

#include "INV_InventoryGrid.generated.h"
class UINV_InventoryComponent;
class UCanvasPanel;
class UINV_GridSlot;
/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Category filter for items in this grid.
	FORCEINLINE EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	// Check if an item component can fit.
	FINV_SlotAvailabilityResult HasRoomForItem(const UINV_ItemComponent* ItemComponent);
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	UFUNCTION()
	// Adds a replicated item to the UI grid.
	void AddItem(UINV_InventoryItem* Item);
	UFUNCTION() void RemoveItem(UINV_InventoryItem* Item);
	UFUNCTION() void HandleInventoryItemChanged(UINV_InventoryItem* Item);
	
	void ShowCursor();
	void HideCursor();
	void DropItem();
	void ClearHoverItem();
	void AssignHoverItem(UINV_InventoryItem* InventoryItem);
	bool ReturnHoverItemToPreviousSlot();
	bool HasOpenItemPopup() const;
	void CloseItemPopup();
	
	UINV_HoverItem* GetHoverItem() const { return HoverItem; } 
	
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	
	FORCEINLINE bool HasHoverItem() const { return HoverItem.Get() != nullptr; }
	
	float GetTileSize() const { return TileSize; }

private:
	// Owning inventory component for events.
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel { nullptr };

	// Private overloads for internal use
	FINV_SlotAvailabilityResult HasRoomForItem(const UINV_InventoryItem* Item);
	FINV_SlotAvailabilityResult HasRoomForItem(const FINV_ItemManifest* Manifest);
	FINV_SlotAvailabilityResult HasRoomForItem(const FINV_ItemManifest* Manifest, bool bUseItemRarity, const FGameplayTag& ItemRarityTag);
	// Apply UI updates for item placement.
	void AddItemToIndices(const FINV_SlotAvailabilityResult& Result, UINV_InventoryItem* NewItem);
	FVector2D GetDrawSize(const FINV_GridFragment* GridFragment) const;

	UINV_SlottedItem* CreateSlottedItem(UINV_InventoryItem* Item, 
										const bool bStackable, 
										int32 StackAmount, 
										const FINV_GridFragment* GridFragment,
										const FINV_ImageFragment* ImageFragment,
										const int32 Index);
	UINV_SlottedItem* AcquireSlottedItem(UINV_InventoryItem* Item,
		const bool bStackable,
		int32 StackAmount,
		const FINV_GridFragment* GridFragment,
		const FINV_ImageFragment* ImageFragment,
		const int32 Index);
	void ReleaseSlottedItem(UINV_SlottedItem* SlottedItem);
	void BindSlottedItemDelegates(UINV_SlottedItem* SlottedItem);
	void BindInventoryItemNotifications(UINV_InventoryItem* Item);
	void UnbindInventoryItemNotifications(UINV_InventoryItem* Item);
	int32 FindUpperLeftIndexForItem(const UINV_InventoryItem* Item) const;
	void RefreshItemVisuals(UINV_InventoryItem* Item);
	
	void AddItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	void PlaceItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	void AddSlottedItemToCanvas(const int32 Index, const FINV_GridFragment* GridFragment, UINV_SlottedItem* SlottedItem);
	void UpdateGridSlots(UINV_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount);
	void PutDownOnIndex(const int32 Index);
	UINV_InventoryItem* GetHoverInventoryItem() const;
	void SetCursorWidget(UUserWidget* CursorWidget);
	void SwapWithHoverItem(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	void SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	void ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	void FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index);
	void CreateItemPopup(const int32 GridIndex);
	void CloseActiveItemPopup();
	void ClosePopupIfClickedOutside();
	void FinalizeHoverItemAssignment();
	TOptional<FVector2D> ComputeSourceViewportCenterForGridIndex(int32 GridIndex) const;
	int32 ResolveControllerActionIndex(int32 GridIndex) const;
	void ApplyItemFootprintVisualAtIndex(int32 GridIndex, bool bSelectedVisual);
	void BindItemPopupCallbacksIfNeeded();
	
	UPROPERTY(EditAnywhere, Category="INV|Inventory")
	TSubclassOf<UINV_ItemPopUp> ItemPopUpClass;
	
	UPROPERTY() TObjectPtr<UINV_ItemPopUp> ItemPopUp;
	TWeakObjectPtr<UINV_ItemPopUp> BoundItemPopUpForCallbacks { nullptr };
	
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	UUserWidget* GetCursorWidget(TObjectPtr<UUserWidget>& CachedWidget, const TSubclassOf<UUserWidget>& WidgetClass);
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<UUserWidget> VisibleCursorWidgetClass;
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<UUserWidget> HiddenCursorWidgetClass;
	
	UPROPERTY() TObjectPtr<UUserWidget> VisibleCursorWidget;
	UPROPERTY() TObjectPtr<UUserWidget> HiddenCursorWidget;

	// Note: Core placement logic methods moved to FINV_GridPlacementEngine for reusability and testability
	bool MatchesCategory(const UINV_InventoryItem* Item) const;
	bool CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Loc);
	bool GetRightClickedInventoryItem(int32 Index, UINV_InventoryItem*& RightClickedItem);
	
	
	// Helper to create factory config with current grid settings
	FINV_GridWidgetFactoryConfig CreateFactoryConfig() const;

	void ConstructGrid();
	void Pickup(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	void AssignHoverItem(UINV_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex);
	void RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex);
	void UpdateTileParams(const FVector2D& CanvasPos, const FVector2D& MousePos);
	void OnTileParamsUpdated(const FINV_TileParams& Params);
	void ApplyHoverPlacementVisuals(const FIntPoint& Dimensions);
	void HighlightHoverFootprintSelection(const FIntPoint& Dimensions);
	void HighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	void HighlightBlockingItems(const TArray<int32>& BlockingUpperLeftIndices);
	void UnHighlightBlockingItems();
	void RefreshGridSlotVisualsFromAvailability();
	void ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EINV_GridSlotState GridSlotState);

	FIntPoint CalculateHoverCoordinates(const FVector2D& CanvasPos, const FVector2D& MousePos) const;
	EINV_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPos, const FVector2D& MousePos) const;
	const FINV_GridFragment* TryGetGridFragmentAtIndex(int32 Index) const;
	FIntPoint GetItemDimensionsOrDefault(const UINV_InventoryItem* Item) const;
	
	UFUNCTION() void AddStacks(const FINV_SlotAvailabilityResult& Result);
	FINV_StackDetails CalculateStackDetails(int32 GridIndex, UINV_InventoryItem* ClickedInventoryItem);
	FINV_GridClickExecutionCallbacks BuildGridSlotClickCallbacks(const FPointerEvent& MouseEvent);
	FINV_GridClickExecutionCallbacks BuildSlottedItemClickCallbacks();
	FINV_GridSwapCallbacks BuildGridSwapCallbacks();

	UFUNCTION() void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnSlottedItemHovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnSlottedItemUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	
	UFUNCTION() void OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnPopUpMenuSplit(int32 SplitAmount, int32 Index);
	UFUNCTION() void OnPopUpMenuDrop(int32 Index);
	UFUNCTION() void OnPopUpMenuConsume(int32 Index);
	UFUNCTION() void OnPopUpMenuInspect(int32 Index);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category="INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> CanvasPanel;
	
	UPROPERTY(EditAnywhere, Category = "INV|Grid") TSubclassOf<UINV_SlottedItem> SlottedItemClass;
	
	UPROPERTY() TMap<int32, TObjectPtr<UINV_SlottedItem>> SlottedItems;
	UPROPERTY() TArray<TObjectPtr<UINV_SlottedItem>> SlottedItemPool;
	TMap<UINV_InventoryItem*, FDelegateHandle> ItemChangedDelegateHandles;
	UPROPERTY(EditAnywhere, Category = "INV|Grid", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxPooledSlottedItems { 128 };
	
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
	
	bool bMouseWithinCanvas { false };
	bool bLastMouseWithinCanvas { false };
	bool bHasLastTickInputs { false };
	int32 LastHighlightedIndex { INDEX_NONE };
	int32 LastHoveredSlottedIndex { INDEX_NONE };
	FIntPoint LastHighlightedDimensions;
	TArray<int32> LastGrayedOutUpperLeftIndices;
	FVector2D LastTickCanvasPos { FVector2D::ZeroVector };
	FVector2D LastTickCanvasSize { FVector2D::ZeroVector };
	FVector2D LastTickMousePos { FVector2D::ZeroVector };
	double LastPopupOpenedRealtimeSeconds { -1.0 };
	double LastHoverAssignedRealtimeSeconds { -1.0 };
};

