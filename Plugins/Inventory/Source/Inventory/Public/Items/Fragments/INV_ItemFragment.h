// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "INV_ItemFragment.generated.h"

USTRUCT(BlueprintType)
struct FInv_ItemFragment
{
	GENERATED_BODY()
	
	FInv_ItemFragment() {}
	FInv_ItemFragment(const FInv_ItemFragment&) = default;
	FInv_ItemFragment& operator=(const FInv_ItemFragment&) = default;
	FInv_ItemFragment(FInv_ItemFragment&&) = default;
	FInv_ItemFragment& operator=(FInv_ItemFragment&&) = default;
	virtual ~FInv_ItemFragment() {}
	
	FORCEINLINE FGameplayTag GetFragmentTag() const { return FragmentTag; }
	FORCEINLINE void SetFragmentTag(FGameplayTag Tag) { FragmentTag = Tag; }
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Inventory", meta = (Categories = "FragmentTags"))
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };
};

USTRUCT(BlueprintType)
struct FINV_GridFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	FIntPoint GetGridSize() const { return GridSize; }
	float GetGridPadding() const { return GridPadding; }
	void SetGridSize(FIntPoint Size) { GridSize = Size; }
	void SetGridPadding(float Padding) { GridPadding = Padding; }
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	FIntPoint GridSize { 1, 1 };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float GridPadding { 0.f };
};