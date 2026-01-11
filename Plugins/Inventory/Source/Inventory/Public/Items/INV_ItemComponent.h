// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Manifest/INV_ItemManifest.h"
#include "INV_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UINV_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_ItemComponent();
	FORCEINLINE FString GetPickupMessage() const { return PickupMessage; }
	FINV_ItemManifest GetItemManifest() const { return ItemManifest; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
	UPROPERTY(Replicated, EditAnywhere, Category="INV|Inventory")
	FINV_ItemManifest ItemManifest;
	
	UPROPERTY(EditAnywhere, Category="INV|Inventory")
	FString PickupMessage { TEXT("Null Message") };
	
};
