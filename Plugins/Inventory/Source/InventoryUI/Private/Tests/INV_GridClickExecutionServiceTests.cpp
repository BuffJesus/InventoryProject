#include "UI/Inventory/Services/INV_GridClickExecutionService.h"
#include "Items/INV_InventoryItem.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridClickExecutionService_SlottedPickupTest,
	"Inventory.UI.ClickExecution.Slotted.Pickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridClickExecutionService_SlottedPickupTest::RunTest(const FString& Parameters)
{
	FINV_GridClickResult ActionResult;
	ActionResult.Action = EINV_ClickAction::Pickup;

	FINV_StackDetails StackDetails;
	bool bPickupCalled = false;
	UINV_InventoryItem* ReceivedItem = nullptr;
	int32 ReceivedIndex = INDEX_NONE;
	UINV_InventoryItem* ClickedItem = NewObject<UINV_InventoryItem>();

	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.Pickup = [&](UINV_InventoryItem* Item, const int32 GridIndex)
	{
		bPickupCalled = true;
		ReceivedItem = Item;
		ReceivedIndex = GridIndex;
	};

	constexpr int32 TestIndex = 7;
	FINV_GridClickExecutionService::ExecuteSlottedItemClick(
		ActionResult,
		StackDetails,
		ClickedItem,
		TestIndex,
		Callbacks);

	TestTrue(TEXT("Pickup callback should be called"), bPickupCalled);
	TestEqual(TEXT("Pickup callback receives clicked item"), ReceivedItem, ClickedItem);
	TestEqual(TEXT("Pickup callback receives grid index"), ReceivedIndex, TestIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridClickExecutionService_SlottedFillStackTest,
	"Inventory.UI.ClickExecution.Slotted.FillStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridClickExecutionService_SlottedFillStackTest::RunTest(const FString& Parameters)
{
	FINV_GridClickResult ActionResult;
	ActionResult.Action = EINV_ClickAction::FillStack;
	ActionResult.AuxiliaryValue = 3;

	FINV_StackDetails StackDetails;
	StackDetails.HoveredStackCount = 10;

	bool bFillCalled = false;
	int32 ReceivedFillAmount = 0;
	int32 ReceivedRemainder = 0;
	int32 ReceivedIndex = INDEX_NONE;

	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.FillStack = [&](const int32 FillAmount, const int32 Remainder, const int32 GridIndex)
	{
		bFillCalled = true;
		ReceivedFillAmount = FillAmount;
		ReceivedRemainder = Remainder;
		ReceivedIndex = GridIndex;
	};

	constexpr int32 TestIndex = 12;
	FINV_GridClickExecutionService::ExecuteSlottedItemClick(
		ActionResult,
		StackDetails,
		nullptr,
		TestIndex,
		Callbacks);

	TestTrue(TEXT("FillStack callback should be called"), bFillCalled);
	TestEqual(TEXT("Fill amount should match auxiliary value"), ReceivedFillAmount, 3);
	TestEqual(TEXT("Remainder should be hovered stack minus fill amount"), ReceivedRemainder, 7);
	TestEqual(TEXT("FillStack callback receives grid index"), ReceivedIndex, TestIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridClickExecutionService_GridSlotPlaceItemTest,
	"Inventory.UI.ClickExecution.GridSlot.PlaceItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridClickExecutionService_GridSlotPlaceItemTest::RunTest(const FString& Parameters)
{
	FINV_GridClickResult ActionResult;
	ActionResult.Action = EINV_ClickAction::PlaceItem;
	ActionResult.TargetIndex = 15;

	bool bPlaceCalled = false;
	int32 ReceivedIndex = INDEX_NONE;

	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.PlaceItem = [&](const int32 GridIndex)
	{
		bPlaceCalled = true;
		ReceivedIndex = GridIndex;
	};

	FINV_GridClickExecutionService::ExecuteGridSlotClick(ActionResult, Callbacks);

	TestTrue(TEXT("PlaceItem callback should be called"), bPlaceCalled);
	TestEqual(TEXT("PlaceItem callback receives target index"), ReceivedIndex, 15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridClickExecutionService_GridSlotSwapRouteTest,
	"Inventory.UI.ClickExecution.GridSlot.SwapRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridClickExecutionService_GridSlotSwapRouteTest::RunTest(const FString& Parameters)
{
	FINV_GridClickResult ActionResult;
	ActionResult.Action = EINV_ClickAction::SwapItems;
	ActionResult.TargetIndex = 4;

	bool bRouteCalled = false;
	int32 ReceivedIndex = INDEX_NONE;

	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.RouteToSlottedClick = [&](const int32 GridIndex)
	{
		bRouteCalled = true;
		ReceivedIndex = GridIndex;
	};

	FINV_GridClickExecutionService::ExecuteGridSlotClick(ActionResult, Callbacks);

	TestTrue(TEXT("RouteToSlottedClick callback should be called"), bRouteCalled);
	TestEqual(TEXT("Route callback receives target index"), ReceivedIndex, 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
