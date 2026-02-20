#include "UI/Inventory/Stack/INV_GridStackOperations.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace INV_GridStackOperationsTests
{
	static UINV_InventoryItem* CreateStackableItem(const int32 StackCount = 1)
	{
		FINV_ItemManifest Manifest;
		TArray<TInstancedStruct<FInv_ItemFragment>>& Fragments = Manifest.GetFragmentsMutable();

		TInstancedStruct<FInv_ItemFragment> StackableFragment;
		StackableFragment.InitializeAs<FINV_StackableFragment>();
		FINV_StackableFragment* MutableStackableFragment = StackableFragment.GetMutablePtr<FINV_StackableFragment>();
		MutableStackableFragment->SetFragmentTag(FragmentTags::StackableFragment);
		MutableStackableFragment->SetStackCount(StackCount);
		Fragments.Add(StackableFragment);

		UINV_InventoryItem* Item = NewObject<UINV_InventoryItem>();
		Item->SetItemManifest(Manifest);
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridStackOperations_CalculateStackDetailsTest,
	"Inventory.UI.GridStackOperations.CalculateStackDetails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridStackOperations_CalculateStackDetailsTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridStackOperationsTests;

	UINV_GridSlot* GridSlot = NewObject<UINV_GridSlot>();
	GridSlot->SetStackCount(0);

	UINV_HoverItem* HoverItem = NewObject<UINV_HoverItem>();
	HoverItem->SetStackCount(2);

	UINV_InventoryItem* ClickedItem = CreateStackableItem(1);

	const FINV_StackDetails Details = FINV_GridStackOperations::CalculateStackDetails(GridSlot, HoverItem, ClickedItem);
	TestEqual(TEXT("Clicked stack count should come from grid slot"), Details.ClickedStackCount, 0);
	TestEqual(TEXT("Hovered stack count should come from hover item"), Details.HoveredStackCount, 2);
	TestEqual(TEXT("Default max stack for fragment should be 1"), Details.MaxStackSize, 1);
	TestEqual(TEXT("Room should be max minus clicked"), Details.RoomInClickedSlot, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridStackOperations_IsSameStackableTest,
	"Inventory.UI.GridStackOperations.IsSameStackable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridStackOperations_IsSameStackableTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridStackOperationsTests;

	UINV_InventoryItem* SharedItem = CreateStackableItem();

	UINV_HoverItem* HoverItem = NewObject<UINV_HoverItem>();
	HoverItem->SetInventoryItem(SharedItem);
	HoverItem->SetIsStackable(true);

	TestTrue(TEXT("Same item + stackable hover should be true"), FINV_GridStackOperations::IsSameStackable(HoverItem, SharedItem));

	UINV_InventoryItem* DifferentItem = CreateStackableItem();
	TestFalse(TEXT("Different item should be false"), FINV_GridStackOperations::IsSameStackable(HoverItem, DifferentItem));

	HoverItem->SetIsStackable(false);
	TestFalse(TEXT("Not stackable hover should be false"), FINV_GridStackOperations::IsSameStackable(HoverItem, SharedItem));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
