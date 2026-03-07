// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "INV_EquipmentComponent.generated.h"

struct FINV_ItemManifest;
struct FINV_EquipmentFragment;
class AINV_EquipActor;
class UINV_InventoryItem;
class UINV_InventoryComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYUI_API UINV_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	TWeakObjectPtr<APlayerController> OwningPlayerController { nullptr };
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh { nullptr };
	
	UFUNCTION() void OnItemEquipped(UINV_InventoryItem* EquippedItem);
	UFUNCTION() void OnItemUnequipped(UINV_InventoryItem* UnequippedItem);
	bool TryGetEquipmentFragment(UINV_InventoryItem* Item, FINV_EquipmentFragment*& OutEquipmentFragment);
	void ProcessEquipmentItem(UINV_InventoryItem* Item, bool bEquip);
	void InitInventoryComponent();
	
	AINV_EquipActor* SpawnEquippedActor(FINV_EquipmentFragment* EquipmentFragment, USkeletalMeshComponent* AttachMesh);
	
	UPROPERTY() TArray<TObjectPtr<AINV_EquipActor>> EquippedActors;
};
