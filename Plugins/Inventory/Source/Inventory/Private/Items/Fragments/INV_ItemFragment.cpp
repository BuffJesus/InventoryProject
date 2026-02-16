// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Composite/INV_Composite_Base.h"
#include "UI/Composite/INV_Leaf_Image.h"

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

void FINV_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Health Potion consumed!"));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}

void FINV_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Mana Potion consumed!"));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}
