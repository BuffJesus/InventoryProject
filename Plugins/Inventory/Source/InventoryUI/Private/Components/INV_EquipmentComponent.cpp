// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/INV_EquipmentComponent.h"
#include "Components/INV_InventoryComponent.h"
#include "EquipmentManagement/INV_EquipActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Utils/INV_InventoryStatics.h"

void UINV_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshOwningReferences();
	if (OwningPlayerController.IsValid())
	{
		InitInventoryComponent();
	}
}

void UINV_EquipmentComponent::RefreshOwningReferences()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		OwningPlayerController = nullptr;
		OwningSkeletalMesh = nullptr;
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
	else
	{
		OwningPlayerController = nullptr;
	}

	OwningSkeletalMesh = nullptr;
	if (OwningPlayerController.IsValid())
	{
		if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
		{
			OwningSkeletalMesh = OwnerCharacter->GetMesh();
		}
	}
}

void UINV_EquipmentComponent::InitInventoryComponent()
{
	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;
	
	InventoryComponent->OnItemEquipped.AddUniqueDynamic(this, &ThisClass::OnItemEquipped);
	InventoryComponent->OnItemUnequipped.AddUniqueDynamic(this, &ThisClass::OnItemUnequipped);
}

void UINV_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	AINV_EquipActor* EquippedActor { FindEquippedActor(EquipmentTypeTag) };
	if (!EquippedActor) return;
	EquippedActors.Remove(EquippedActor);
	EquippedActor->Destroy();
}

void UINV_EquipmentComponent::OnItemEquipped(UINV_InventoryItem* EquippedItem)
{
	RefreshOwningReferences();

	FINV_EquipmentFragment* EquipmentFragment { nullptr };
	if (!TryGetEquipmentFragment(EquippedItem, EquipmentFragment)) return;

	ProcessEquipmentItem(EquippedItem, true);
	if (!OwningSkeletalMesh.IsValid()) return;
	if (!OwningPlayerController.IsValid()) return;
	if (!OwningPlayerController->HasAuthority()) return;

	AINV_EquipActor* SpawnedEquipActor { SpawnEquippedActor(EquipmentFragment, OwningSkeletalMesh.Get()) };
	if (!IsValid(SpawnedEquipActor)) return;
	
	EquippedActors.Add(SpawnedEquipActor);
}

void UINV_EquipmentComponent::OnItemUnequipped(UINV_InventoryItem* UnequippedItem)
{
	RefreshOwningReferences();
	ProcessEquipmentItem(UnequippedItem, false);

	FINV_EquipmentFragment* EquipmentFragment { nullptr };
	if (!TryGetEquipmentFragment(UnequippedItem, EquipmentFragment)) return;

	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
}

bool UINV_EquipmentComponent::TryGetEquipmentFragment(UINV_InventoryItem* Item, FINV_EquipmentFragment*& OutEquipmentFragment)
{
	OutEquipmentFragment = nullptr;

	if (!IsValid(Item)) return false;

	FINV_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	FINV_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FINV_EquipmentFragment>();
	if (!EquipmentFragment) return false;

	OutEquipmentFragment = EquipmentFragment;
	return true;
}

void UINV_EquipmentComponent::ProcessEquipmentItem(UINV_InventoryItem* Item, bool bEquip)
{
	RefreshOwningReferences();

	if (!OwningPlayerController.IsValid()) return;
	if (!OwningPlayerController->HasAuthority()) return;

	FINV_EquipmentFragment* EquipmentFragment { nullptr };
	if (!TryGetEquipmentFragment(Item, EquipmentFragment)) return;

	if (bEquip) { EquipmentFragment->OnEquip(OwningPlayerController.Get()); }
	else { EquipmentFragment->OnUnequip(OwningPlayerController.Get()); }
}

AINV_EquipActor* UINV_EquipmentComponent::SpawnEquippedActor(FINV_EquipmentFragment* EquipmentFragment, USkeletalMeshComponent* AttachMesh)
{
	AINV_EquipActor* SpawnedEquipActor { EquipmentFragment->SpawnAttachedActor(AttachMesh) };
	if (!IsValid(SpawnedEquipActor)) return nullptr;

	SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
	SpawnedEquipActor->SetOwner(GetOwner());
	EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
	return SpawnedEquipActor;	
}

AINV_EquipActor* UINV_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	auto FoundActor { EquippedActors.FindByPredicate([&EquipmentTypeTag](const AINV_EquipActor* EquippedActor)
	{
		return EquippedActor->GetEquipmentType().MatchesTagExact(EquipmentTypeTag);
	}) };
	return FoundActor ? *FoundActor : nullptr;
}
