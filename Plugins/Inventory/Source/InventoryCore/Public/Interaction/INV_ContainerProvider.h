// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INV_ContainerProvider.generated.h"

class UINV_ContainerComponent;

UINTERFACE(BlueprintType)
class INVENTORYCORE_API UINV_ContainerProvider : public UInterface
{
	GENERATED_BODY()
};

class INVENTORYCORE_API IINV_ContainerProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "INV|Container")
	UINV_ContainerComponent* GetContainerComponent() const;
};
