// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "INV_ItemPresentationSnapshot.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemTextLine
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FText Text;
};

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemStatLine
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FText Label;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FText Value;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	bool bCollapseLabel { false };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	bool bCollapseValue { false };
};

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemPresentationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FGameplayTag ItemType { FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	TObjectPtr<UTexture2D> Icon { nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FVector2D IconDimensions { 100.f, 100.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	FIntPoint GridSize { 1, 1 };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	int32 StackCount { 0 };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	bool bStackable { false };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	bool bConsumable { false };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	bool bEquippable { false };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	TArray<FINV_ItemTextLine> TextLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Presentation")
	TArray<FINV_ItemStatLine> StatLines;

	void Reset()
	{
		DisplayName = FText::GetEmpty();
		ItemType = FGameplayTag::EmptyTag;
		ItemRarityTag = FGameplayTag::EmptyTag;
		Icon = nullptr;
		IconDimensions = FVector2D(100.f, 100.f);
		GridSize = FIntPoint(1, 1);
		StackCount = 0;
		bStackable = false;
		bConsumable = false;
		bEquippable = false;
		TextLines.Reset();
		StatLines.Reset();
	}
};
