// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/INV_EquipmentComponent.h"
#include "Components/INV_InventoryComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Utils/INV_InventoryStatics.h"

void UINV_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(OwnerActor))
	{
		OwningPlayerController = PlayerController;
	}
	else if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		OwningPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	}

	if (OwningPlayerController.IsValid())
	{
		if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
		{
			OwnerSkeletalMesh = OwnerCharacter->GetMesh();
		}

		InitInventoryComponent();
	}
}

void UINV_EquipmentComponent::InitInventoryComponent()
{
	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;
	
	InventoryComponent->OnItemEquipped.AddUniqueDynamic(this, &ThisClass::OnItemEquipped);
	InventoryComponent->OnItemUnequipped.AddUniqueDynamic(this, &ThisClass::OnItemUnequipped);
}


void UINV_EquipmentComponent::OnItemEquipped(UINV_InventoryItem* EquippedItem)
{
	ProcessEquipmentItem(EquippedItem, true);
}

void UINV_EquipmentComponent::OnItemUnequipped(UINV_InventoryItem* UnequippedItem)
{
	ProcessEquipmentItem(UnequippedItem, false);
}

void UINV_EquipmentComponent::ProcessEquipmentItem(UINV_InventoryItem* Item, bool bEquip)
{
	if (!IsValid(Item)) return;
	if (!OwningPlayerController.IsValid()) return;
	if (!OwningPlayerController->HasAuthority()) return;

	FINV_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	FINV_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_EquipmentFragment>();
	if (!EquipmentFragment) return;

	if (bEquip) { EquipmentFragment->OnEquip(OwningPlayerController.Get()); }
	else { EquipmentFragment->OnUnequip(OwningPlayerController.Get()); }
}
