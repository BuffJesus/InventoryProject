// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "UI/Inventory/Base/INV_InventoryBase.h"

UINV_InventoryComponent::UINV_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UINV_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructInventory();
}

void UINV_InventoryComponent::ToggleInventoryMenu()
{
	bInventoryMenuOpen ? HandleInventoryMenu(ESlateVisibility::Collapsed, false)
		: HandleInventoryMenu(ESlateVisibility::Visible, true);
}

void UINV_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("OwningController cannot be null"));
	if (!OwningController->IsLocalController()) return;
	
	Inventory = CreateWidget<UINV_InventoryBase>(OwningController.Get(), InventoryClass);
	checkf(Inventory, TEXT("Inventory cannot be null"));
	Inventory->AddToViewport();
}

void UINV_InventoryComponent::HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen)
{
	if (!IsValid(Inventory)) return;
	
	Inventory->SetVisibility(Visibility);
	bInventoryMenuOpen = bIsOpen;
	
	if (!OwningController.IsValid()) return;
	
	bIsOpen ? OwningController->SetInputMode(FInputModeGameAndUI()) 
			: OwningController->SetInputMode(FInputModeGameOnly());
	
	OwningController->SetShowMouseCursor(bIsOpen);
}
