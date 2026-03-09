// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "INV_ProxyMesh.generated.h"

class UINV_EquipmentComponent;

UCLASS()
class INVENTORYCORE_API AINV_ProxyMesh : public AActor
{
	GENERATED_BODY()

public:
	AINV_ProxyMesh();

protected:
	virtual void BeginPlay() override;

private:
	// This is the mesh on the player controlled character
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UINV_EquipmentComponent> EquipmentComponent;
	
	// This is the proxy mesh that we will see in the inventory menu
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;
	
	FTimerHandle TimerForNextTick;
	
	void DelayedInitializeOwner();
	void DelayedInitialization();
};
