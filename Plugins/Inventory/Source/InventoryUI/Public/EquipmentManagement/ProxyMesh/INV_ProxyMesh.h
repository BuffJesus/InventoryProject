// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "INV_ProxyMesh.generated.h"

class UINV_EquipmentComponent;
class UINV_InventoryItem;

UCLASS()
class INVENTORYUI_API AINV_ProxyMesh : public AActor
{
	GENERATED_BODY()

public:
	AINV_ProxyMesh();
	UFUNCTION(BlueprintCallable, Category = "INV|Proxy")
	void InitializeProxy(APlayerController* PlayerController);
	void SetPreviewVisible(bool bVisible);
	void PreviewEquipItem(UINV_InventoryItem* Item);
	void PreviewUnequipItem(UINV_InventoryItem* Item);
	
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// This is the mesh on the player controlled character
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
	TWeakObjectPtr<APlayerController> ObservedPlayerController;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UINV_EquipmentComponent> EquipmentComponent;

	// This is the proxy mesh that we will see in the inventory menu
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	FTimerHandle TimerForNextTick;
	int32 InitializationAttempts { 0 };
	int32 MaxInitializationAttempts { 30 };

	void AttemptInitialization();
	void ScheduleInitializationRetry();
	bool TryInitializeFromPlayerController(APlayerController* PlayerController);
	bool HasAssignedTarget() const;
};
