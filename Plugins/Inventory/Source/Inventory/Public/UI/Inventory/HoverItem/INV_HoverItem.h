// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "INV_HoverItem.generated.h"

class UTextBlock;
class UINV_InventoryItem;
class UImage;

/**
 * The HoverItem is the item that will appear and follow mouse when inventory item has been clicked
 */
UCLASS()
class INVENTORY_API UINV_HoverItem : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(const int32 Count) const;
	
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
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;	
	
private:
	void UpdatePosition();
    
	FTimerHandle PositionTimerHandle;
    
	UPROPERTY(EditDefaultsOnly, Category = "Hover Item")
	float PositionUpdateRate = 0.016f; // ~60fps
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_StackCount;
	
	int32 PreviousGridIndex { INDEX_NONE };
	FIntPoint GridDimensions;
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	bool bIsStackable { false };
	int32 StackCount { 0 };
};
