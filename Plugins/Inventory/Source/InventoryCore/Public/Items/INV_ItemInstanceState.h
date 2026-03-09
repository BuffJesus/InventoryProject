// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "INV_ItemInstanceState.generated.h"

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemRuntimeStatValue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|InstanceState")
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|InstanceState")
	float Value { 0.f };
};

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemInstanceState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|InstanceState")
	int32 StackCount { 0 };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|InstanceState")
	bool bEquipped { false };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|InstanceState")
	TArray<FINV_ItemRuntimeStatValue> RuntimeStatValues;

	void Reset()
	{
		StackCount = 0;
		bEquipped = false;
		RuntimeStatValues.Reset();
	}

	void AddRuntimeStatValue(const FGameplayTag& FragmentTag, const float InValue)
	{
		FINV_ItemRuntimeStatValue& RuntimeValue = RuntimeStatValues.AddDefaulted_GetRef();
		RuntimeValue.FragmentTag = FragmentTag;
		RuntimeValue.Value = InValue;
	}
};
