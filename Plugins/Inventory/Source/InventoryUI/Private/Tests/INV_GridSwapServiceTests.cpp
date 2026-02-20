#include "UI/Inventory/Services/INV_GridSwapService.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace INV_GridSwapServiceTests
{
	static UINV_InventoryItem* CreateItem(const FIntPoint& Dimensions, const bool bStackable = false)
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
			Fragments.Add(StackableFragment);
		}

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

	static void PlaceItemAt(
		TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		UINV_InventoryItem* Item,
		const int32 UpperLeftIndex,
		const int32 StackCount = 0)
	{
		const FINV_GridFragment* GridFragment = Item ? Item->GetCachedGridFragment() : nullptr;
		const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);

		const int32 StartX = UpperLeftIndex % GridSize.X;
		const int32 StartY = UpperLeftIndex / GridSize.X;

		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			for (int32 X = 0; X < Dimensions.X; ++X)
			{
				const int32 Index = (StartY + Y) * GridSize.X + (StartX + X);
				GridSlots[Index]->SetInventoryItem(Item);
				GridSlots[Index]->SetUpperLeftIndex(UpperLeftIndex);
				GridSlots[Index]->SetAvailability(false);
			}
		}

		GridSlots[UpperLeftIndex]->SetStackCount(StackCount);
	}

	static bool ContainsItemAtIndex(const TArray<TPair<UINV_InventoryItem*, int32>>& Pairs, UINV_InventoryItem* Item, const int32 Index)
	{
		for (const TPair<UINV_InventoryItem*, int32>& Pair : Pairs)
		{
			if (Pair.Key == Item && Pair.Value == Index)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridSwapService_FailsWhenClickedNotOverlappedTest,
	"Inventory.UI.GridSwapService.FailsWhenClickedNotOverlapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridSwapService_FailsWhenClickedNotOverlappedTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridSwapServiceTests;

	const FIntPoint GridSize(2, 2);
	TArray<TObjectPtr<UINV_GridSlot>> GridSlots = CreateGrid(GridSize);
	UINV_InventoryItem* ItemAt0 = CreateItem(FIntPoint(1, 1));
	UINV_InventoryItem* ItemAt1 = CreateItem(FIntPoint(1, 1));
	UINV_InventoryItem* HoverItem = CreateItem(FIntPoint(1, 1));

	PlaceItemAt(GridSlots, GridSize, ItemAt0, 0);
	PlaceItemAt(GridSlots, GridSize, ItemAt1, 1);

	const bool bResult = FINV_GridSwapService::ExecuteSwapWithHoverItem(
		GridSlots,
		GridSize,
		1,          // hover overlaps index 1 only
		ItemAt0,    // clicked item at index 0 is not overlapped
		0,
		HoverItem,
		FIntPoint(1, 1),
		5,
		true,
		0,
		FINV_GridSwapCallbacks{});

	TestFalse(TEXT("Swap should fail when clicked item is not in overlapped blockers"), bResult);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FINV_GridSwapService_ExecutesRelocationPlanTest,
	"Inventory.UI.GridSwapService.ExecutesRelocationPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINV_GridSwapService_ExecutesRelocationPlanTest::RunTest(const FString& Parameters)
{
	using namespace INV_GridSwapServiceTests;

	const FIntPoint GridSize(2, 2);
	TArray<TObjectPtr<UINV_GridSlot>> GridSlots = CreateGrid(GridSize);

	UINV_InventoryItem* ClickedItem = CreateItem(FIntPoint(1, 1), true);
	UINV_InventoryItem* DisplacedItem = CreateItem(FIntPoint(1, 1), false);
	UINV_InventoryItem* HoverItem = CreateItem(FIntPoint(2, 1), true);

	PlaceItemAt(GridSlots, GridSize, ClickedItem, 0, 11);
	PlaceItemAt(GridSlots, GridSize, DisplacedItem, 1, 3);

	int32 AssignHoverCalls = 0;
	int32 RemoveCalls = 0;
	int32 AddCalls = 0;
	int32 UpdateCalls = 0;
	int32 RefreshCalls = 0;
	TArray<TPair<UINV_InventoryItem*, int32>> RemovedPairs;
	TArray<TPair<UINV_InventoryItem*, int32>> AddedPairs;

	FINV_GridSwapCallbacks Callbacks;
	Callbacks.AssignHoverItem = [&](UINV_InventoryItem* Item, const int32 GridIndex, const int32 PreviousGridIndex)
	{
		++AssignHoverCalls;
		TestEqual(TEXT("AssignHover receives clicked item"), Item, ClickedItem);
		TestEqual(TEXT("AssignHover receives clicked index"), GridIndex, 0);
		TestEqual(TEXT("AssignHover receives hover previous index"), PreviousGridIndex, 9);
	};
	Callbacks.RemoveItemFromGrid = [&](UINV_InventoryItem* Item, const int32 GridIndex)
	{
		++RemoveCalls;
		RemovedPairs.Add(TPair<UINV_InventoryItem*, int32>(Item, GridIndex));
	};
	Callbacks.AddItemAtIndex = [&](UINV_InventoryItem* Item, const int32 GridIndex, const bool bStackable, const int32 StackCount)
	{
		++AddCalls;
		AddedPairs.Add(TPair<UINV_InventoryItem*, int32>(Item, GridIndex));

		if (Item == HoverItem)
		{
			TestTrue(TEXT("Hover item add should preserve stackable flag"), bStackable);
			TestEqual(TEXT("Hover item add should preserve stack count"), StackCount, 5);
		}
	};
	Callbacks.UpdateGridSlots = [&](UINV_InventoryItem* Item, const int32 GridIndex, const bool bStackable, const int32 StackCount)
	{
		++UpdateCalls;
		if (Item == HoverItem)
		{
			TestTrue(TEXT("Hover item update should preserve stackable flag"), bStackable);
			TestEqual(TEXT("Hover item update should preserve stack count"), StackCount, 5);
		}
	};
	Callbacks.RefreshGridSlotVisuals = [&]()
	{
		++RefreshCalls;
	};

	const bool bResult = FINV_GridSwapService::ExecuteSwapWithHoverItem(
		GridSlots,
		GridSize,
		0,
		ClickedItem,
		0,
		HoverItem,
		FIntPoint(2, 1),
		5,
		true,
		9,
		Callbacks);

	TestTrue(TEXT("Swap should execute successfully"), bResult);
	TestEqual(TEXT("AssignHover should be called once"), AssignHoverCalls, 1);
	TestEqual(TEXT("Two overlapped blockers should be removed"), RemoveCalls, 2);
	TestEqual(TEXT("Hover placement + one relocation add"), AddCalls, 2);
	TestEqual(TEXT("Hover placement + one relocation update"), UpdateCalls, 2);
	TestEqual(TEXT("Grid refresh should be called once"), RefreshCalls, 1);

	TestTrue(TEXT("Removed clicked item at index 0"), ContainsItemAtIndex(RemovedPairs, ClickedItem, 0));
	TestTrue(TEXT("Removed displaced item at index 1"), ContainsItemAtIndex(RemovedPairs, DisplacedItem, 1));

	TestTrue(TEXT("Hover item added at placement index 0"), ContainsItemAtIndex(AddedPairs, HoverItem, 0));
	TestTrue(TEXT("Displaced item relocated to first-fit index 2"), ContainsItemAtIndex(AddedPairs, DisplacedItem, 2));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
