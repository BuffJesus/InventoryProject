// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Popup/INV_ItemPopUp.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UINV_ItemPopUp::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Split->OnClicked.AddDynamic(this, &ThisClass::SplitButtonClicked);
	Button_Drop->OnClicked.AddDynamic(this, &ThisClass::DropButtonClicked);
	Button_Consume->OnClicked.AddDynamic(this, &ThisClass::ConsumeButtonClicked);
	if (IsValid(Button_Inspect))
	{
		Button_Inspect->OnClicked.AddDynamic(this, &ThisClass::InspectButtonClicked);
	}
	Slider_Split->OnValueChanged.AddDynamic(this, &ThisClass::SliderValueChanged);
}

void UINV_ItemPopUp::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

int32 UINV_ItemPopUp::GetSplitAmount() const
{
	return FMath::Floor(Slider_Split->GetValue());
}

void UINV_ItemPopUp::FocusDefaultAction()
{
	SetIsFocusable(true);

	auto IsWidgetUsable = [](const UWidget* Widget) -> bool
	{
		if (!IsValid(Widget)) return false;
		if (Widget->GetVisibility() != ESlateVisibility::Visible) return false;
		return Widget->GetIsEnabled();
	};

	UWidget* FocusTarget = nullptr;
	if (IsWidgetUsable(Button_Split))
	{
		FocusTarget = Button_Split;
	}
	else if (IsWidgetUsable(Button_Drop))
	{
		FocusTarget = Button_Drop;
	}
	else if (IsWidgetUsable(Button_Consume))
	{
		FocusTarget = Button_Consume;
	}
	else if (IsWidgetUsable(Button_Inspect))
	{
		FocusTarget = Button_Inspect;
	}
	else if (IsWidgetUsable(Slider_Split))
	{
		FocusTarget = Slider_Split;
	}

	if (IsValid(FocusTarget))
	{
		FocusTarget->SetKeyboardFocus();
	}
	else
	{
		SetKeyboardFocus();
	}
}

void UINV_ItemPopUp::ExecuteAndClose(const TFunctionRef<bool()>& Action)
{
	if (Action())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UINV_ItemPopUp::ResetMenuState() const
{
	Button_Split->SetVisibility(ESlateVisibility::Visible);
	Slider_Split->SetVisibility(ESlateVisibility::Visible);
	Text_SplitAmount->SetVisibility(ESlateVisibility::Visible);
	Button_Drop->SetVisibility(ESlateVisibility::Visible);
	Button_Consume->SetVisibility(ESlateVisibility::Visible);
	if (IsValid(Button_Inspect))
	{
		Button_Inspect->SetVisibility(ESlateVisibility::Visible);
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

FVector2D UINV_ItemPopUp::GetBoxSize() const
{
	return IsValid(SizeBox_Root.Get())
		? FVector2D(SizeBox_Root->GetWidthOverride(), SizeBox_Root->GetHeightOverride())
		: FVector2D::ZeroVector;
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

void UINV_ItemPopUp::InspectButtonClicked()
{
	ExecuteAndClose([this]() { return OnInspect.ExecuteIfBound(GridIndex); });
}

void UINV_ItemPopUp::SliderValueChanged(float Value)
{
	Text_SplitAmount->SetText(FText::AsNumber(FMath::Floor(Value)));
}
