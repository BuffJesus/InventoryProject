// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Composite/INV_Leaf_Text.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Components/TextBlock.h"
#include "UI/Utils/INV_WidgetUtils.h"

void UINV_Leaf_Text::SetText(const FText& Text) const
{
	if (!IsValid(Text_LeafText)) return;
	Text_LeafText->SetText(Text);
}

void UINV_Leaf_Text::ApplyRarityColorFromItemRarity(const FGameplayTag& InItemRarityTag, const FGameplayTag& SourceFragmentTag) const
{
	if (!IsValid(Text_LeafText) || !bUseRarityFragmentTextColor) return;
	if (RequiredSourceFragmentTagForRarityColor.IsValid()
		&& !SourceFragmentTag.MatchesTagExact(RequiredSourceFragmentTagForRarityColor))
	{
		return;
	}

	FLinearColor ColorToApply { DefaultTextColorWhenNoRarity };

	if (InItemRarityTag.MatchesTagExact(FragmentTags::Rarity::Rarity_Common))
	{
		ColorToApply = CommonRarityTextColor;
	}
	else if (InItemRarityTag.MatchesTagExact(FragmentTags::Rarity::Rarity_Uncommon))
	{
		ColorToApply = UncommonRarityTextColor;
	}
	else if (InItemRarityTag.MatchesTagExact(FragmentTags::Rarity::Rarity_Rare))
	{
		ColorToApply = RareRarityTextColor;
	}
	else if (InItemRarityTag.MatchesTagExact(FragmentTags::Rarity::Rarity_Epic))
	{
		ColorToApply = EpicRarityTextColor;
	}
	else if (InItemRarityTag.MatchesTagExact(FragmentTags::Rarity::Rarity_Legendary))
	{
		ColorToApply = LegendaryRarityTextColor;
	}

	Text_LeafText->SetColorAndOpacity(FSlateColor(ColorToApply));
}

void UINV_Leaf_Text::NativePreConstruct()
{
	Super::NativePreConstruct();

	UINV_WidgetUtils::SetTextBlockFontSize(Text_LeafText, FontSize);
}
