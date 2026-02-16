// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Composite/INV_Leaf_Text.h"
#include "Components/TextBlock.h"

void UINV_Leaf_Text::SetText(const FText& Text) const
{
	if (!IsValid(Text_LeafText)) return;
	Text_LeafText->SetText(Text);
}

void UINV_Leaf_Text::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!IsValid(Text_LeafText)) return;
	
	FSlateFontInfo FontInfo = Text_LeafText->GetFont();
	FontInfo.Size = FontSize;
	Text_LeafText->SetFont(FontInfo);
}
