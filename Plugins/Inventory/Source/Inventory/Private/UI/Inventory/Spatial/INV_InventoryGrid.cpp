// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/INV_InventoryComponent.h"
#include "UI/Inventory/Placement/INV_GridPlacementEngine.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"
#include "UI/Utils/INV_InventoryStatics.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Inventory/Services/INV_GridSwapService.h"
#include "UI/Inventory/Services/INV_GridClickExecutionService.h"
#include "UI/Utils/INV_WidgetUtils.h"
#include "UI/Inventory/Factory/INV_GridWidgetFactory.h"
#include "UI/Inventory/State/INV_GridStateManager.h"
#include "UI/Inventory/Popup/INV_GridPopupManager.h"
#include "UI/Inventory/Stack/INV_GridStackOperations.h"
#include "UI/Inventory/Hover/INV_HoverItemManager.h"
#include "UI/Inventory/Coordinate/INV_GridCoordinateCalculator.h"
#include "UI/Inventory/Click/INV_GridClickActionResolver.h"
#include "UI/Inventory/Items/INV_GridItemOperations.h"
#include "UI/Inventory/Actions/INV_GridPopupActions.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Popup/INV_ItemPopUp.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
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
	// Delegate to the placement engine for pure logic computation
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
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_InventoryGrid_NativeTick);
	Super::NativeTick(MyGeometry, InDeltaTime);
	ClosePopupIfClickedOutside();
	
	// Track the mouse position for hover placement.
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

const FINV_GridFragment* UINV_InventoryGrid::TryGetGridFragmentAtIndex(const int32 Index) const
{
	if (!GridSlots.IsValidIndex(Index)) return nullptr;
	const UINV_InventoryItem* Item = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(Item)) return nullptr;
	return Item->GetCachedGridFragment();
}

FIntPoint UINV_InventoryGrid::GetItemDimensionsOrDefault(const UINV_InventoryItem* Item) const
{
	const FINV_GridFragment* GridFragment = IsValid(Item) ? Item->GetCachedGridFragment() : nullptr;
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
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
	FINV_GridPopupManager::ClosePopupIfClickedOutside(ItemPopUp, GetOwningPlayer());
}

void UINV_InventoryGrid::CloseActiveItemPopup()
{
	FINV_GridPopupManager::CloseActiveItemPopup(ItemPopUp);
}

void UINV_InventoryGrid::OnTileParamsUpdated(const FINV_TileParams& Params)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_InventoryGrid_OnTileParamsUpdated);
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
		FINV_GridIteration::ForEach2D(GridSlots, ItemDropIndex, Dimensions, GridSize.X,
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

	LastGrayedOutUpperLeftIndices = UINV_GridStateManager::HighlightBlockingItems(
		GridSlots, GridSize.X, BlockingUpperLeftIndices,
		[this](const int32 Index) { return TryGetGridFragmentAtIndex(Index); });
}

void UINV_InventoryGrid::UnHighlightBlockingItems()
{
	UINV_GridStateManager::UnHighlightBlockingItems(
		GridSlots, GridSize.X, LastGrayedOutUpperLeftIndices,
		[this](const int32 Index) { return TryGetGridFragmentAtIndex(Index); });
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
	return FINV_GridWidgetFactory::CalculateDrawSize(GridFragment, TileSize);
}

FINV_GridWidgetFactoryConfig UINV_InventoryGrid::CreateFactoryConfig() const
{
	FINV_GridWidgetFactoryConfig Config;
	Config.TileSize = TileSize;
	Config.GridWidth = GridSize.X;
	Config.CanvasPanel = CanvasPanel;
	return Config;
}

UINV_SlottedItem* UINV_InventoryGrid::CreateSlottedItem(UINV_InventoryItem* Item, const bool bStackable, const int32 StackAmount,
	const FINV_GridFragment* GridFragment, const FINV_ImageFragment* ImageFragment, const int32 Index)
{
	return AcquireSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
}

UINV_SlottedItem* UINV_InventoryGrid::AcquireSlottedItem(UINV_InventoryItem* Item, const bool bStackable, const int32 StackAmount,
	const FINV_GridFragment* GridFragment, const FINV_ImageFragment* ImageFragment, const int32 Index)
{
	const FINV_GridWidgetFactoryConfig Config = CreateFactoryConfig();
	UINV_SlottedItem* SlottedItem = nullptr;

	while (SlottedItemPool.Num() > 0 && !IsValid(SlottedItem))
	{
		SlottedItem = SlottedItemPool.Pop(EAllowShrinking::No);
	}

	if (IsValid(SlottedItem))
	{
		FINV_GridWidgetFactory::ConfigureSlottedItem(
			SlottedItem,
			Item,
			bStackable,
			StackAmount,
			GridFragment,
			ImageFragment,
			Index,
			Config);
	}
	else
	{
		SlottedItem = FINV_GridWidgetFactory::CreateSlottedItem(
			Item, bStackable, StackAmount, GridFragment, ImageFragment, Index, Config, this, SlottedItemClass);
	}

	BindSlottedItemDelegates(SlottedItem);
	return SlottedItem;
}

void UINV_InventoryGrid::ReleaseSlottedItem(UINV_SlottedItem* SlottedItem)
{
	if (!IsValid(SlottedItem))
	{
		return;
	}

	SlottedItem->SetInventoryItem(nullptr);
	SlottedItem->SetGridIndex(INDEX_NONE);
	SlottedItem->SetIsStackable(false);
	SlottedItem->UpdateStackCount(0);
	SlottedItem->SetVisibility(ESlateVisibility::Collapsed);
	SlottedItem->RemoveFromParent();

	if (MaxPooledSlottedItems <= 0 || SlottedItemPool.Num() >= MaxPooledSlottedItems)
	{
		return;
	}

	SlottedItemPool.Add(SlottedItem);
}

void UINV_InventoryGrid::BindSlottedItemDelegates(UINV_SlottedItem* SlottedItem)
{
	if (!IsValid(SlottedItem))
	{
		return;
	}

	SlottedItem->OnSlottedItemClicked.RemoveDynamic(this, &ThisClass::OnSlottedItemClicked);
	SlottedItem->OnSlottedItemHovered.RemoveDynamic(this, &ThisClass::OnSlottedItemHovered);
	SlottedItem->OnSlottedItemUnhovered.RemoveDynamic(this, &ThisClass::OnSlottedItemUnhovered);
	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	SlottedItem->OnSlottedItemHovered.AddDynamic(this, &ThisClass::OnSlottedItemHovered);
	SlottedItem->OnSlottedItemUnhovered.AddDynamic(this, &ThisClass::OnSlottedItemUnhovered);
}

void UINV_InventoryGrid::AddItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable,
                                        const int32 StackAmount)
{
	// Build the slotted widget and place it on the canvas using factory
	const FINV_GridFragment* GridFragment { IsValid(Item) ? Item->GetCachedGridFragment() : nullptr };
	const FINV_ImageFragment* ImageFragment { IsValid(Item) ? Item->GetCachedImageFragment() : nullptr };
	if (!GridFragment || !ImageFragment) return;

	UINV_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	if (!IsValid(SlottedItem)) return;

	const FINV_GridWidgetFactoryConfig Config = CreateFactoryConfig();
	FINV_GridWidgetFactory::AddSlottedItemToCanvas(SlottedItem, Index, GridFragment, Config);

	SlottedItems.Add(Index, SlottedItem);
}

void UINV_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FINV_GridFragment* GridFragment,
	UINV_SlottedItem* SlottedItem)
{
	const FINV_GridWidgetFactoryConfig Config = CreateFactoryConfig();
	FINV_GridWidgetFactory::AddSlottedItemToCanvas(SlottedItem, Index, GridFragment, Config);
}

void UINV_InventoryGrid::UpdateGridSlots(UINV_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount)
{
	checkf(GridSlots.IsValidIndex(Index), TEXT("Index out of bounds!"));
	FINV_GridItemOperations::UpdateGridSlots(GridSlots, NewItem, Index, bStackableItem, StackAmount, GridSize.X);
}

void UINV_InventoryGrid::ConstructGrid()
{
	// Validate grid dimensions
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("Invalid grid dimensions: %dx%d. Grid size must be positive."), GridSize.X, GridSize.Y);
		return;
	}

	// Set up factory configuration
	const FINV_GridWidgetFactoryConfig Config = CreateFactoryConfig();

	// Create grid slots and place them on the canvas using factory
	GridSlots.Reserve(GridSize.X * GridSize.Y);

	for (int32 j = 0; j < GridSize.Y; ++j)
	{
		for (int32 i = 0; i < GridSize.X; ++i)
		{
			const FIntPoint TilePosition { i, j };
			const int32 Index = UINV_WidgetUtils::GetIndexFromPosition(TilePosition, GridSize.X);

			UINV_GridSlot* GridSlot = FINV_GridWidgetFactory::CreateGridSlot(Index, Config, this, SlotClass);
			if (!IsValid(GridSlot)) continue;

			FINV_GridWidgetFactory::AddGridSlotToCanvas(GridSlot, Index, Config);

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
	const FINV_GridClickResult ActionResult = FINV_GridClickActionResolver::ResolveEmptySlotClick(
		HoverItem,
		CurrentQueryResult,
		GridSlots,
		GridIndex,
		ItemDropIndex);

	const FINV_GridClickExecutionCallbacks Callbacks
	{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		[this, MouseEvent](const int32 TargetIndex)
		{
			if (GridSlots.IsValidIndex(TargetIndex))
			{
				OnSlottedItemClicked(TargetIndex, MouseEvent);
			}
		},
		[this](const int32 TargetIndex)
		{
			PutDownOnIndex(TargetIndex);
		}
	};

	FINV_GridClickExecutionService::ExecuteGridSlotClick(ActionResult, Callbacks);
}

void UINV_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	// Convert a hover item into a slotted item.
	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

void UINV_InventoryGrid::ClearHoverItem()
{
	FINV_HoverItemManager::ClearHoverItem(HoverItem);
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
	FINV_HoverItemManager::SetCursorWidget(GetOwningPlayer(), CursorWidget);
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
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_InventoryGrid_SwapWithHoverItem);
	if (!IsValid(HoverItem)) return;
	if (!IsValid(ClickedInventoryItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid()) return;

	UINV_InventoryItem* HoverInventoryItem = HoverItem->GetInventoryItem();
	const FINV_GridFragment* HoverGridFragment { IsValid(HoverInventoryItem) ? HoverInventoryItem->GetCachedGridFragment() : nullptr };
	if (!HoverGridFragment) return;

	const int32 TempStackCount { HoverItem->GetStackCount() };
	const bool bTempIsStackable { HoverItem->IsStackable() };
	const int32 HoverPreviousIndex = HoverItem->GetPreviousGridIndex();

	const FINV_GridSwapCallbacks Callbacks{
		[this](UINV_InventoryItem* Item, const int32 NewGridIndex, const int32 PreviousGridIndex)
		{
			AssignHoverItem(Item, NewGridIndex, PreviousGridIndex);
		},
		[this](UINV_InventoryItem* Item, const int32 RemoveIndex)
		{
			RemoveItemFromGrid(Item, RemoveIndex);
		},
		[this](UINV_InventoryItem* Item, const int32 AddIndex, const bool bStackable, const int32 StackCount)
		{
			AddItemAtIndex(Item, AddIndex, bStackable, StackCount);
		},
		[this](UINV_InventoryItem* Item, const int32 UpdateIndex, const bool bStackable, const int32 StackCount)
		{
			UpdateGridSlots(Item, UpdateIndex, bStackable, StackCount);
		},
		[this]()
		{
			RefreshGridSlotVisualsFromAvailability();
		}
	};

	const bool bSwapExecuted = FINV_GridSwapService::ExecuteSwapWithHoverItem(
		GridSlots,
		GridSize,
		ItemDropIndex,
		ClickedInventoryItem,
		GridIndex,
		HoverInventoryItem,
		HoverGridFragment->GetGridSize(),
		TempStackCount,
		bTempIsStackable,
		HoverPreviousIndex,
		Callbacks);
	if (!bSwapExecuted) return;
}

void UINV_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
{
	FINV_GridStackOperations::SwapStackCounts(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		HoverItem,
		ClickedStackCount,
		HoveredStackCount);
}

void UINV_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	FINV_GridStackOperations::ConsumeHoverItemStacks(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		ClickedStackCount,
		HoveredStackCount);

	ClearHoverItem();
	ShowCursor();

	const FINV_GridFragment* GridFragment { GridSlots[Index]->GetInventoryItem()->GetCachedGridFragment() };
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	HighlightSlots(Index, Dimensions);
}

void UINV_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	FINV_GridStackOperations::FillInStack(
		GridSlots[Index],
		SlottedItems.FindChecked(Index),
		HoverItem,
		FillAmount,
		Remainder);
}

UUserWidget* UINV_InventoryGrid::GetCursorWidget(TObjectPtr<UUserWidget>& CachedWidget,
                                                 const TSubclassOf<UUserWidget>& WidgetClass)
{
	return FINV_HoverItemManager::GetOrCreateCursorWidget(CachedWidget, WidgetClass, GetOwningPlayer());
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
	// Return true if the item was found and is valid
	return IsValid(RightClickedItem);
}

void UINV_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UINV_InventoryItem* RightClickedItem;
	if (!GetRightClickedInventoryItem(Index, RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;

	UINV_GridPopupActions::ExecuteSplit(
		GridSlots,
		SlottedItems,
		RightClickedItem,
		Index,
		SplitAmount,
		[this](UINV_InventoryItem* Item, int32 GridIndex, int32 PrevIndex)
		{
			AssignHoverItem(Item, GridIndex, PrevIndex);
		},
		[this](UINV_HoverItem*, int32 StackCount)
		{
			if (IsValid(HoverItem))
			{
				HoverItem->UpdateStackCount(StackCount);
			}
		});
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

	const int32 NewStackCount = UINV_GridPopupActions::ExecuteConsume(
		GridSlots,
		SlottedItems,
		RightClickedItem,
		Index);

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

	const FIntPoint Dimensions = GetItemDimensionsOrDefault(HoveredInventoryItem);

	FINV_GridIteration::ForEach2D(GridSlots, GridIndex, Dimensions, GridSize.X, [](UINV_GridSlot* GridSlot)
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

	const FIntPoint Dimensions = GetItemDimensionsOrDefault(HoveredInventoryItem);

	FINV_GridIteration::ForEach2D(GridSlots, GridIndex, Dimensions, GridSize.X, [](UINV_GridSlot* GridSlot)
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
	const FINV_GridFragment* GridFragment { IsValid(InventoryItem) ? InventoryItem->GetCachedGridFragment() : nullptr };
	const FINV_ImageFragment* ImageFragment { IsValid(InventoryItem) ? InventoryItem->GetCachedImageFragment() : nullptr };
	if (!GridFragment || !ImageFragment) return;

	HoverItem = FINV_HoverItemManager::AssignHoverItem(
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
	FINV_HoverItemManager::ConfigureHoverItemProperties(HoverItem, GridSlots[GridIndex], InventoryItem, PreviousGridIndex);
}

void UINV_InventoryGrid::RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex)
{
	TObjectPtr<UINV_SlottedItem> PooledSlottedItem;
	if (SlottedItems.RemoveAndCopyValue(GridIndex, PooledSlottedItem))
	{
		ReleaseSlottedItem(PooledSlottedItem);
	}

	FINV_GridItemOperations::RemoveItemFromGrid(GridSlots, SlottedItems, InventoryItem, GridIndex, GridSize.X);
}

void UINV_InventoryGrid::AddStacks(const FINV_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;

	// Apply stack changes using utility
	FINV_GridItemOperations::ApplyStackUpdates(GridSlots, SlottedItems, Result);

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
	return FINV_GridStackOperations::CalculateStackDetails(
		GridSlots[GridIndex],
		HoverItem,
		ClickedInventoryItem);
}

void UINV_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_InventoryGrid_OnSlottedItemClicked);
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
	const FINV_GridClickResult ActionResult = FINV_GridClickActionResolver::ResolveSlottedItemClick(
		HoverItem,
		ClickedInventoryItem,
		MouseEvent,
		GridIndex,
		StackDetails,
		ItemDropIndex);

	const FINV_GridClickExecutionCallbacks Callbacks
	{
		[this](UINV_InventoryItem* Item, const int32 Index)
		{
			Pickup(Item, Index);
		},
		[this](const int32 Index)
		{
			CreateItemPopup(Index);
		},
		[this](const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
		{
			SwapStackCounts(ClickedStackCount, HoveredStackCount, Index);
		},
		[this](const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
		{
			ConsumeHoverItemStacks(ClickedStackCount, HoveredStackCount, Index);
		},
		[this](const int32 FillAmount, const int32 Remainder, const int32 Index)
		{
			FillInStack(FillAmount, Remainder, Index);
		},
		[this](UINV_InventoryItem* Item, const int32 Index)
		{
			SwapWithHoverItem(Item, Index);
		},
		nullptr,
		nullptr
	};

	FINV_GridClickExecutionService::ExecuteSlottedItemClick(
		ActionResult,
		StackDetails,
		ClickedInventoryItem,
		GridIndex,
		Callbacks);
}

void UINV_InventoryGrid::CreateItemPopup(const int32 GridIndex)
{
	ItemPopUp = FINV_GridPopupManager::CreateItemPopup(
		ItemPopUp,
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


