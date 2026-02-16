// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Fragments/INV_ItemFragment.h"

#include "ContentBrowserItemData.h"
#include "UI/Composite/INV_Composite_Base.h"
#include "UI/Composite/INV_Leaf_Image.h"
#include "UI/Composite/INV_Leaf_LabeledValue.h"
#include "UI/Composite/INV_Leaf_Text.h"

void FINV_InventoryItemFragment::Assimilate(UINV_Composite_Base* Composite) const
{
	if (!MatchesWidgetTag(Composite)) return;
	Composite->Expand();
}

bool FINV_InventoryItemFragment::MatchesWidgetTag(const UINV_Composite_Base* Composite) const
{
	return Composite->GetFragmentTag().MatchesTagExact(GetFragmentTag());
}

void FINV_ImageFragment::Assimilate(UINV_Composite_Base* Composite) const
{
	FINV_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;
	
	UINV_Leaf_Image* Image { Cast<UINV_Leaf_Image>(Composite) };
	if (!IsValid(Image)) return;
	
	Image->SetImage(Icon);
	Image->SetBoxSize(IconDimensions);
	Image->SetImageSize(IconDimensions);
}

void FINV_TextFragment::Assimilate(UINV_Composite_Base* Composite) const
{
	FINV_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;
	
	UINV_Leaf_Text* LeafText { Cast<UINV_Leaf_Text>(Composite) };
	if (!IsValid(LeafText)) return;
	
	LeafText->SetText(FragmentText);
}

void FINV_LabeledNumberFragment::Assimilate(UINV_Composite_Base* Composite) const
{
	FINV_InventoryItemFragment::Assimilate(Composite);
	
	if (!MatchesWidgetTag(Composite)) return;
	
	UINV_Leaf_LabeledValue* LabeledValue { Cast<UINV_Leaf_LabeledValue>(Composite) };
	if (!IsValid(LabeledValue)) return;
	
	LabeledValue->SetText_Label(Text_Label, bCollapseLabel);
	
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = MinFractionalDigits;
	Options.MaximumFractionalDigits = MaxFractionalDigits;
	
	LabeledValue->SetText_Value(FText::AsNumber(Value, &Options), bCollapseValue);
}

void FINV_LabeledNumberFragment::InitializeRuntimeState()
{
	FINV_InventoryItemFragment::InitializeRuntimeState();
	
	if (!bRuntimeStateInitialized && bRandomizeOnInitialization)
	{
		Value = FMath::FRandRange(Min, Max);
	}

	bRuntimeStateInitialized = true;
}

void FINV_ConsumableFragment::OnConsume(APlayerController* PC)
{
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.OnConsume(PC);
	}
}

void FINV_ConsumableFragment::Assimilate(UINV_Composite_Base* Composite) const
{
	FINV_InventoryItemFragment::Assimilate(Composite);
	for (const auto& Modifier : ConsumeModifiers)
	{
		const auto& ModRef { Modifier.Get() };
		ModRef.Assimilate(Composite);
	}
}

void FINV_ConsumableFragment::InitializeRuntimeState()
{
	FINV_InventoryItemFragment::InitializeRuntimeState();
	
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.InitializeRuntimeState();
	}
}

void FINV_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Health Potion consumed! Restoring health by: %f"), GetValue()));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}

void FINV_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Mana Potion consumed! Restoring mana by: %f"), GetValue()));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}
