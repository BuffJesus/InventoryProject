// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/HUD/INV_HUDWidget.h"
#include "UI/Widgets/HUD/INV_InfoMessage.h"

void UINV_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UINV_HUDWidget::OnNoRoom()
{
	// Show a temporary warning message.
	if (!IsValid(InfoMessage)) { return; }
	InfoMessage->SetMessage(FText::FromString("No room in inventory!"));
}
