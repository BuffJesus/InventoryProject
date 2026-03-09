// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "INV_ContainerTypes.generated.h"

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ContainerMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container")
	FText DisplayName { FText::FromString(TEXT("Container")) };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container")
	FGameplayTag ContainerTag { FGameplayTag::EmptyTag };
};

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ContainerStarterItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container")
	FINV_ItemManifest ItemManifest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container", meta = (ClampMin = "1", UIMin = "1"))
	int32 StackCount { 1 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container")
	bool bUseItemRarity { false };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "INV|Container", meta = (Categories = "FragmentTags.Rarity", EditCondition = "bUseItemRarity", EditConditionHides))
	FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };
};
