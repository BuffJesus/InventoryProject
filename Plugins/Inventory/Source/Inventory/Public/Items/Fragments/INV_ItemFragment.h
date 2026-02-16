// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_ItemFragment.generated.h"

class UINV_Composite_Base;
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
	
	virtual void InitializeRuntimeState() {}
	
private:
	// Tag used to identify this fragment type.
	UPROPERTY(EditAnywhere, Category = "INV|Item", meta = (Categories = "FragmentTags"))
	FGameplayTag FragmentTag { FGameplayTag::EmptyTag };
};

//Item fragment specifically for assimilation into widget
USTRUCT(BlueprintType)
struct FINV_InventoryItemFragment : public FInv_ItemFragment
{
	GENERATED_BODY()
	virtual void Assimilate(UINV_Composite_Base* Composite) const;
	
protected:
	bool MatchesWidgetTag(const UINV_Composite_Base* Composite) const;
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
struct FINV_ImageFragment : public FINV_InventoryItemFragment
{
	GENERATED_BODY()
	// Icon texture used for UI.
	FORCEINLINE UTexture2D* GetIcon() const { return Icon.Get(); }
	virtual void Assimilate(UINV_Composite_Base* Composite) const override;
	
private:
	// Icon asset.
	UPROPERTY(EditAnywhere, Category = "INV|Image")
	TObjectPtr<UTexture2D> Icon { nullptr };
	
	// Optional icon size hint.
	UPROPERTY(EditAnywhere, Category = "INV|Image")
	FVector2D IconDimensions { 100.f, 100.f };
};

USTRUCT(BlueprintType)
struct FINV_TextFragment : public FINV_InventoryItemFragment
{
	GENERATED_BODY()
	FText GetText() const { return FragmentText; }
	void SetText(const FText& Text) { FragmentText = Text; }
	virtual void Assimilate(UINV_Composite_Base* Composite) const override;
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Text")
	FText FragmentText { NSLOCTEXT("Inventory", "DefaultText", "Default Text") };
};

USTRUCT(BlueprintType)
struct FINV_LabeledNumberFragment : public FINV_InventoryItemFragment
{
	GENERATED_BODY()
	virtual void Assimilate(UINV_Composite_Base* Composite) const override;
	virtual void InitializeRuntimeState() override;
	float GetValue() const { return Value; }
	
	// When runtime state is initialized, this fragment will randomize. However, once equipped and dropped,
	// an item should retain same value (do not reapply randomization)
	bool bRandomizeOnInitialization { true };
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	FText Text_Label { NSLOCTEXT("Inventory", "DefaultText", "Default Text") };
	
	UPROPERTY(VisibleAnywhere, Category = "INV|Labels")
	float Value { 0.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	float Min { 0.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	float Max { 0.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	bool bCollapseLabel { false };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	bool bCollapseValue { false };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	int32 MinFractionalDigits { 1 };
	
	UPROPERTY(EditAnywhere, Category = "INV|Labels")
	int32 MaxFractionalDigits { 1 };
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

// Consume Fragments
USTRUCT(BlueprintType)
struct FINV_ConsumeModifier : public FINV_LabeledNumberFragment
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) {}
};

USTRUCT(BlueprintType)
struct FINV_ConsumableFragment : public FINV_InventoryItemFragment
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) {}
	virtual void Assimilate(UINV_Composite_Base* Composite) const override;
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Consumable", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FINV_ConsumeModifier>> ConsumeModifiers;
};

USTRUCT(BlueprintType)
struct FINV_HealthPotionFragment : public FINV_ConsumeModifier
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) override;
};

USTRUCT(BlueprintType)
struct FINV_ManaPotionFragment : public FINV_ConsumeModifier
{
	GENERATED_BODY()
	virtual void OnConsume(APlayerController* PC) override;
};
