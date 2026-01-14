// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_GridSlot.generated.h"

class UImage;

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
	FORCEINLINE void SetTileIndex(int32 Index) { TileIndex = Index; }
	FORCEINLINE int32 GetTileIndex() const { return TileIndex; }
	FORCEINLINE EINV_GridSlotState GetGridSlotState() const { return GridSlotState; }
	
	void SetUnoccupiedTexture();
	void SetOccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();
	
private:
	int32 TileIndex;
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
