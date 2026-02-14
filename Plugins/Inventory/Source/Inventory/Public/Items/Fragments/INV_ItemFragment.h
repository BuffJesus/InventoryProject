// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "INV_ItemFragment.generated.h"

class APlayerController;

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
	// Tag used to identify this fragment type.
	UPROPERTY(EditAnywhere, Category = "INV|Item", meta = (Categories = "FragmentTags"))
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };
};

USTRUCT(BlueprintType)
struct FINV_GridFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	// Grid dimensions and padding for this item.
	FIntPoint GetGridSize() const { return GridSize; }
	float GetGridPadding() const { return GridPadding; }
	void SetGridSize(FIntPoint Size) { GridSize = Size; }
	void SetGridPadding(float Padding) { GridPadding = Padding; }
	
private:
	// Tile size in the inventory grid.
	UPROPERTY(EditAnywhere, Category = "INV|Grid")
	FIntPoint GridSize { 1, 1 };
	
	// Pixel padding inside the grid slot.
	UPROPERTY(EditAnywhere, Category = "INV|Grid")
	float GridPadding { 0.f };
};

USTRUCT(BlueprintType)
struct FINV_ImageFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	// Icon texture used for UI.
	FORCEINLINE UTexture2D* GetIcon() const { return Icon.Get(); }
	
private:
	// Icon asset.
	UPROPERTY(EditAnywhere, Category = "INV|Image")
	TObjectPtr<UTexture2D> Icon { nullptr };
	
	// Optional icon size hint.
	UPROPERTY(EditAnywhere, Category = "INV|Image")
	FVector2D IconDimensions { 44.f, 44.f };
};

USTRUCT(BlueprintType)
struct FINV_StackableFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	// Stack size data for stackable items.
	int32 GetMaxStackSize() const { return MaxStackSize; }
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(int32 Count) { StackCount = Count; }
	
	
private:
	// Max stacks per slot.
	UPROPERTY(EditAnywhere, Category = "INV|Stackable")
	int32 MaxStackSize { 1 };
	
	// Current stack count in the pickup.
	UPROPERTY(EditAnywhere, Category = "INV|Stackable")
	int32 StackCount { 1 };
};

USTRUCT(BlueprintType)
struct FINV_ConsumableFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) {}
};

USTRUCT(BlueprintType)
struct FINV_HealthPotionFragment : public FINV_ConsumableFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "INV|Consumable")
	float HealAmount { 20.f };
	
	virtual void OnConsume(APlayerController* PC) override;
};

USTRUCT(BlueprintType)
struct FINV_ManaPotionFragment : public FINV_ConsumableFragment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "INV|Consumable")
	float ManaAmount { 20.f };
	
	virtual void OnConsume(APlayerController* PC) override;
};
