// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/INV_InventoryItem.h"
#include "UI/ItemPopUp/INV_ItemPopUp.h"
#include "INV_GridSlot.generated.h"

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
	// Hover/click events for the grid.
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
	FORCEINLINE UINV_ItemPopUp* GetItemPopUp() const { return ItemPopUp.Get(); }
	void SetItemPopUp(UINV_ItemPopUp* PopUp);
	
	// Updates slot visuals.
	void SetUnoccupiedTexture();
	void SetOccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();
	
	FGridSlotEvent GridSlotClicked;
	FGridSlotEvent GridSlotHovered;
	FGridSlotEvent GridSlotUnhovered;
	
private:
	// Index in the grid array.
	int32 TileIndex { INDEX_NONE };
	// Stack count at this slot.
	int32 StackCount { 0 };
	// Upper-left index for multi-tile items.
	int32 UpperLeftIndex { INDEX_NONE };
	// Item currently in this slot.
	TWeakObjectPtr<UINV_InventoryItem> InventoryItem;
	TWeakObjectPtr<UINV_ItemPopUp> ItemPopUp;
	// If false, slot is occupied.
	bool bAvailable { true };
	
	// Image widget used for slot visuals.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_GridSlot;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Unoccupied;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Occupied;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_Selected;
	
	UPROPERTY(EditAnywhere, Category="INV|Grid")
	FSlateBrush Brush_GrayedOut;

	// When enabled, occupied slot tint is derived from item rarity.
	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity")
	bool bUseRarityTintForOccupiedTexture { false };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor CommonRarityOccupiedTint { FLinearColor(0.6f, 0.6f, 0.6f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor UncommonRarityOccupiedTint { FLinearColor(0.2f, 0.9f, 0.2f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor RareRarityOccupiedTint { FLinearColor(0.2f, 0.5f, 1.f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor EpicRarityOccupiedTint { FLinearColor(0.7f, 0.3f, 1.f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor LegendaryRarityOccupiedTint { FLinearColor(1.f, 0.55f, 0.1f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	FLinearColor DefaultOccupiedTintWhenNoRarity { FLinearColor::White };

	// Multiplies occupied rarity tint intensity to make rarity more/less noticeable.
	UPROPERTY(EditAnywhere, Category="INV|Grid|Rarity", meta = (ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "3.0", EditCondition = "bUseRarityTintForOccupiedTexture", EditConditionHides))
	float OccupiedRarityTintStrength { 1.5f };
	
	EINV_GridSlotState GridSlotState { EINV_GridSlotState::Unoccupied };
	
	FLinearColor ResolveOccupiedTintFromItemRarity() const;

	UFUNCTION() void OnItemPopUpDestruct(UUserWidget* Menu);
};
