// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_ItemPopUp.generated.h"

class USizeBox;
class UTextBlock;
class USlider;
class UButton;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_ItemPopUp : public UUserWidget
{
	GENERATED_BODY()
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Split;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Drop;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_Consume;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> Slider_Split;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SplitAmount;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USizeBox> SizeBox_Root;
};
