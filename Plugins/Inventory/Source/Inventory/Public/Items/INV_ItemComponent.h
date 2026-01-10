// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "INV_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORY_API UINV_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_ItemComponent();
	FORCEINLINE FString GetPickupMessage() const { return PickupMessage; }
	
private:
	UPROPERTY(EditAnywhere, Category="INV|Inventory")
	FString PickupMessage { "Null Message" };
	
};
