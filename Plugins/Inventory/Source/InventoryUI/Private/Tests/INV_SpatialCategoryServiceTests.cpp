#include "UI/Inventory/Services/INV_SpatialCategoryService.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

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

	TestEqual(
		TEXT("INDEX_NONE current index should fallback to zero before advancing"),
		FINV_SpatialCategoryService::ResolveNextCategoryIndex(INDEX_NONE, 1, 3),
		1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
