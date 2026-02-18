#include "UI/ItemDescription/INV_ItemPresentationUtils.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UI/Composite/INV_Composite_Base.h"
#include "UI/Composite/INV_Leaf_Image.h"
#include "UI/Composite/INV_Leaf_LabeledValue.h"
#include "UI/Composite/INV_Leaf_Text.h"

namespace
{
template <typename FragmentType, typename ApplyFn>
void ApplyFragmentToMatchingWidgets(const FragmentType& Fragment, UINV_Composite_Base* Composite, ApplyFn&& Apply)
{
	if (!IsValid(Composite)) return;
	Composite->ApplyFunction([&Fragment, &Apply](UINV_Composite_Base* Widget)
	{
		if (!IsValid(Widget)) return;
		if (!Widget->GetFragmentTag().MatchesTagExact(Fragment.GetFragmentTag())) return;
		Widget->Expand();
		Apply(Widget);
	});
}

void ApplyLabeledNumberFragment(const FINV_LabeledNumberFragment& Fragment, UINV_Composite_Base* Composite)
{
	ApplyFragmentToMatchingWidgets(Fragment, Composite, [&Fragment](UINV_Composite_Base* Widget)
	{
		UINV_Leaf_LabeledValue* LabeledValue = Cast<UINV_Leaf_LabeledValue>(Widget);
		if (!IsValid(LabeledValue)) return;

		LabeledValue->SetText_Label(Fragment.GetLabelText(), Fragment.GetCollapseLabel());

		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = Fragment.GetMinFractionalDigits();
		Options.MaximumFractionalDigits = Fragment.GetMaxFractionalDigits();
		LabeledValue->SetText_Value(FText::AsNumber(Fragment.GetValue(), &Options), Fragment.GetCollapseValue());
	});
}
}

void FINV_ItemPresentationUtils::AssimilateInventoryFragments(const FINV_ItemManifest& Manifest, UINV_Composite_Base* Composite)
{
	if (!IsValid(Composite)) return;

	for (const FINV_ImageFragment* Fragment : Manifest.GetFragmentsOfType<FINV_ImageFragment>())
	{
		if (!Fragment) continue;
		ApplyFragmentToMatchingWidgets(*Fragment, Composite, [Fragment](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Image* Image = Cast<UINV_Leaf_Image>(Widget);
			if (!IsValid(Image)) return;
			Image->SetImage(Fragment->GetIcon());
			Image->SetBoxSize(Fragment->GetIconDimensions());
			Image->SetImageSize(Fragment->GetIconDimensions());
		});
	}

	for (const FINV_TextFragment* Fragment : Manifest.GetFragmentsOfType<FINV_TextFragment>())
	{
		if (!Fragment) continue;
		ApplyFragmentToMatchingWidgets(*Fragment, Composite, [Fragment](UINV_Composite_Base* Widget)
		{
			UINV_Leaf_Text* LeafText = Cast<UINV_Leaf_Text>(Widget);
			if (!IsValid(LeafText)) return;
			LeafText->SetText(Fragment->GetText());
			LeafText->ApplyRarityColorFromItemRarity(Widget->GetItemRarityTag(), Fragment->GetFragmentTag());
		});
	}

	for (const FINV_LabeledNumberFragment* Fragment : Manifest.GetFragmentsOfType<FINV_LabeledNumberFragment>())
	{
		if (!Fragment) continue;
		ApplyLabeledNumberFragment(*Fragment, Composite);
	}

	for (const FINV_ConsumableFragment* Fragment : Manifest.GetFragmentsOfType<FINV_ConsumableFragment>())
	{
		if (!Fragment) continue;
		for (const TInstancedStruct<FINV_ConsumeModifier>& Modifier : Fragment->GetConsumeModifiers())
		{
			ApplyLabeledNumberFragment(Modifier.Get(), Composite);
		}
	}
}
