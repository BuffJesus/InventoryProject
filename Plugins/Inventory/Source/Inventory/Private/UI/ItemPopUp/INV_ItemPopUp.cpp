// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ItemPopUp/INV_ItemPopUp.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UINV_ItemPopUp::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Split->OnClicked.AddDynamic(this, &ThisClass::SplitButtonClicked);
	Button_Drop->OnClicked.AddDynamic(this, &ThisClass::DropButtonClicked);
	Button_Consume->OnClicked.AddDynamic(this, &ThisClass::ConsumeButtonClicked);
	Slider_Split->OnValueChanged.AddDynamic(this, &ThisClass::SliderValueChanged);
}

void UINV_ItemPopUp::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RemoveFromParent();
}

int32 UINV_ItemPopUp::GetSplitAmount() const
{
	return FMath::Floor(Slider_Split->GetValue());
}

void UINV_ItemPopUp::ExecuteAndClose(const TFunctionRef<bool()>& Action)
{
	if (Action())
	{
		RemoveFromParent();
	}
}

void UINV_ItemPopUp::CollapseSplitButton() const
{
	Button_Split->SetVisibility(ESlateVisibility::Collapsed);
	Slider_Split->SetVisibility(ESlateVisibility::Collapsed);
	Text_SplitAmount->SetVisibility(ESlateVisibility::Collapsed);
}

void UINV_ItemPopUp::CollapseConsumeButton() const
{
	Button_Consume->SetVisibility(ESlateVisibility::Collapsed);
}

void UINV_ItemPopUp::SetSliderParams(const float Max, const float Value) const
{
	Slider_Split->SetMaxValue(Max);
	Slider_Split->SetMinValue(1);
	Slider_Split->SetValue(Value);
	Text_SplitAmount->SetText(FText::AsNumber(FMath::Floor(Value)));
}

void UINV_ItemPopUp::SplitButtonClicked()
{
	ExecuteAndClose([this]() { return OnSplit.ExecuteIfBound(GetSplitAmount(), GridIndex); });
}

void UINV_ItemPopUp::DropButtonClicked()
{
	ExecuteAndClose([this]() { return OnDrop.ExecuteIfBound(GridIndex); });
}

void UINV_ItemPopUp::ConsumeButtonClicked()
{
	ExecuteAndClose([this]() { return OnConsume.ExecuteIfBound(GridIndex); });
}

void UINV_ItemPopUp::SliderValueChanged(float Value)
{
	Text_SplitAmount->SetText(FText::AsNumber(FMath::Floor(Value)));
}
