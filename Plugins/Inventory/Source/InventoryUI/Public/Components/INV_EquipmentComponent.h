// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "INV_EquipmentComponent.generated.h"

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
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwnerSkeletalMesh;
	
	UFUNCTION() void OnItemEquipped(UINV_InventoryItem* EquippedItem);
	UFUNCTION() void OnItemUnequipped(UINV_InventoryItem* UnequippedItem);
	void ProcessEquipmentItem(UINV_InventoryItem* Item, bool bEquip);
	
	void InitInventoryComponent();
};
