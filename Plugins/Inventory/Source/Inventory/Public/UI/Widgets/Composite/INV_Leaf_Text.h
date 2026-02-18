// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "INV_Leaf.h"
#include "INV_Leaf_Text.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_Leaf_Text : public UINV_Leaf
{
	GENERATED_BODY()
	
public:
	void SetText(const FText& Text) const;
	void ApplyRarityColorFromItemRarity(const FGameplayTag& InItemRarityTag, const FGameplayTag& SourceFragmentTag) const;
	virtual void NativePreConstruct() override;
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_LeafText { nullptr };
	
	UPROPERTY(EditAnywhere, Category="INV|Text", meta = (ClampMin = 8, ClampMax = 100))
	int32 FontSize { 12 };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity")
	bool bUseRarityFragmentTextColor { false };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (Categories = "FragmentTags", EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FGameplayTag RequiredSourceFragmentTagForRarityColor { FGameplayTag::EmptyTag };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor CommonRarityTextColor { FLinearColor(0.6f, 0.6f, 0.6f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor UncommonRarityTextColor { FLinearColor(0.2f, 0.9f, 0.2f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor RareRarityTextColor { FLinearColor(0.2f, 0.5f, 1.f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor EpicRarityTextColor { FLinearColor(0.7f, 0.3f, 1.f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor LegendaryRarityTextColor { FLinearColor(1.f, 0.55f, 0.1f, 1.f) };

	UPROPERTY(EditAnywhere, Category="INV|Text|Rarity", meta = (EditCondition = "bUseRarityFragmentTextColor", EditConditionHides))
	FLinearColor DefaultTextColorWhenNoRarity { FLinearColor::White };
};
