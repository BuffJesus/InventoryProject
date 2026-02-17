// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "InventoryManagement/GridPlacement/INV_GridPlacementEngine.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Types/INV_ItemTransferHandler.h"
#include "UI/INV_WidgetUtils.h"
#include "UI/Inventory/EventHandling/INV_GridEventHandler.h"
#include "UI/Inventory/Factory/INV_GridWidgetFactory.h"
#include "UI/Inventory/State/INV_GridStateManager.h"
#include "UI/Inventory/Popup/INV_GridPopupManager.h"
#include "UI/Inventory/Stack/INV_GridStackOperations.h"
#include "UI/Inventory/Hover/INV_HoverItemManager.h"
#include "UI/Inventory/Coordinate/INV_GridCoordinateCalculator.h"
#include "UI/Inventory/Click/INV_GridClickActionResolver.h"
#include "UI/Inventory/Items/INV_GridItemOperations.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/ItemPopUp/INV_ItemPopUp.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const UINV_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return FINV_SlotAvailabilityResult {};
	return HasRoomForItem(&ItemComponent->GetItemManifest(), ItemComponent->IsItemRarityEnabled(), ItemComponent->GetItemRarityTag());
}

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const FINV_ItemManifest* Manifest)
{
	return HasRoomForItem(Manifest, false, FGameplayTag::EmptyTag);
}

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const FINV_ItemManifest* Manifest, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	if (!Manifest) return FINV_SlotAvailabilityResult {};
	// Delegate to placement engine for pure logic computation
	return FINV_GridPlacementEngine::HasRoomForItem(GridSlots, GridSize, *Manifest, bUseItemRarity, ItemRarityTag);
}

// NOTE: Core placement methods have been moved to FINV_GridPlacementEngine for reusability

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const UINV_InventoryItem* Item)
{
	if (!IsValid(Item)) return FINV_SlotAvailabilityResult {};
	return HasRoomForItem(&Item->GetItemManifest(), Item->IsItemRarityEnabled(), Item->GetItemRarityTag());
}

void UINV_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// Build slots and wire up inventory events.
	ConstructGrid();
	
	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
		InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
	}
}

void UINV_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ClosePopupIfClickedOutside();
	
	// Track mouse position for hover placement.
	const FVector2D CanvasPos = UINV_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D CanvasSize = UINV_WidgetUtils::GetWidgetSize(CanvasPanel);
	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	
	if (CursorExitedCanvas(CanvasPos, CanvasSize, MousePos))
	{
		return;
	}
	
	UpdateTileParams(CanvasPos, MousePos);
}

FIntPoint UINV_InventoryGrid::CalculateHoverCoordinates(const FVector2D& CanvasPos, const FVector2D& MousePos) const
{
	return UINV_GridCoordinateCalculator::CalculateHoverCoordinates(CanvasPos, MousePos, TileSize);
}

EINV_TileQuadrant UINV_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPos, const FVector2D& MousePos) const
{
	return UINV_GridCoordinateCalculator::CalculateTileQuadrant(CanvasPos, MousePos, TileSize);
}

void UINV_InventoryGrid::UpdateTileParams(const FVector2D& CanvasPos, const FVector2D& MousePos)
{
	if (!bMouseWithinCanvas) return;
	
	// Calculate tile quadrant, index, and coords
	const FIntPoint HoveredTileCoords { CalculateHoverCoordinates(CanvasPos, MousePos) };
	
	LastTileParams = TileParams;
	TileParams.TileCoordinates = HoveredTileCoords;
	TileParams.TileIndex = UINV_WidgetUtils::GetIndexFromPosition(HoveredTileCoords, GridSize.X);
	TileParams.TileQuadrant = CalculateTileQuadrant(CanvasPos, MousePos);
	
	OnTileParamsUpdated(TileParams);
}

void UINV_InventoryGrid::ClosePopupIfClickedOutside()
{
	UINV_GridPopupManager::ClosePopupIfClickedOutside(ItemPopUp, GetOwningPlayer());
}

void UINV_InventoryGrid::CloseActiveItemPopup()
{
	UINV_GridPopupManager::CloseActiveItemPopup(ItemPopUp);
}

void UINV_InventoryGrid::OnTileParamsUpdated(const FINV_TileParams& Params)
{
	if (!IsValid(HoverItem)) return;

	// Get hover item dimensions
	const FIntPoint Dimensions { HoverItem->GetGridDimensions() };

	// Calculate starting coord for highlighting - delegate to placement engine
	const FIntPoint StartingCoord { FINV_GridPlacementEngine::CalculateStartingCoordinate(Params.TileCoordinates, Dimensions, Params.TileQuadrant) };
	ItemDropIndex = UINV_WidgetUtils::GetIndexFromPosition(StartingCoord, GridSize.X);

	// Check hover position - delegate to placement engine
	CurrentQueryResult = FINV_GridPlacementEngine::CheckHoverPosition(GridSlots, GridSize, StartingCoord, Dimensions);

	if (CurrentQueryResult.bHasSpace)
	{
		// Free space: highlight potential drop area.
		UnHighlightBlockingItems();
		HighlightSlots(ItemDropIndex, Dimensions);
		return;
	}
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	HighlightBlockingItems(CurrentQueryResult.BlockingUpperLeftIndices);

	// Show the exact hovered-item placement footprint with a distinct visual.
	if (FINV_GridPlacementEngine::IsInGridBounds(ItemDropIndex, Dimensions, GridSize))
	{
		UINV_InventoryStatics::ForEach2D(GridSlots, ItemDropIndex, Dimensions, GridSize.X,
			[](UINV_GridSlot* GridSlot)
		{
			GridSlot->SetSelectedTexture();
		});
		LastHighlightedIndex = ItemDropIndex;
		LastHighlightedDimensions = Dimensions;
	}
}

void UINV_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UINV_GridStateManager::HighlightSlots(GridSlots, GridSize.X, Index, Dimensions, bMouseWithinCanvas);
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void UINV_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UINV_GridStateManager::UnHighlightSlots(GridSlots, GridSize.X, Index, Dimensions);
}

void UINV_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EINV_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UINV_GridStateManager::ChangeSlotState(GridSlots, GridSize.X, Index, Dimensions, GridSlotState);
	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

void UINV_InventoryGrid::HighlightBlockingItems(const TArray<int32>& BlockingUpperLeftIndices)
{
	UnHighlightBlockingItems();
	LastHighlightedIndex = INDEX_NONE;
	LastHighlightedDimensions = FIntPoint::ZeroValue;

	// Lambda to get grid fragment for an item at a given index
	auto GetFragmentAtIndex = [this](int32 Index) -> const FINV_GridFragment*
	{
		if (!GridSlots.IsValidIndex(Index)) return nullptr;
		const UINV_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
		if (!IsValid(Item)) return nullptr;
		return GetFragment<FINV_GridFragment>(Item, FragmentTags::GridFragment);
	};

	LastGrayedOutUpperLeftIndices = UINV_GridStateManager::HighlightBlockingItems(
		GridSlots, GridSize.X, BlockingUpperLeftIndices, GetFragmentAtIndex);
}

void UINV_InventoryGrid::UnHighlightBlockingItems()
{
	// Lambda to get grid fragment for an item at a given index
	auto GetFragmentAtIndex = [this](int32 Index) -> const FINV_GridFragment*
	{
		if (!GridSlots.IsValidIndex(Index)) return nullptr;
		const UINV_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
		if (!IsValid(Item)) return nullptr;
		return GetFragment<FINV_GridFragment>(Item, FragmentTags::GridFragment);
	};

	UINV_GridStateManager::UnHighlightBlockingItems(
		GridSlots, GridSize.X, LastGrayedOutUpperLeftIndices, GetFragmentAtIndex);
	LastGrayedOutUpperLeftIndices.Reset();
}

void UINV_InventoryGrid::RefreshGridSlotVisualsFromAvailability()
{
	UINV_GridStateManager::RefreshAllSlotVisuals(GridSlots);
}

// NOTE: CalculateStartingCoordinate moved to FINV_GridPlacementEngine

void UINV_InventoryGrid::AddItem(UINV_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	
	// Compute placement and update UI.
	FINV_SlotAvailabilityResult Result { HasRoomForItem(Item) };
	AddItemToIndices(Result, Item);
}

void UINV_InventoryGrid::AddItemToIndices(const FINV_SlotAvailabilityResult& Result, UINV_InventoryItem* NewItem)
{
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UpdateGridSlots(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

FVector2D UINV_InventoryGrid::GetDrawSize(const FINV_GridFragment* GridFragment) const
{
	return UINV_GridWidgetFactory::CalculateDrawSize(GridFragment, TileSize);
}

void UINV_InventoryGrid::SetSlottedItemImageBrush(const FINV_GridFragment* GridFragment,
	const FINV_ImageFragment* ImageFragment, const UINV_SlottedItem* SlottedItem) const
{
	// Deprecated - functionality moved to factory
}

UINV_SlottedItem* UINV_InventoryGrid::CreateSlottedItem(UINV_InventoryItem* Item, const bool bStackable, const int32 StackAmount,
	const FINV_GridFragment* GridFragment, const FINV_ImageFragment* ImageFragment, const int32 Index)
{
	FINV_GridWidgetFactoryConfig Config;
	Config.TileSize = TileSize;
	Config.GridWidth = GridSize.X;
	Config.CanvasPanel = CanvasPanel;

	UINV_SlottedItem* SlottedItem = UINV_GridWidgetFactory::CreateSlottedItem(
		Item, bStackable, StackAmount, GridFragment, ImageFragment, Index, Config, this, SlottedItemClass);

	if (IsValid(SlottedItem))
	{
		SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
		SlottedItem->OnSlottedItemHovered.AddDynamic(this, &ThisClass::OnSlottedItemHovered);
		SlottedItem->OnSlottedItemUnhovered.AddDynamic(this, &ThisClass::OnSlottedItemUnhovered);
	}

	return SlottedItem;
}

void UINV_InventoryGrid::AddItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable,
                                        const int32 StackAmount)
{
	// Build the slotted widget and place it on the canvas using factory
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(Item, FragmentTags::GridFragment) };
	const FINV_ImageFragment* ImageFragment { GetFragment<FINV_ImageFragment>(Item, FragmentTags::IconFragment) };
	if (!GridFragment || !ImageFragment) return;

	UINV_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	if (!IsValid(SlottedItem)) return;

	FINV_GridWidgetFactoryConfig Config;
	Config.TileSize = TileSize;
	Config.GridWidth = GridSize.X;
	Config.CanvasPanel = CanvasPanel;

	UINV_GridWidgetFactory::AddSlottedItemToCanvas(SlottedItem, Index, GridFragment, Config);

	SlottedItems.Add(Index, SlottedItem);
}

void UINV_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FINV_GridFragment* GridFragment,
	UINV_SlottedItem* SlottedItem)
{
	FINV_GridWidgetFactoryConfig Config;
	Config.TileSize = TileSize;
	Config.GridWidth = GridSize.X;
	Config.CanvasPanel = CanvasPanel;

	UINV_GridWidgetFactory::AddSlottedItemToCanvas(SlottedItem, Index, GridFragment, Config);
}

void UINV_InventoryGrid::UpdateGridSlots(UINV_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount)
{
	checkf(GridSlots.IsValidIndex(Index), TEXT("Index out of bounds!"));
	UINV_GridItemOperations::UpdateGridSlots(GridSlots, NewItem, Index, bStackableItem, StackAmount, GridSize.X);
}

void UINV_InventoryGrid::ConstructGrid()
{
	// Validate grid dimensions
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("Invalid grid dimensions: %dx%d. Grid size must be positive."), GridSize.X, GridSize.Y);
		return;
	}

	// Setup factory configuration
	FINV_GridWidgetFactoryConfig Config;
	Config.TileSize = TileSize;
	Config.GridWidth = GridSize.X;
	Config.CanvasPanel = CanvasPanel;

	// Create grid slots and place them on the canvas using factory
	GridSlots.Reserve(GridSize.X * GridSize.Y);

	for (int32 j = 0; j < GridSize.Y; ++j)
	{
		for (int32 i = 0; i < GridSize.X; ++i)
		{
			const FIntPoint TilePosition { i, j };
			const int32 Index = UINV_WidgetUtils::GetIndexFromPosition(TilePosition, GridSize.X);

			UINV_GridSlot* GridSlot = UINV_GridWidgetFactory::CreateGridSlot(Index, Config, this, SlotClass);
			if (!IsValid(GridSlot)) continue;

			UINV_GridWidgetFactory::AddGridSlotToCanvas(GridSlot, Index, Config);

			GridSlots.Add(GridSlot);
			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		}
	}
}

void UINV_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	CloseActiveItemPopup();

	// If we have a hover item, try to place it
	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;

	// Resolve what action to perform for empty slot click
	const FINV_GridClickResult ActionResult = UINV_GridClickActionResolver::ResolveEmptySlotClick(
		HoverItem,
		CurrentQueryResult,
		GridSlots,
		GridIndex,
		ItemDropIndex);

	// Execute the resolved action
	switch (ActionResult.Action)
	{
	case EINV_ClickAction::SwapItems:
		// Route to slotted item handler for the target item
		if (GridSlots.IsValidIndex(ActionResult.TargetIndex))
		{
			OnSlottedItemClicked(ActionResult.TargetIndex, MouseEvent);
		}
		break;

	case EINV_ClickAction::PlaceItem:
		PutDownOnIndex(ActionResult.TargetIndex);
		break;

	case EINV_ClickAction::None:
	default:
		// No action
		break;
	}
}

void UINV_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	// Convert hover item into a slotted item.
	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

void UINV_InventoryGrid::ClearHoverItem()
{
	UINV_HoverItemManager::ClearHoverItem(HoverItem);
	ShowCursor();
}

void UINV_InventoryGrid::ShowCursor()
{
	SetCursorWidget(GetVisibleCursorWidget());
}

void UINV_InventoryGrid::HideCursor()
{
	SetCursorWidget(GetHiddenCursorWidget());
}

void UINV_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UINV_InventoryGrid::SetCursorWidget(UUserWidget* CursorWidget)
{
	UINV_HoverItemManager::SetCursorWidget(GetOwningPlayer(), CursorWidget);
}

/**
 * Complex multi-item swap algorithm that handles spatial inventory item placement.
 *
 * Algorithm Overview:
 * 1. Determines target drop location for the hovered item
 * 2. Identifies all items that would be overlapped by placing the hover item
 * 3. Validates that the clicked item is one of the overlapped items (security check)
 * 4. Separates the clicked item (becomes new hover) from other displaced items
 * 5. Simulates grid occupancy to validate all relocations are possible
 * 6. Uses first-fit algorithm to find new positions for displaced items
 * 7. Only commits changes if ALL items can be safely relocated
 *
 * This prevents item loss and ensures grid integrity during complex swaps.
 */
void UINV_InventoryGrid::SwapWithHoverItem(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;
	if (!IsValid(ClickedInventoryItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid()) return;

	UINV_InventoryItem* HoverInventoryItem = HoverItem->GetInventoryItem();
	const FINV_GridFragment* HoverGridFragment { GetFragment<FINV_GridFragment>(HoverInventoryItem, FragmentTags::GridFragment) };
	if (!HoverGridFragment) return;

	// Prefer the computed hover drop index, but fall back to clicked index if needed
	const int32 TargetDropIndex = GridSlots.IsValidIndex(ItemDropIndex) ? ItemDropIndex : GridIndex;

	// Delegate swap planning to transfer handler
	FINV_SwapResult SwapPlan = FINV_ItemTransferHandler::PlanSwapOperation(
		GridSlots,
		GridSize,
		HoverGridFragment->GetGridSize(),
		TargetDropIndex,
		GridIndex);

	if (!SwapPlan.bSuccess)
	{
		// Swap not possible
		return;
	}

	// Store hover item state before modification
	const int32 TempStackCount { HoverItem->GetStackCount() };
	const bool bTempIsStackable { HoverItem->IsStackable() };
	const int32 HoverPreviousIndex = HoverItem->GetPreviousGridIndex();

	// Keep same previous grid index for the newly hovered clicked item
	AssignHoverItem(ClickedInventoryItem, GridIndex, HoverPreviousIndex);

	// Remove all overlapped blockers
	for (const TPair<UINV_InventoryItem*, int32>& ItemToRemove : SwapPlan.ItemsToRemove)
	{
		if (IsValid(ItemToRemove.Key))
		{
			RemoveItemFromGrid(ItemToRemove.Key, ItemToRemove.Value);
		}
	}

	// Place hovered item at destination
	AddItemAtIndex(HoverInventoryItem, SwapPlan.PlacementIndex, bTempIsStackable, TempStackCount);
	UpdateGridSlots(HoverInventoryItem, SwapPlan.PlacementIndex, bTempIsStackable, TempStackCount);

	// Re-home displaced blockers
	for (int32 i = 0; i < SwapPlan.ItemsToRelocate.Num(); ++i)
	{
		const TPair<UINV_InventoryItem*, int32>& Relocation = SwapPlan.ItemsToRelocate[i];
		const int32 StackCount = SwapPlan.RelocationStackCounts[i];
		const bool bStackable = SwapPlan.RelocationStackableFlags[i];

		AddItemAtIndex(Relocation.Key, Relocation.Value, bStackable, StackCount);
		UpdateGridSlots(Relocation.Key, Relocation.Value, bStackable, StackCount);
	}

	// Ensure no stale hover-highlight textures remain after relocation
	RefreshGridSlotVisualsFromAvailability();
}

void UINV_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
{
	UINV_GridStackOperations::SwapStackCounts(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		HoverItem,
		ClickedStackCount,
		HoveredStackCount);
}

void UINV_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	UINV_GridStackOperations::ConsumeHoverItemStacks(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		ClickedStackCount,
		HoveredStackCount);

	ClearHoverItem();
	ShowCursor();

	const FINV_GridFragment* GridFragment { GridSlots[Index]->GetInventoryItem()->GetItemManifest().GetFragmentOfType<FINV_GridFragment>() };
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	HighlightSlots(Index, Dimensions);
}

void UINV_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UINV_GridStackOperations::FillInStack(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		HoverItem,
		FillAmount,
		Remainder);
}

UUserWidget* UINV_InventoryGrid::GetCursorWidget(TObjectPtr<UUserWidget>& CachedWidget,
                                                 const TSubclassOf<UUserWidget>& WidgetClass)
{
	return UINV_HoverItemManager::GetOrCreateCursorWidget(CachedWidget, WidgetClass, GetOwningPlayer());
}

UUserWidget* UINV_InventoryGrid::GetVisibleCursorWidget()
{
	return GetCursorWidget(VisibleCursorWidget, VisibleCursorWidgetClass);
}

UUserWidget* UINV_InventoryGrid::GetHiddenCursorWidget()
{
	return GetCursorWidget(HiddenCursorWidget, HiddenCursorWidgetClass);
}

void UINV_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	
	UINV_GridSlot* GridSlot { GridSlots[GridIndex] };
	if (!GridSlot->GetAvailability()) return;
	GridSlot->SetOccupiedTexture();
} 

void UINV_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	
	UINV_GridSlot* GridSlot { GridSlots[GridIndex] };
	if (!GridSlot->GetAvailability()) return;
	GridSlot->SetUnoccupiedTexture();
}

bool UINV_InventoryGrid::GetRightClickedInventoryItem(int32 Index, UINV_InventoryItem*& RightClickedItem)
{
	if (!GridSlots.IsValidIndex(Index))
	{
		RightClickedItem = nullptr;
		return false;
	}

	RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	// Return true if item was found and is valid
	return IsValid(RightClickedItem);
}

void UINV_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UINV_InventoryItem* RightClickedItem;
	if (!GetRightClickedInventoryItem(Index, RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;
	
	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UINV_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	AssignHoverItem(RightClickedItem, UpperLeftIndex, UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UINV_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UINV_InventoryItem* RightClickedItem;
	if (!GetRightClickedInventoryItem(Index, RightClickedItem)) return;

	Pickup(RightClickedItem, Index);
	DropItem();
}

void UINV_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	UINV_InventoryItem* RightClickedItem;
	if (!GetRightClickedInventoryItem(Index, RightClickedItem)) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UINV_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	InventoryComponent->Server_ConsumeItem(RightClickedItem);
	
	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(RightClickedItem, Index);
	}
}

void UINV_InventoryGrid::OnPopUpMenuInspect(int32 Index)
{
	UINV_InventoryItem* RightClickedItem;
	if (!GetRightClickedInventoryItem(Index, RightClickedItem)) return;

	FVector2D OpenPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	if (IsValid(ItemPopUp))
	{
		if (UCanvasPanelSlot* PopUpSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp))
		{
			OpenPosition = PopUpSlot->GetPosition() + (ItemPopUp->GetBoxSize() / 2.0f);
		}
	}

	UINV_InventoryStatics::ItemInspected(GetOwningPlayer(), RightClickedItem, OpenPosition);
}

void UINV_InventoryGrid::OnSlottedItemHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;

	const UINV_InventoryItem* HoveredInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	if (!IsValid(HoveredInventoryItem)) return;

	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(HoveredInventoryItem, FragmentTags::GridFragment) };
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);

	UINV_InventoryStatics::ForEach2D(GridSlots, GridIndex, Dimensions, GridSize.X, [](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetSelectedTexture();
	});
}

void UINV_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;

	const UINV_InventoryItem* HoveredInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	if (!IsValid(HoveredInventoryItem)) return;

	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(HoveredInventoryItem, FragmentTags::GridFragment) };
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);

	UINV_InventoryStatics::ForEach2D(GridSlots, GridIndex, Dimensions, GridSize.X, [](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
}

void UINV_InventoryGrid::Pickup(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	// Convert a slotted item into a hover item.
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UINV_InventoryGrid::DropItem()
{
	if (!IsValid(HoverItem)) return;
	if (!IsValid(HoverItem->GetInventoryItem())) return;
	
	InventoryComponent->Server_DropItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	
	ClearHoverItem();
	ShowCursor();
}

void UINV_InventoryGrid::AssignHoverItem(UINV_InventoryItem* InventoryItem)
{
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(InventoryItem, FragmentTags::GridFragment) };
	const FINV_ImageFragment* ImageFragment { GetFragment<FINV_ImageFragment>(InventoryItem, FragmentTags::IconFragment) };
	if (!GridFragment || !ImageFragment) return;

	HoverItem = UINV_HoverItemManager::AssignHoverItem(
		HoverItem,
		HoverItemClass,
		InventoryItem,
		GridFragment,
		ImageFragment,
		TileSize,
		GetOwningPlayer(),
		this);
}

void UINV_InventoryGrid::AssignHoverItem(UINV_InventoryItem* InventoryItem, const int32 GridIndex,
	const int32 PreviousGridIndex)
{
	AssignHoverItem(InventoryItem);
	UINV_HoverItemManager::ConfigureHoverItemProperties(HoverItem, GridSlots[GridIndex], InventoryItem, PreviousGridIndex);
}

void UINV_InventoryGrid::RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex)
{
	UINV_GridItemOperations::RemoveItemFromGrid(GridSlots, SlottedItems, InventoryItem, GridIndex, GridSize.X);
}

void UINV_InventoryGrid::AddStacks(const FINV_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;

	// Apply stack changes using utility
	UINV_GridItemOperations::ApplyStackUpdates(GridSlots, SlottedItems, Result);

	// Add new items at indices that didn't have items
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (!Availability.bItemAtIndex)
		{
			AddItemAtIndex(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
			UpdateGridSlots(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

FINV_StackDetails UINV_InventoryGrid::CalculateStackDetails(int32 GridIndex, UINV_InventoryItem* ClickedInventoryItem)
{
	return UINV_GridStackOperations::CalculateStackDetails(
		GridSlots[GridIndex],
		HoverItem,
		ClickedInventoryItem);
}

void UINV_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	CloseActiveItemPopup();

	checkf(GridSlots.IsValidIndex(GridIndex), TEXT("Index out of bounds!"));
	UINV_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	if (!IsValid(ClickedInventoryItem))
	{
		return;
	}

	// Get stack details for potential stack operations
	const FINV_StackDetails StackDetails = CalculateStackDetails(GridIndex, ClickedInventoryItem);

	// Resolve what action to perform
	const FINV_GridClickResult ActionResult = UINV_GridClickActionResolver::ResolveSlottedItemClick(
		HoverItem,
		ClickedInventoryItem,
		MouseEvent,
		GridIndex,
		StackDetails,
		ItemDropIndex);

	// Execute the resolved action
	switch (ActionResult.Action)
	{
	case EINV_ClickAction::Pickup:
		Pickup(ClickedInventoryItem, GridIndex);
		break;

	case EINV_ClickAction::CreatePopup:
		CreateItemPopup(GridIndex);
		break;

	case EINV_ClickAction::SwapStackCounts:
		SwapStackCounts(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
		break;

	case EINV_ClickAction::ConsumeHoverStacks:
		ConsumeHoverItemStacks(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
		break;

	case EINV_ClickAction::FillStack:
		FillInStack(ActionResult.AuxiliaryValue, StackDetails.HoveredStackCount - ActionResult.AuxiliaryValue, GridIndex);
		break;

	case EINV_ClickAction::SwapItems:
		SwapWithHoverItem(ClickedInventoryItem, GridIndex);
		break;

	case EINV_ClickAction::None:
	default:
		// No action
		break;
	}
}

void UINV_InventoryGrid::CreateItemPopup(const int32 GridIndex)
{
	ItemPopUp = UINV_GridPopupManager::CreateItemPopup(
		GridSlots,
		GridIndex,
		ItemPopUpClass,
		OwningCanvasPanel.Get(),
		this,
		GetOwningPlayer());

	if (!IsValid(ItemPopUp)) return;

	// Bind callbacks using dynamic delegates
	ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);
	ItemPopUp->OnInspect.BindDynamic(this, &ThisClass::OnPopUpMenuInspect);
	ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpMenuConsume);
}

bool UINV_InventoryGrid::MatchesCategory(const UINV_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}

bool UINV_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return UINV_GridEventHandler::IsRightClick(MouseEvent);
}

bool UINV_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return UINV_GridEventHandler::IsLeftClick(MouseEvent);
}

bool UINV_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize,
	const FVector2D& Loc)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = !UINV_GridCoordinateCalculator::IsOutsideBounds(BoundaryPos, BoundarySize, Loc);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
		UnHighlightBlockingItems();
		return true;
	}
	return false;
}

bool UINV_InventoryGrid::IsSameStackable(const UINV_InventoryItem* ClickedInventoryItem) const
{
	return UINV_GridStackOperations::IsSameStackable(HoverItem, ClickedInventoryItem);
}

bool UINV_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount,
	const int32 MaxStackSize) const
{
	// Delegate to event handler
	return UINV_GridEventHandler::DetermineStackAction(0, HoveredStackCount, MaxStackSize, RoomInClickedSlot) ==
		FINV_ClickActionResult::EAction::SwapStacks;
}

bool UINV_InventoryGrid::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot) const
{
	// Delegate to event handler
	return UINV_GridEventHandler::DetermineStackAction(0, HoveredStackCount, 0, RoomInClickedSlot) ==
		FINV_ClickActionResult::EAction::MergeStacks;
}

bool UINV_InventoryGrid::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	// Delegate to event handler
	return UINV_GridEventHandler::DetermineStackAction(0, HoveredStackCount, 0, RoomInClickedSlot) ==
		FINV_ClickActionResult::EAction::FillStack;
}
