// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Composite/INV_Leaf_LabeledValue.h"
#include "Components/TextBlock.h"
#include "UI/Utils/INV_WidgetUtils.h"

void UINV_Leaf_LabeledValue::SetText_Label(const FText& Text, bool bCollapse) const
{
	if (!IsValid(Text_Label)) return;
	if (bCollapse)
	{
		Text_Label->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Label->SetText(Text);
}

void UINV_Leaf_LabeledValue::SetText_Value(const FText& Text, bool bCollapse) const
{
	if (!IsValid(Text_Value)) return;
	if (bCollapse)
	{
		Text_Value->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Value->SetText(Text);
}

void UINV_Leaf_LabeledValue::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	UINV_WidgetUtils::SetTextBlockFontSize(Text_Label, FontSize_Label);
	UINV_WidgetUtils::SetTextBlockFontSize(Text_Value, FontSize_Value);
}
