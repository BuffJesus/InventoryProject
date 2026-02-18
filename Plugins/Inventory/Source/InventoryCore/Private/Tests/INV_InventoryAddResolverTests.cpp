#include "InventoryManagement/Rules/INV_InventoryAddResolver.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_AddResolver_NoRoomTest,
	"Inventory.Core.AddResolver.NoRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_AddResolver_NoRoomTest::RunTest(const FString& Parameters)
{
	FINV_SlotAvailabilityResult SpaceResult;
	SpaceResult.TotalRoomToFill = 0;
	SpaceResult.bStackable = true;

	const FINV_AddItemDecision Decision = FINV_InventoryAddResolver::Resolve(SpaceResult, true);
	TestEqual(TEXT("Action should be NoRoom"), Decision.Action, EINV_AddItemAction::NoRoom);
	TestEqual(TEXT("StackCountToAdd should be 0"), Decision.StackCountToAdd, 0);
	TestEqual(TEXT("Remainder should be 0"), Decision.Remainder, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_AddResolver_AddStacksTest,
	"Inventory.Core.AddResolver.AddStacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_AddResolver_AddStacksTest::RunTest(const FString& Parameters)
{
	FINV_SlotAvailabilityResult SpaceResult;
	SpaceResult.TotalRoomToFill = 5;
	SpaceResult.Remainder = 2;
	SpaceResult.bStackable = true;

	const FINV_AddItemDecision Decision = FINV_InventoryAddResolver::Resolve(SpaceResult, true);
	TestEqual(TEXT("Action should be AddStacks"), Decision.Action, EINV_AddItemAction::AddStacks);
	TestEqual(TEXT("StackCountToAdd should match room"), Decision.StackCountToAdd, 5);
	TestEqual(TEXT("Remainder should be preserved"), Decision.Remainder, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_AddResolver_AddNewItemStackableTest,
	"Inventory.Core.AddResolver.AddNewItemStackable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_AddResolver_AddNewItemStackableTest::RunTest(const FString& Parameters)
{
	FINV_SlotAvailabilityResult SpaceResult;
	SpaceResult.TotalRoomToFill = 4;
	SpaceResult.Remainder = 1;
	SpaceResult.bStackable = true;

	const FINV_AddItemDecision Decision = FINV_InventoryAddResolver::Resolve(SpaceResult, false);
	TestEqual(TEXT("Action should be AddNewItem"), Decision.Action, EINV_AddItemAction::AddNewItem);
	TestEqual(TEXT("StackCountToAdd should match room for stackable"), Decision.StackCountToAdd, 4);
	TestEqual(TEXT("Remainder should stay default for new item"), Decision.Remainder, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_AddResolver_AddNewItemNonStackableTest,
	"Inventory.Core.AddResolver.AddNewItemNonStackable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_AddResolver_AddNewItemNonStackableTest::RunTest(const FString& Parameters)
{
	FINV_SlotAvailabilityResult SpaceResult;
	SpaceResult.TotalRoomToFill = 1;
	SpaceResult.bStackable = false;

	const FINV_AddItemDecision Decision = FINV_InventoryAddResolver::Resolve(SpaceResult, false);
	TestEqual(TEXT("Action should be AddNewItem"), Decision.Action, EINV_AddItemAction::AddNewItem);
	TestEqual(TEXT("StackCountToAdd should be 0 for non-stackable"), Decision.StackCountToAdd, 0);
	TestEqual(TEXT("Remainder should be 0"), Decision.Remainder, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
