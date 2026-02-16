// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Composite/INV_Leaf_Text.h"
#include "Components/TextBlock.h"
#include "UI/INV_WidgetUtils.h"

void UINV_Leaf_Text::SetText(const FText& Text) const
{
	if (!IsValid(Text_LeafText)) return;
	Text_LeafText->SetText(Text);
}

void UINV_Leaf_Text::NativePreConstruct()
{
	Super::NativePreConstruct();

	UINV_WidgetUtils::SetTextBlockFontSize(Text_LeafText, FontSize);
}
