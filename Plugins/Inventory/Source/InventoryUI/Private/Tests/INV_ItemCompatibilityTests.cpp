#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "UI/Inventory/EventHandling/INV_GridEventHandler.h"
#include "UI/Inventory/Transfer/INV_ItemTransferHandler.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace INV_ItemCompatibilityTests
{
	static UINV_InventoryItem* CreateStackableItem()
	{
		FINV_ItemManifest Manifest;
		TArray<TInstancedStruct<FInv_ItemFragment>>& Fragments = Manifest.GetFragmentsMutable();

		TInstancedStruct<FInv_ItemFragment> StackableFragment;
		StackableFragment.InitializeAs<FINV_StackableFragment>();
		FINV_StackableFragment* MutableStackableFragment = StackableFragment.GetMutablePtr<FINV_StackableFragment>();
		MutableStackableFragment->SetFragmentTag(FragmentTags::StackableFragment);
		Fragments.Add(StackableFragment);

		UINV_InventoryItem* Item = NewObject<UINV_InventoryItem>();
		Item->SetItemManifest(Manifest);
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_ItemCompatibility_StackMergeRejectsRarityModeMismatchTest,
	"Inventory.UI.ItemCompatibility.StackMergeRejectsRarityModeMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_ItemCompatibility_StackMergeRejectsRarityModeMismatchTest::RunTest(const FString& Parameters)
{
	using namespace INV_ItemCompatibilityTests;

	UINV_InventoryItem* SourceItem = CreateStackableItem();
	UINV_InventoryItem* TargetItem = CreateStackableItem();

	SourceItem->SetItemRarityOptions(true, FragmentTags::Rarity::Rarity_Common);
	TargetItem->SetItemRarityOptions(false, FGameplayTag::EmptyTag);

	TestFalse(
		TEXT("Event handler merge check should reject rarity mode mismatch"),
		FINV_GridEventHandler::CanMergeStacks(SourceItem, TargetItem));

	TestFalse(
		TEXT("Transfer handler merge check should reject rarity mode mismatch"),
		FINV_ItemTransferHandler::AreItemsStackCompatible(SourceItem, TargetItem));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_ItemCompatibility_StackMergeAllowsExactTypeAndRarityTest,
	"Inventory.UI.ItemCompatibility.StackMergeAllowsExactTypeAndRarity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_ItemCompatibility_StackMergeAllowsExactTypeAndRarityTest::RunTest(const FString& Parameters)
{
	using namespace INV_ItemCompatibilityTests;

	UINV_InventoryItem* SourceItem = CreateStackableItem();
	UINV_InventoryItem* TargetItem = CreateStackableItem();

	SourceItem->SetItemRarityOptions(true, FragmentTags::Rarity::Rarity_Epic);
	TargetItem->SetItemRarityOptions(true, FragmentTags::Rarity::Rarity_Epic);

	TestTrue(
		TEXT("Event handler merge check should allow exact rarity match"),
		FINV_GridEventHandler::CanMergeStacks(SourceItem, TargetItem));

	TestTrue(
		TEXT("Transfer handler merge check should allow exact rarity match"),
		FINV_ItemTransferHandler::AreItemsStackCompatible(SourceItem, TargetItem));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
