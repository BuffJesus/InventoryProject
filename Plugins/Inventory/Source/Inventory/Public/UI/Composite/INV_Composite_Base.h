// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "INV_Composite_Base.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_Composite_Base : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE FGameplayTag GetFragmentTag() const { return FragmentTag; }
	FORCEINLINE void SetFragmentTag(FGameplayTag Tag) { FragmentTag = Tag; }
	FORCEINLINE const FGameplayTag& GetItemRarityTag() const { return ItemRarityTag; }
	FORCEINLINE void SetItemRarityTag(const FGameplayTag& Tag) { ItemRarityTag = Tag; }
	virtual void Collapse();
	void Expand();
	
	using FuncType = TFunction<void(UINV_Composite_Base*)>;
	virtual void ApplyFunction(FuncType Function);
	
private:
	UPROPERTY(EditAnywhere, Category = "INV|Composite", meta = (Categories = "FragmentTags"))
	FGameplayTag FragmentTag;

	UPROPERTY(Transient)
	FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };
	
};
