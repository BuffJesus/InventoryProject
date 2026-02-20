#include "UI/Inventory/Placement/INV_GridPlacementEngine.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace INV_GridPlacementEngineTests
{
	static FINV_ItemManifest CreateManifest(const FIntPoint& Dimensions, const bool bStackable = false, const int32 StackCount = 1, const int32 MaxStackSize = 10)
	{
		FINV_ItemManifest Manifest;
		TArray<TInstancedStruct<FInv_ItemFragment>>& Fragments = Manifest.GetFragmentsMutable();

		TInstancedStruct<FInv_ItemFragment> GridFragment;
		GridFragment.InitializeAs<FINV_GridFragment>();
		FINV_GridFragment* MutableGridFragment = GridFragment.GetMutablePtr<FINV_GridFragment>();
		MutableGridFragment->SetFragmentTag(FragmentTags::GridFragment);
		MutableGridFragment->SetGridSize(Dimensions);
		Fragments.Add(GridFragment);

		if (bStackable)
		{
			TInstancedStruct<FInv_ItemFragment> StackableFragment;
			StackableFragment.InitializeAs<FINV_StackableFragment>();
			FINV_StackableFragment* MutableStackableFragment = StackableFragment.GetMutablePtr<FINV_StackableFragment>();
			MutableStackableFragment->SetFragmentTag(FragmentTags::StackableFragment);
			MutableStackableFragment->SetStackCount(StackCount);
			Fragments.Add(StackableFragment);

			(void)MaxStackSize;
		}

		return Manifest;
	}

	static UINV_InventoryItem* CreateItem(const FIntPoint& Dimensions, const bool bStackable = false, const int32 StackCount = 1)
	{
		FINV_ItemManifest Manifest = CreateManifest(Dimensions, bStackable, StackCount);
		UINV_InventoryItem* Item = NewObject<UINV_InventoryItem>();
		Item->SetItemManifest(Manifest);
		return Item;
	}

	static TArray<TObjectPtr<UINV_GridSlot>> CreateGrid(const FIntPoint& GridSize)
	{
		TArray<TObjectPtr<UINV_GridSlot>> GridSlots;
		GridSlots.Reserve(GridSize.X * GridSize.Y);

		for (int32 Index = 0; Index < GridSize.X * GridSize.Y; ++Index)
		{
			UINV_GridSlot* GridSlot = NewObject<UINV_GridSlot>();
			GridSlot->SetTileIndex(Index);
			GridSlot->SetInventoryItem(nullptr);
			GridSlot->SetUpperLeftIndex(INDEX_NONE);
			GridSlot->SetStackCount(0);
			GridSlot->SetAvailability(true);
			GridSlots.Add(GridSlot);
		}

		return GridSlots;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridPlacementEngine_HasRoomForSingleItemTest,
	"Inventory.UI.GridPlacementEngine.HasRoomForSingleItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridPlacementEngine_HasRoomForSingleItemTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridPlacementEngineTests;

	const FIntPoint GridSize(2, 2);
	TArray<TObjectPtr<UINV_GridSlot>> GridSlots = CreateGrid(GridSize);
	const FINV_ItemManifest Manifest = CreateManifest(FIntPoint(1, 1), false);

	const FINV_SlotAvailabilityResult Result = FINV_GridPlacementEngine::HasRoomForItem(
		GridSlots,
		GridSize,
		Manifest,
		false,
		FGameplayTag::EmptyTag);

	TestFalse(TEXT("Non-stackable result should not be stackable"), Result.bStackable);
	TestEqual(TEXT("Should have room for one non-stackable item"), Result.TotalRoomToFill, 1);
	TestTrue(TEXT("Should produce at least one candidate slot"), Result.SlotAvailabilities.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridPlacementEngine_CheckHoverPositionSingleBlockerTest,
	"Inventory.UI.GridPlacementEngine.CheckHoverPositionSingleBlocker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridPlacementEngine_CheckHoverPositionSingleBlockerTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridPlacementEngineTests;

	const FIntPoint GridSize(2, 2);
	TArray<TObjectPtr<UINV_GridSlot>> GridSlots = CreateGrid(GridSize);
	UINV_InventoryItem* Item = CreateItem(FIntPoint(1, 1));

	GridSlots[1]->SetInventoryItem(Item);
	GridSlots[1]->SetUpperLeftIndex(1);
	GridSlots[1]->SetAvailability(false);

	const FINV_SpaceQueryResult Result = FINV_GridPlacementEngine::CheckHoverPosition(
		GridSlots,
		GridSize,
		FIntPoint(1, 0),
		FIntPoint(1, 1));

	TestFalse(TEXT("Space should be blocked"), Result.bHasSpace);
	TestEqual(TEXT("Should resolve one blocking upper-left index"), Result.UpperLeftIndex, 1);
	TestEqual(TEXT("Should return blocking item"), Result.ValidItem.Get(), Item);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridPlacementEngine_CalculateStartingCoordinateEvenTopRightTest,
	"Inventory.UI.GridPlacementEngine.CalculateStartingCoordinateEvenTopRight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridPlacementEngine_CalculateStartingCoordinateEvenTopRightTest::RunTest(const FString& Parameters)
{
	const FIntPoint Cursor(5, 5);
	const FIntPoint Dimensions(2, 2);
	const FIntPoint Start = FINV_GridPlacementEngine::CalculateStartingCoordinate(Cursor, Dimensions, EINV_TileQuadrant::TopRight);

	TestEqual(TEXT("Even 2x2 at top-right should offset to expected upper-left"), Start, FIntPoint(5, 4));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
