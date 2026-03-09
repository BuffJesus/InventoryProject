#include "UI/Utils/INV_ItemPresentationUtils.h"

#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemPresentationSnapshot.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UI/Widgets/Composite/INV_Composite_Base.h"
#include "UI/Widgets/Composite/INV_Leaf_Image.h"
#include "UI/Widgets/Composite/INV_Leaf_LabeledValue.h"
#include "UI/Widgets/Composite/INV_Leaf_Text.h"

namespace
{
template <typename ApplyFn>
void ApplyToMatchingWidgets(const FGameplayTag& FragmentTag, UINV_Composite_Base* Composite, ApplyFn&& Apply)
{
	if (!IsValid(Composite)) return;

	Composite->ApplyFunction([&FragmentTag, &Apply](UINV_Composite_Base* Widget)
	{
		if (!IsValid(Widget)) return;
		if (!Widget->GetFragmentTag().MatchesTagExact(FragmentTag)) return;
		Widget->Expand();
		Apply(Widget);
	});
}

template <typename FragmentType, typename ApplyFn>
void ApplyFragmentToMatchingWidgets(const FragmentType& Fragment, UINV_Composite_Base* Composite, ApplyFn&& Apply)
{
	ApplyToMatchingWidgets(Fragment.GetFragmentTag(), Composite, Forward<ApplyFn>(Apply));
}

void ApplyStatLine(const FINV_ItemStatLine& StatLine, UINV_Composite_Base* Composite)
{
	ApplyToMatchingWidgets(StatLine.FragmentTag, Composite, [&StatLine](UINV_Composite_Base* Widget)
	{
		UINV_Leaf_LabeledValue* LabeledValue = Cast<UINV_Leaf_LabeledValue>(Widget);
		if (!IsValid(LabeledValue)) return;

		LabeledValue->SetText_Label(StatLine.Label, StatLine.bCollapseLabel);
		LabeledValue->SetText_Value(StatLine.Value, StatLine.bCollapseValue);
	});
}
}

void FINV_ItemPresentationUtils::AssimilateInventoryFragments(const FINV_ItemManifest& Manifest, UINV_Composite_Base* Composite)
{
	if (!IsValid(Composite)) return;

	Manifest.ForEachFragment<FINV_ImageFragment>([Composite](const FINV_ImageFragment& Fragment)
	{
		ApplyFragmentToMatchingWidgets(Fragment, Composite, [&Fragment](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Image* Image = Cast<UINV_Leaf_Image>(Widget);
			if (!IsValid(Image)) return;
			Image->SetImage(Fragment.GetIcon());
			Image->SetBoxSize(Fragment.GetIconDimensions());
			Image->SetImageSize(Fragment.GetIconDimensions());
		});
	});

	Manifest.ForEachFragment<FINV_TextFragment>([Composite](const FINV_TextFragment& Fragment)
	{
		ApplyFragmentToMatchingWidgets(Fragment, Composite, [&Fragment](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Text* LeafText = Cast<UINV_Leaf_Text>(Widget);
			if (!IsValid(LeafText)) return;
			LeafText->SetText(Fragment.GetText());
			LeafText->ApplyRarityColorFromItemRarity(Widget->GetItemRarityTag(), Fragment.GetFragmentTag());
		});
	});

	Manifest.ForEachFragment<FINV_LabeledNumberFragment>([Composite](const FINV_LabeledNumberFragment& Fragment)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = Fragment.GetMinFractionalDigits();
		Options.MaximumFractionalDigits = Fragment.GetMaxFractionalDigits();

		const FINV_ItemStatLine StatLine {
			Fragment.GetFragmentTag(),
			Fragment.GetLabelText(),
			FText::AsNumber(Fragment.GetValue(), &Options),
			Fragment.GetCollapseLabel(),
			Fragment.GetCollapseValue()
		};
		ApplyStatLine(StatLine, Composite);
	});

	Manifest.ForEachFragment<FINV_ConsumableFragment>([Composite](const FINV_ConsumableFragment& Fragment)
	{
		for (const TInstancedStruct<FINV_ConsumeModifier>& Modifier : Fragment.GetConsumeModifiers())
		{
			const FINV_ConsumeModifier& ConsumeModifier = Modifier.Get();
			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = ConsumeModifier.GetMinFractionalDigits();
			Options.MaximumFractionalDigits = ConsumeModifier.GetMaxFractionalDigits();

			const FINV_ItemStatLine StatLine {
				ConsumeModifier.GetFragmentTag(),
				ConsumeModifier.GetLabelText(),
				FText::AsNumber(ConsumeModifier.GetValue(), &Options),
				ConsumeModifier.GetCollapseLabel(),
				ConsumeModifier.GetCollapseValue()
			};
			ApplyStatLine(StatLine, Composite);
		}
	});

	Manifest.ForEachFragment<FINV_EquipmentFragment>([Composite](const FINV_EquipmentFragment& Fragment)
	{
		for (const TInstancedStruct<FINV_EquipModifier>& Modifier : Fragment.GetEquipModifiers())
		{
			const FINV_EquipModifier& EquipModifier = Modifier.Get();
			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = EquipModifier.GetMinFractionalDigits();
			Options.MaximumFractionalDigits = EquipModifier.GetMaxFractionalDigits();

			const FINV_ItemStatLine StatLine {
				EquipModifier.GetFragmentTag(),
				EquipModifier.GetLabelText(),
				FText::AsNumber(EquipModifier.GetValue(), &Options),
				EquipModifier.GetCollapseLabel(),
				EquipModifier.GetCollapseValue()
			};
			ApplyStatLine(StatLine, Composite);
		}
	});
}

void FINV_ItemPresentationUtils::AssimilateInventoryItem(const UINV_InventoryItem& Item, UINV_Composite_Base* Composite)
{
	AssimilatePresentationSnapshot(Item.GetPresentationSnapshot(), Composite);
}

void FINV_ItemPresentationUtils::AssimilatePresentationSnapshot(const FINV_ItemPresentationSnapshot& Snapshot, UINV_Composite_Base* Composite)
{
	if (!IsValid(Composite)) return;

	if (Snapshot.Icon != nullptr)
	{
		ApplyToMatchingWidgets(FragmentTags::IconFragment, Composite, [&Snapshot](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Image* Image = Cast<UINV_Leaf_Image>(Widget);
			if (!IsValid(Image)) return;
			Image->SetImage(Snapshot.Icon);
			Image->SetBoxSize(Snapshot.IconDimensions);
			Image->SetImageSize(Snapshot.IconDimensions);
		});
	}

	for (const FINV_ItemTextLine& TextLine : Snapshot.TextLines)
	{
		ApplyToMatchingWidgets(TextLine.FragmentTag, Composite, [&TextLine](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Text* LeafText = Cast<UINV_Leaf_Text>(Widget);
			if (!IsValid(LeafText)) return;
			LeafText->SetText(TextLine.Text);
			LeafText->ApplyRarityColorFromItemRarity(Widget->GetItemRarityTag(), TextLine.FragmentTag);
		});
	}

	for (const FINV_ItemStatLine& StatLine : Snapshot.StatLines)
	{
		ApplyStatLine(StatLine, Composite);
	}
}
