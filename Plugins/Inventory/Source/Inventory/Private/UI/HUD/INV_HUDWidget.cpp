// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/INV_HUDWidget.h"
#include "Components/INV_InventoryComponent.h"
#include "UI/Utils/INV_InventoryStatics.h"
#include "UI/HUD/INV_InfoMessage.h"

void UINV_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// Bind to inventory events.
	UINV_InventoryComponent* InventoryComponent { UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer()) };
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnNoRoomInInventory.AddDynamic(this, &ThisClass::OnNoRoom);
	}
}

void UINV_HUDWidget::OnNoRoom()
{
	// Show a temporary warning message.
	if (!IsValid(InfoMessage)) { return; }
	InfoMessage->SetMessage(FText::FromString("No room in inventory!"));
}
