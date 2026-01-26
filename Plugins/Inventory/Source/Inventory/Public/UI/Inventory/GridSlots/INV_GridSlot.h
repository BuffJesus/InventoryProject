// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_GridSlot.generated.h"

class UINV_InventoryItem;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGridSlotEvent, int32, GridIndex, const FPointerEvent&, MouseEvent);

UENUM(BlueprintType)
enum class EINV_GridSlotState : uint8
{
	Unoccupied,
	Occupied,
	Selected,
	GrayedOut
};

UCLASS()
class INVENTORY_API UINV_GridSlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	FORCEINLINE void SetTileIndex(int32 Index) { TileIndex = Index; }
	FORCEINLINE int32 GetTileIndex() const { return TileIndex; }
	FORCEINLINE EINV_GridSlotState GetGridSlotState() const { return GridSlotState; }
	FORCEINLINE TWeakObjectPtr<UINV_InventoryItem> GetInventoryItem() const { return InventoryItem; }
	FORCEINLINE void SetInventoryItem(UINV_InventoryItem* Item) { InventoryItem = Item; }
	FORCEINLINE int32 GetUpperLeftIndex() const { return UpperLeftIndex; }
	FORCEINLINE void SetUpperLeftIndex(int32 Index) { UpperLeftIndex = Index; }
	FORCEINLINE int32 GetStackCount() const { return StackCount; }
	FORCEINLINE void SetStackCount(int32 Count) { StackCount = Count; }
	FORCEINLINE bool GetAvailability() const { return bAvailable; }
	FORCEINLINE void SetAvailability(bool bIsAvailable) { bAvailable = bIsAvailable; }
	
	void SetUnoccupiedTexture();
	void SetOccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();
	
	FGridSlotEvent GridSlotClicked;
	FGridSlotEvent GridSlotHovered;
	FGridSlotEvent GridSlotUnhovered;
	
private:
	int32 TileIndex { INDEX_NONE };
	int32 StackCount { 0 };
	int32 UpperLeftIndex { INDEX_NONE };
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	bool bAvailable { true };
	
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_GridSlot;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Unoccupied;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Occupied;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Selected;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_GrayedOut;
	
	EINV_GridSlotState GridSlotState { EINV_GridSlotState::Unoccupied };
	
	
};
