// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"

UINV_InventoryComponent* UINV_InventoryStatics::GetInventoryComponent(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return nullptr;
	UINV_InventoryComponent* InventoryComponent { PlayerController->FindComponentByClass<UINV_InventoryComponent>() };
	return InventoryComponent;
}
