// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ItemPopUp/INV_ItemPopUp.h"
#include "Components/Button.h"
#include "Components/Slider.h"

void UINV_ItemPopUp::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Split->OnClicked.AddDynamic(this, &ThisClass::SplitButtonClicked);
	Button_Drop->OnClicked.AddDynamic(this, &ThisClass::DropButtonClicked);
	Button_Consume->OnClicked.AddDynamic(this, &ThisClass::ConsumeButtonClicked);
	Slider_Split->OnValueChanged.AddDynamic(this, &ThisClass::SliderValueChanged);
}

void UINV_ItemPopUp::SplitButtonClicked()
{
	
}

void UINV_ItemPopUp::DropButtonClicked()
{
	
}

void UINV_ItemPopUp::ConsumeButtonClicked()
{
	
}

void UINV_ItemPopUp::SliderValueChanged(float Value)
{
	
}
