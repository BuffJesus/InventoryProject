#include "UI/Inventory/EventHandling/INV_GridEventHandler.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridEventHandler_PrefersLargerAnchorWhenTypeBiasEqualTest,
	"Inventory.UI.GridEventHandler.PrefersLargerAnchorWhenTypeBiasEqual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridEventHandler_PrefersLargerAnchorWhenTypeBiasEqualTest::RunTest(const FString& Parameters)
{
	const TArray<int32> BlockingIndices { 5, 9 };
	const int32 BestAnchor = FINV_GridEventHandler::FindBestAnchorForMultiBlocker(
		BlockingIndices,
		[](const int32 Index) -> FIntPoint
		{
			return Index == 5 ? FIntPoint(1, 1) : FIntPoint(2, 2);
		},
		[](const int32 Index) -> FGameplayTag
		{
			return Index == 5 ? FGameplayTag::EmptyTag : FGameplayTag::EmptyTag;
		},
		FGameplayTag::EmptyTag);

	TestEqual(TEXT("Larger blocker should be selected when type bias is equal"), BestAnchor, 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridEventHandler_PrefersTypeMatchOverAreaTest,
	"Inventory.UI.GridEventHandler.PrefersTypeMatchOverArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridEventHandler_PrefersTypeMatchOverAreaTest::RunTest(const FString& Parameters)
{
	const FGameplayTag HoverType = FragmentTags::Rarity::Rarity_Common;
	const FGameplayTag OtherType = FragmentTags::Rarity::Rarity_Epic;

	const TArray<int32> BlockingIndices { 1, 2 };
	const int32 BestAnchor = FINV_GridEventHandler::FindBestAnchorForMultiBlocker(
		BlockingIndices,
		[](const int32 Index) -> FIntPoint
		{
			return Index == 1 ? FIntPoint(1, 1) : FIntPoint(3, 3);
		},
		[HoverType, OtherType](const int32 Index) -> FGameplayTag
		{
			return Index == 1 ? HoverType : OtherType;
		},
		HoverType);

	TestEqual(TEXT("Type match should beat larger non-matching blocker"), BestAnchor, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridEventHandler_DetermineStackActionBranchesTest,
	"Inventory.UI.GridEventHandler.DetermineStackActionBranches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridEventHandler_DetermineStackActionBranchesTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Full destination + partial hover should swap stacks"),
		FINV_GridEventHandler::DetermineStackAction(10, 4, 10, 0),
		FINV_ClickActionResult::EAction::SwapStacks);

	TestEqual(
		TEXT("Enough room should merge all hover stacks"),
		FINV_GridEventHandler::DetermineStackAction(3, 2, 10, 7),
		FINV_ClickActionResult::EAction::MergeStacks);

	TestEqual(
		TEXT("Partial room should fill destination"),
		FINV_GridEventHandler::DetermineStackAction(8, 5, 10, 2),
		FINV_ClickActionResult::EAction::FillStack);

	TestEqual(
		TEXT("Full destination with full hover should swap items"),
		FINV_GridEventHandler::DetermineStackAction(10, 10, 10, 0),
		FINV_ClickActionResult::EAction::SwapItems);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
