// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "INV_EquipmentComponent.generated.h"

struct FINV_EquipmentFragment;
class AINV_EquipActor;
class UINV_InventoryItem;
class UINV_InventoryComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYUI_API UINV_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void InitializeEquipmentContext(APlayerController* PlayerController, USkeletalMeshComponent* AttachMesh, bool bProxy);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	TWeakObjectPtr<APlayerController> OwningPlayerController { nullptr };
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh { nullptr };
	
	void ResolveOwnerContext();
	void HandleEquipmentItem(UINV_InventoryItem* Item, bool bEquip);
	UFUNCTION() void OnItemEquipped(UINV_InventoryItem* EquippedItem);
	UFUNCTION() void OnItemUnequipped(UINV_InventoryItem* UnequippedItem);
	bool TryGetEquipmentFragment(UINV_InventoryItem* Item, FINV_EquipmentFragment*& OutEquipmentFragment);
	void ApplyGameplayEquipState(FINV_EquipmentFragment* EquipmentFragment, bool bEquip);
	void ApplyVisualEquipState(FINV_EquipmentFragment* EquipmentFragment, bool bEquip);
	bool ShouldProcessGameplayState() const;
	bool ShouldProcessVisualState() const;
	void InitInventoryComponent();
	void UnbindInventoryComponent();
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);
	
	AINV_EquipActor* SpawnEquippedActor(FINV_EquipmentFragment* EquipmentFragment);
	AINV_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag) const;
	
	UPROPERTY() TMap<FGameplayTag, TObjectPtr<AINV_EquipActor>> EquippedActors;
	
	bool bIsProxy { false };
};
