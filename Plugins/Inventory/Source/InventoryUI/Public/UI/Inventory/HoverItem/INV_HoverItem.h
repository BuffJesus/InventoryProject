// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "INV_HoverItem.generated.h"

class UTextBlock;
class UImage;
class UINV_InventoryItem;

/**
 * The HoverItem is the item that will appear and follow mouse when inventory item has been clicked
 */
UCLASS()
class INVENTORYUI_API UINV_HoverItem : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set the hover icon image.
	void SetImageBrush(const FSlateBrush& Brush) const;
	// Update stack count text.
	void UpdateStackCount(const int32 Count);
	
	// Cached item type for quick comparisons.
	FGameplayTag GetItemType() const;
	FORCEINLINE int32 GetStackCount() const { return StackCount; }
	FORCEINLINE void SetStackCount(int32 Count) { StackCount = Count; }
	FORCEINLINE bool IsStackable() const { return bIsStackable; }
	void SetIsStackable(bool bStacks);
	FORCEINLINE int32 GetPreviousGridIndex() const { return PreviousGridIndex; }
	FORCEINLINE void SetPreviousGridIndex(int32 Index) { PreviousGridIndex = Index; }
	FORCEINLINE FIntPoint GetGridDimensions() const { return GridDimensions; }
	FORCEINLINE void SetGridDimensions(const FIntPoint Dimensions) { GridDimensions = Dimensions; }
	UINV_InventoryItem* GetInventoryItem() const;
	void SetInventoryItem(UINV_InventoryItem* Item);
	FORCEINLINE void SetCachedSize(const FVector2D& Size) { CachedSize = Size; }
	FORCEINLINE FVector2D GetCachedSize() const { return CachedSize; }
	
	// Reset hover item state.
	void Clear();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnInitialized() override;

private:
	// Cached size used to center the hover widget on the cursor.
	FVector2D CachedSize { FVector2D::ZeroVector };
     
	// Reserved for timed position updates (currently unused).
	FTimerHandle PositionTimerHandle;
     
	UPROPERTY(EditDefaultsOnly, Category = "Hover Item")
	// How often the hover item updates (in seconds).
	float PositionUpdateRate = 0.016f; // ~60fps
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StackCount;
	
	// Index this item came from.
	int32 PreviousGridIndex { INDEX_NONE };
	// Size in grid tiles.
	FIntPoint GridDimensions;
	// Backing inventory item.
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	// Cached gameplay type of the hovered item.
	FGameplayTag CachedItemType { FGameplayTag::EmptyTag };
	// Whether the item uses stacks.
	bool bIsStackable { false };
	// Stack count shown on hover.
	int32 StackCount { 0 };
};
