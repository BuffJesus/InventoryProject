#include "UI/Inventory/Services/INV_EquippedSlotService.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace INV_EquippedSlotServiceTests
{
	static FPointerEvent MakePointerEvent(const FKey& Button)
	{
		return FPointerEvent(
			0u,
			0u,
			FVector2D::ZeroVector,
			FVector2D::ZeroVector,
			TSet<FKey> { Button },
			Button,
			0.0f,
			FModifierKeysState());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_InvalidItemReturnsNoneTest,
	"Inventory.UI.EquippedSlotService.InvalidItemReturnsNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_InvalidItemReturnsNoneTest::RunTest(const FString& Parameters)
{
	using namespace INV_EquippedSlotServiceTests;

	const EINV_EquippedClickAction Action = FINV_EquippedSlotService::ResolveClickAction(
		MakePointerEvent(EKeys::LeftMouseButton),
		false,
		false);

	TestEqual(TEXT("Invalid slotted item should not produce an action"), Action, EINV_EquippedClickAction::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_RightClickInspectsTest,
	"Inventory.UI.EquippedSlotService.RightClickInspects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_RightClickInspectsTest::RunTest(const FString& Parameters)
{
	using namespace INV_EquippedSlotServiceTests;

	const EINV_EquippedClickAction Action = FINV_EquippedSlotService::ResolveClickAction(
		MakePointerEvent(EKeys::RightMouseButton),
		true,
		true);

	TestEqual(TEXT("Right-click should inspect equipped item"), Action, EINV_EquippedClickAction::Inspect);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_StackableHoverBlocksSwapTest,
	"Inventory.UI.EquippedSlotService.StackableHoverBlocksSwap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_StackableHoverBlocksSwapTest::RunTest(const FString& Parameters)
{
	using namespace INV_EquippedSlotServiceTests;

	const EINV_EquippedClickAction Action = FINV_EquippedSlotService::ResolveClickAction(
		MakePointerEvent(EKeys::LeftMouseButton),
		true,
		true);

	TestEqual(TEXT("Stackable hover item should block equip swap"), Action, EINV_EquippedClickAction::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_LeftClickSwapsTest,
	"Inventory.UI.EquippedSlotService.LeftClickSwaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_LeftClickSwapsTest::RunTest(const FString& Parameters)
{
	using namespace INV_EquippedSlotServiceTests;

	const EINV_EquippedClickAction Action = FINV_EquippedSlotService::ResolveClickAction(
		MakePointerEvent(EKeys::LeftMouseButton),
		true,
		false);

	TestEqual(TEXT("Left-click with non-stackable hover should swap equipped item"), Action, EINV_EquippedClickAction::Swap);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_RemoveSlottedItemInvokesUnbindTest,
	"Inventory.UI.EquippedSlotService.RemoveSlottedItemInvokesUnbind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_RemoveSlottedItemInvokesUnbindTest::RunTest(const FString& Parameters)
{
	bool bUnbindCalled = false;
	FINV_EquippedSlotCallbacks Callbacks;
	Callbacks.UnbindEquippedSlottedItemDelegates = [&bUnbindCalled](UINV_EquippedSlottedItem* Item)
	{
		bUnbindCalled = IsValid(Item);
	};

	UINV_EquippedSlottedItem* SlottedItem = NewObject<UINV_EquippedSlottedItem>();
	FINV_EquippedSlotService::RemoveSlottedItem(SlottedItem, Callbacks);

	TestTrue(TEXT("RemoveSlottedItem should invoke unbind callback for valid slotted item"), bUnbindCalled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_EquippedSlotService_RemoveSlottedItemIgnoresNullTest,
	"Inventory.UI.EquippedSlotService.RemoveSlottedItemIgnoresNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_EquippedSlotService_RemoveSlottedItemIgnoresNullTest::RunTest(const FString& Parameters)
{
	bool bUnbindCalled = false;
	FINV_EquippedSlotCallbacks Callbacks;
	Callbacks.UnbindEquippedSlottedItemDelegates = [&bUnbindCalled](UINV_EquippedSlottedItem*)
	{
		bUnbindCalled = true;
	};

	FINV_EquippedSlotService::RemoveSlottedItem(nullptr, Callbacks);

	TestFalse(TEXT("RemoveSlottedItem should not invoke unbind callback for null slotted item"), bUnbindCalled);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
