// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Fragments/INV_ItemFragment.h"

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
