// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/INV_HUDWidget.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "UI/HUD/INV_InfoMessage.h"

void UINV_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	UINV_InventoryComponent* InventoryComponent { UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer()) };
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnNoRoomInInventory.AddDynamic(this, &ThisClass::OnNoRoom);
	}
}

void UINV_HUDWidget::OnNoRoom()
{
	if (!IsValid(InfoMessage)) { return; }
	InfoMessage->SetMessage(FText::FromString("No room in inventory!"));
}
