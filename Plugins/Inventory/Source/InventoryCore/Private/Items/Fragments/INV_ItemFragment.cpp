// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Fragments/INV_ItemFragment.h"
#include "EquipmentManagement/INV_EquipActor.h"

namespace
{
void NotifyNoOpEquipModifier(const TCHAR* ActionText)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Orange,
			FString::Printf(TEXT("Equipment modifier %s but uses the base FINV_EquipModifier type."), ActionText));
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Equipment modifier %s, but the configured struct is the base FINV_EquipModifier type. Use a derived modifier struct instead."),
		ActionText);
}
}

void FINV_LabeledNumberFragment::InitializeRuntimeState()
{
	FINV_InventoryItemFragment::InitializeRuntimeState();
	
	// Only roll once so dropped/re-picked items keep their existing value.
	if (!bRuntimeStateInitialized && bRandomizeOnInitialization)
	{
		Value = FMath::FRandRange(Min, Max);
	}

	bRuntimeStateInitialized = true;
}

void FINV_ConsumableFragment::OnConsume(APlayerController* PC)
{
	// Forward consume behavior to each configured modifier.
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.OnConsume(PC);
	}
}

void FINV_ConsumableFragment::InitializeRuntimeState()
{
	FINV_InventoryItemFragment::InitializeRuntimeState();
	
	for (auto& Modifier : ConsumeModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.InitializeRuntimeState();
	}
}

void FINV_EquipModifier::OnEquip(APlayerController* PC)
{
	NotifyNoOpEquipModifier(TEXT("equipped"));
}

void FINV_EquipModifier::OnUnequip(APlayerController* PC)
{
	NotifyNoOpEquipModifier(TEXT("unequipped"));
}

void FINV_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Health Potion consumed! Restoring health by: %f"), GetValue()));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}

void FINV_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Mana Potion consumed! Restoring mana by: %f"), GetValue()));
	
	// Apply gameplay effect maybe? Decide per project. Broadcast maybe?
}

void FINV_AttributeModifier::OnEquip(APlayerController* PC)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Attribute modifier equipped! Modifying attribute by: %f"), GetValue()));
	}
}

void FINV_AttributeModifier::OnUnequip(APlayerController* PC)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Attribute modifier unequipped! Reverting attribute change by: %f"), GetValue()));
	}
}

void FINV_EquipmentFragment::InitializeRuntimeState()
{
	FINV_InventoryItemFragment::InitializeRuntimeState();

	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.InitializeRuntimeState();
	}
}

void FINV_EquipmentFragment::OnEquip(APlayerController* PC)
{
	if (bEquipped) return;
	bEquipped = true;
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.OnEquip(PC);
	}
}

void FINV_EquipmentFragment::OnUnequip(APlayerController* PC)
{
	if (!bEquipped) return;
	bEquipped = false;
	for (auto& Modifier : EquipModifiers)
	{
		auto& ModRef { Modifier.GetMutable() };
		ModRef.OnUnequip(PC);
	}
}

AINV_EquipActor* FINV_EquipmentFragment::SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const
{
	if (!IsValid(EquipActorClass) || !IsValid(AttachMesh)) return nullptr;
	
	AINV_EquipActor* SpawnedActor { AttachMesh->GetWorld()->SpawnActor<AINV_EquipActor>(EquipActorClass) };
	if (!IsValid(SpawnedActor)) return nullptr;
	
	SpawnedActor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketAttachPoint);
	
	return SpawnedActor;
}

void FINV_EquipmentFragment::DestroyAttachedActor() const
{
	if (EquippedActor.IsValid())
	{
		EquippedActor->Destroy();
	}
}
