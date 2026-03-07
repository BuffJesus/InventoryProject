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
	if (!InventoryComponent.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("EquipmentComponent::InitInventoryComponent failed. Owner=%s PC=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(OwningPlayerController.Get()));
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("EquipmentComponent::InitInventoryComponent bound delegates. Owner=%s PC=%s InventoryComponent=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OwningPlayerController.Get()),
		*GetNameSafe(InventoryComponent.Get()));
	
	InventoryComponent->OnItemEquipped.AddUniqueDynamic(this, &ThisClass::OnItemEquipped);
	InventoryComponent->OnItemUnequipped.AddUniqueDynamic(this, &ThisClass::OnItemUnequipped);
}


void UINV_EquipmentComponent::OnItemEquipped(UINV_InventoryItem* EquippedItem)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("EquipmentComponent::OnItemEquipped Owner=%s Item=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(EquippedItem));
	ProcessEquipmentItem(EquippedItem, true);
}

void UINV_EquipmentComponent::OnItemUnequipped(UINV_InventoryItem* UnequippedItem)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("EquipmentComponent::OnItemUnequipped Owner=%s Item=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(UnequippedItem));
	ProcessEquipmentItem(UnequippedItem, false);
}

void UINV_EquipmentComponent::ProcessEquipmentItem(UINV_InventoryItem* Item, bool bEquip)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::ProcessEquipmentItem aborted: invalid item."));
		return;
	}
	if (!OwningPlayerController.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::ProcessEquipmentItem aborted: invalid player controller."));
		return;
	}
	if (!OwningPlayerController->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("EquipmentComponent::ProcessEquipmentItem aborted: no authority. PC=%s"),
			*GetNameSafe(OwningPlayerController.Get()));
		return;
	}

	FINV_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	FINV_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_EquipmentFragment>();
	if (!EquipmentFragment)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("EquipmentComponent::ProcessEquipmentItem aborted: no equipment fragment. Item=%s"),
			*GetNameSafe(Item));
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("EquipmentComponent::ProcessEquipmentItem executing %s for Item=%s"),
		bEquip ? TEXT("equip") : TEXT("unequip"),
		*GetNameSafe(Item));

	if (bEquip) { EquipmentFragment->OnEquip(OwningPlayerController.Get()); }
	else { EquipmentFragment->OnUnequip(OwningPlayerController.Get()); }
}

