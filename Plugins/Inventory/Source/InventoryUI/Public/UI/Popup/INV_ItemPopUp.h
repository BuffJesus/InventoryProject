// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_ItemPopUp.generated.h"

class USizeBox;
class UTextBlock;
class USlider;
class UButton;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPopUpMenuSplit, int32, SplitAmount, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuDrop, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuConsume, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuInspect, int32, Index);

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_ItemPopUp : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
	int32 GetSplitAmount() const;
	int32 GetGridIndex() const { return GridIndex; }
	
	FPopUpMenuSplit OnSplit;
	FPopUpMenuDrop OnDrop;
	FPopUpMenuConsume OnConsume;
	FPopUpMenuInspect OnInspect;
	
	void ExecuteAndClose(const TFunctionRef<bool()>& Action);
	void ResetMenuState() const;
	void CollapseSplitButton() const;
	void CollapseConsumeButton() const;
	void SetSliderParams(const float Max, const float Value) const;
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	void FocusDefaultAction();
	
	FVector2D GetBoxSize() const;
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Split;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Drop;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Consume;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Inspect;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> Slider_Split;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SplitAmount;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USizeBox> SizeBox_Root;
	
	int32 GridIndex { INDEX_NONE };
	bool bUpdatingSliderFromSnap { false };
	
	UFUNCTION() void SplitButtonClicked();
	UFUNCTION() void DropButtonClicked();
	UFUNCTION() void ConsumeButtonClicked();
	UFUNCTION() void InspectButtonClicked();
	UFUNCTION() void SliderValueChanged(float Value);
};
