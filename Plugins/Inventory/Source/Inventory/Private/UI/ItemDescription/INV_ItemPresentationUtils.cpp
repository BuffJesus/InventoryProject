#include "UI/ItemDescription/INV_ItemPresentationUtils.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UI/Composite/INV_Composite_Base.h"

void FINV_ItemPresentationUtils::AssimilateInventoryFragments(const FINV_ItemManifest& Manifest, UINV_Composite_Base* Composite)
{
	if (!IsValid(Composite)) return;

	const TArray<const FINV_InventoryItemFragment*> InventoryFragments = Manifest.GetFragmentsOfType<FINV_InventoryItemFragment>();
	for (const FINV_InventoryItemFragment* Fragment : InventoryFragments)
	{
		if (!Fragment) continue;
		Composite->ApplyFunction([Fragment](UINV_Composite_Base* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}
