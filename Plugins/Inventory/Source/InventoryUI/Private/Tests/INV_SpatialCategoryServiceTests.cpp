#include "UI/Inventory/Services/INV_SpatialCategoryService.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_SpatialCategoryService_IsGridNavigationInputTest,
	"Inventory.UI.SpatialCategoryService.IsGridNavigationInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_SpatialCategoryService_IsGridNavigationInputTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Gamepad DPad up should be considered grid navigation input"),
		FINV_SpatialCategoryService::IsGridNavigationInput(EKeys::Gamepad_DPad_Up));

	TestFalse(
		TEXT("Keyboard key should not be considered grid navigation input"),
		FINV_SpatialCategoryService::IsGridNavigationInput(EKeys::A));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_SpatialCategoryService_ResolveNextCategoryIndexWrapsForwardTest,
	"Inventory.UI.SpatialCategoryService.ResolveNextCategoryIndexWrapsForward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_SpatialCategoryService_ResolveNextCategoryIndexWrapsForwardTest::RunTest(const FString& Parameters)
{
	const int32 NextIndex = FINV_SpatialCategoryService::ResolveNextCategoryIndex(2, +1, 3);
	TestEqual(TEXT("Forward direction should wrap to first category"), NextIndex, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_SpatialCategoryService_ResolveNextCategoryIndexWrapsBackwardTest,
	"Inventory.UI.SpatialCategoryService.ResolveNextCategoryIndexWrapsBackward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_SpatialCategoryService_ResolveNextCategoryIndexWrapsBackwardTest::RunTest(const FString& Parameters)
{
	const int32 NextIndex = FINV_SpatialCategoryService::ResolveNextCategoryIndex(0, -1, 3);
	TestEqual(TEXT("Backward direction should wrap to last category"), NextIndex, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_SpatialCategoryService_ResolveNextCategoryIndexHandlesInvalidInputTest,
	"Inventory.UI.SpatialCategoryService.ResolveNextCategoryIndexHandlesInvalidInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_SpatialCategoryService_ResolveNextCategoryIndexHandlesInvalidInputTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Zero direction should return INDEX_NONE"),
		FINV_SpatialCategoryService::ResolveNextCategoryIndex(0, 0, 3),
		INDEX_NONE);

	TestEqual(
		TEXT("Non-positive category count should return INDEX_NONE"),
		FINV_SpatialCategoryService::ResolveNextCategoryIndex(0, 1, 0),
		INDEX_NONE);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
