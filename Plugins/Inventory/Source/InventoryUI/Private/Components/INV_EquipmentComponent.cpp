// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/INV_EquipmentComponent.h"
#include "Components/INV_InventoryComponent.h"
#include "EquipmentManagement/INV_EquipActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Utils/INV_InventoryStatics.h"

void UINV_EquipmentComponent::InitializeEquipmentContext(
	APlayerController* PlayerController,
	USkeletalMeshComponent* AttachMesh,
	const bool bProxy)
{
	OwningPlayerController = PlayerController;
	OwningSkeletalMesh = AttachMesh;
	bIsProxy = bProxy;
	InitInventoryComponent();
}

void UINV_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveOwnerContext();
	if (OwningPlayerController.IsValid())
	{
		InitInventoryComponent();
	}
}

void UINV_EquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInventoryComponent();
	Super::EndPlay(EndPlayReason);
}

void UINV_EquipmentComponent::ResolveOwnerContext()
{
	if (bIsProxy)
	{
		return;
	}

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
	if (!OwningPlayerController.IsValid()) return;

	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;
	
	InventoryComponent->OnItemEquipped.AddUniqueDynamic(this, &ThisClass::OnItemEquipped);
	InventoryComponent->OnItemUnequipped.AddUniqueDynamic(this, &ThisClass::OnItemUnequipped);
}

void UINV_EquipmentComponent::UnbindInventoryComponent()
{
	if (!InventoryComponent.IsValid()) return;

	InventoryComponent->OnItemEquipped.RemoveDynamic(this, &ThisClass::OnItemEquipped);
	InventoryComponent->OnItemUnequipped.RemoveDynamic(this, &ThisClass::OnItemUnequipped);
}

void UINV_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	AINV_EquipActor* EquippedActor { FindEquippedActor(EquipmentTypeTag) };
	if (!EquippedActor) return;

	EquippedActors.Remove(EquipmentTypeTag);
	EquippedActor->Destroy();
}

void UINV_EquipmentComponent::OnItemEquipped(UINV_InventoryItem* EquippedItem)
{
	HandleEquipmentItem(EquippedItem, true);
}

void UINV_EquipmentComponent::OnItemUnequipped(UINV_InventoryItem* UnequippedItem)
{
	HandleEquipmentItem(UnequippedItem, false);
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

void UINV_EquipmentComponent::HandleEquipmentItem(UINV_InventoryItem* Item, const bool bEquip)
{
	ResolveOwnerContext();

	FINV_EquipmentFragment* EquipmentFragment { nullptr };
	if (!TryGetEquipmentFragment(Item, EquipmentFragment)) return;

	ApplyGameplayEquipState(EquipmentFragment, bEquip);
	ApplyVisualEquipState(EquipmentFragment, bEquip);
}

void UINV_EquipmentComponent::ApplyGameplayEquipState(FINV_EquipmentFragment* EquipmentFragment, const bool bEquip)
{
	if (!ShouldProcessGameplayState()) return;
	if (!EquipmentFragment) return;

	if (bEquip) { EquipmentFragment->OnEquip(OwningPlayerController.Get()); }
	else { EquipmentFragment->OnUnequip(OwningPlayerController.Get()); }
}

void UINV_EquipmentComponent::ApplyVisualEquipState(FINV_EquipmentFragment* EquipmentFragment, const bool bEquip)
{
	if (!ShouldProcessVisualState()) return;
	if (!EquipmentFragment) return;

	const FGameplayTag EquipmentTypeTag = EquipmentFragment->GetEquipmentType();
	if (!bEquip)
	{
		RemoveEquippedActor(EquipmentTypeTag);
		return;
	}

	RemoveEquippedActor(EquipmentTypeTag);
	AINV_EquipActor* SpawnedEquipActor { SpawnEquippedActor(EquipmentFragment) };
	if (!IsValid(SpawnedEquipActor)) return;

	EquippedActors.Add(EquipmentTypeTag, SpawnedEquipActor);
}

bool UINV_EquipmentComponent::ShouldProcessGameplayState() const
{
	return !bIsProxy && OwningPlayerController.IsValid() && OwningPlayerController->HasAuthority();
}

bool UINV_EquipmentComponent::ShouldProcessVisualState() const
{
	if (!OwningSkeletalMesh.IsValid()) return false;
	if (bIsProxy) return true;

	return OwningPlayerController.IsValid() && OwningPlayerController->HasAuthority();
}

AINV_EquipActor* UINV_EquipmentComponent::SpawnEquippedActor(FINV_EquipmentFragment* EquipmentFragment)
{
	if (!EquipmentFragment) return nullptr;
	if (!OwningSkeletalMesh.IsValid()) return nullptr;

	AINV_EquipActor* SpawnedEquipActor { EquipmentFragment->SpawnAttachedActor(OwningSkeletalMesh.Get()) };
	if (!IsValid(SpawnedEquipActor)) return nullptr;

	SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
	SpawnedEquipActor->SetOwner(GetOwner());
	EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
	return SpawnedEquipActor;	
}

AINV_EquipActor* UINV_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag) const
{
	const TObjectPtr<AINV_EquipActor>* FoundActor = EquippedActors.Find(EquipmentTypeTag);
	return FoundActor ? FoundActor->Get() : nullptr;
}
