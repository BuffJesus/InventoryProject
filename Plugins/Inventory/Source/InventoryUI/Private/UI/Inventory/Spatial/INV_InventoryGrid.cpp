// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "Items/Manifest/INV_ItemManifest.h"
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
#include "Controllers/INV_PlayerController.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformTime.h"

namespace
{
FPointerEvent MakeSimulatedLeftMouseClickEvent()
{
	return FPointerEvent(
		0u,
		0u,
		FVector2D::ZeroVector,
		FVector2D::ZeroVector,
		TSet<FKey> { EKeys::LeftMouseButton },
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
}
}

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
	SetIsFocusable(true);
	
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

	const bool bHasHoverItem = IsValid(HoverItem);
	const bool bHasOpenPopup = HasOpenItemPopup();

	// Idle fast-path: no drag interaction and no active popup to manage.
	if (!bHasHoverItem && !bHasOpenPopup)
	{
		return;
	}

	if (bHasOpenPopup)
	{
		ClosePopupIfClickedOutside();
	}

	// No hover item means no placement/highlight updates are required this frame.
	if (!bHasHoverItem)
	{
		return;
	}
	
	// Track the mouse position for hover placement.
	const FVector2D CanvasPos = UINV_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D CanvasSize = UINV_WidgetUtils::GetWidgetSize(CanvasPanel);
	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	// Skip redundant hover placement computation when tick inputs are effectively unchanged.
	if (bHasLastTickInputs)
	{
		const bool bSameCanvasPos = CanvasPos.Equals(LastTickCanvasPos, KINDA_SMALL_NUMBER);
		const bool bSameCanvasSize = CanvasSize.Equals(LastTickCanvasSize, KINDA_SMALL_NUMBER);
		const bool bSameMousePos = MousePos.Equals(LastTickMousePos, KINDA_SMALL_NUMBER);
		if (bSameCanvasPos && bSameCanvasSize && bSameMousePos)
		{
			return;
		}
	}

	LastTickCanvasPos = CanvasPos;
	LastTickCanvasSize = CanvasSize;
	LastTickMousePos = MousePos;
	bHasLastTickInputs = true;
	
	if (CursorExitedCanvas(CanvasPos, CanvasSize, MousePos))
	{
		return;
	}
	
	UpdateTileParams(CanvasPos, MousePos);
}

FIntPoint UINV_InventoryGrid::CalculateHoverCoordinates(const FVector2D& CanvasPos, const FVector2D& MousePos) const
{
	return FINV_GridCoordinateCalculator::CalculateHoverCoordinates(CanvasPos, MousePos, TileSize);
}

EINV_TileQuadrant UINV_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPos, const FVector2D& MousePos) const
{
	return FINV_GridCoordinateCalculator::CalculateTileQuadrant(CanvasPos, MousePos, TileSize);
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
	if (!IsValid(HoverItem)) return;
	
	// Calculate tile quadrant, index, and coords
	const FIntPoint HoveredTileCoords { CalculateHoverCoordinates(CanvasPos, MousePos) };
	
	LastTileParams = TileParams;
	TileParams.TileCoordinates = HoveredTileCoords;
	TileParams.TileIndex = UINV_WidgetUtils::GetIndexFromPosition(HoveredTileCoords, GridSize.X);
	TileParams.TileQuadrant = CalculateTileQuadrant(CanvasPos, MousePos);

	if (TileParams == LastTileParams) return;
	
	OnTileParamsUpdated(TileParams);
}

void UINV_InventoryGrid::ClosePopupIfClickedOutside()
{
	if (!HasOpenItemPopup())
	{
		return;
	}

	// Ignore the input frame that opened the popup to avoid immediate close.
	if (LastPopupOpenedRealtimeSeconds > 0.0 && (FPlatformTime::Seconds() - LastPopupOpenedRealtimeSeconds) < 0.05)
	{
		return;
	}

	FINV_GridPopupManager::ClosePopupIfClickedOutside(ItemPopUp, GetOwningPlayer());
}

void UINV_InventoryGrid::CloseActiveItemPopup()
{
	FINV_GridPopupManager::CloseActiveItemPopup(ItemPopUp);
}

void UINV_InventoryGrid::CloseItemPopup()
{
	CloseActiveItemPopup();
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

	ApplyHoverPlacementVisuals(Dimensions);
}

void UINV_InventoryGrid::ApplyHoverPlacementVisuals(const FIntPoint& Dimensions)
{
	if (CurrentQueryResult.bHasSpace)
	{
		// Free space: highlight potential drop area.
		UnHighlightBlockingItems();
		HighlightSlots(ItemDropIndex, Dimensions);
		return;
	}

	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	HighlightBlockingItems(CurrentQueryResult.BlockingUpperLeftIndices);
	HighlightHoverFootprintSelection(Dimensions);
}

void UINV_InventoryGrid::HighlightHoverFootprintSelection(const FIntPoint& Dimensions)
{
	// Show the exact hovered-item placement footprint with a distinct visual.
	if (!FINV_GridPlacementEngine::IsInGridBounds(ItemDropIndex, Dimensions, GridSize))
	{
		return;
	}

	FINV_GridIteration::ForEach2D(GridSlots, ItemDropIndex, Dimensions, GridSize.X,
		[](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetSelectedTexture();
	});
	LastHighlightedIndex = ItemDropIndex;
	LastHighlightedDimensions = Dimensions;
}

void UINV_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	FINV_GridStateManager::HighlightSlots(GridSlots, GridSize.X, Index, Dimensions, bMouseWithinCanvas);
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void UINV_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	FINV_GridStateManager::UnHighlightSlots(GridSlots, GridSize.X, Index, Dimensions);
}

void UINV_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EINV_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	FINV_GridStateManager::ChangeSlotState(GridSlots, GridSize.X, Index, Dimensions, GridSlotState);
	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

void UINV_InventoryGrid::HighlightBlockingItems(const TArray<int32>& BlockingUpperLeftIndices)
{
	UnHighlightBlockingItems();
	LastHighlightedIndex = INDEX_NONE;
	LastHighlightedDimensions = FIntPoint::ZeroValue;

	LastGrayedOutUpperLeftIndices = FINV_GridStateManager::HighlightBlockingItems(
		GridSlots, GridSize.X, BlockingUpperLeftIndices,
		[this](const int32 Index) { return TryGetGridFragmentAtIndex(Index); });
}

void UINV_InventoryGrid::UnHighlightBlockingItems()
{
	FINV_GridStateManager::UnHighlightBlockingItems(
		GridSlots, GridSize.X, LastGrayedOutUpperLeftIndices,
		[this](const int32 Index) { return TryGetGridFragmentAtIndex(Index); });
	LastGrayedOutUpperLeftIndices.Reset();
}

void UINV_InventoryGrid::RefreshGridSlotVisualsFromAvailability()
{
	FINV_GridStateManager::RefreshAllSlotVisuals(GridSlots);
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
		PlaceItemAtIndex(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

FReply UINV_InventoryGrid::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply SuperReply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);

	return SuperReply.IsEventHandled() ? MoveTemp(SuperReply) : FReply::Unhandled();
}

void UINV_InventoryGrid::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	ApplyControllerSelectionVisual();
}

void UINV_InventoryGrid::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	ClearControllerSelectionVisual();
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

void UINV_InventoryGrid::PlaceItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable,
	const int32 StackAmount)
{
	AddItemAtIndex(Item, Index, bStackable, StackAmount);
	UpdateGridSlots(Item, Index, bStackable, StackAmount);
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
		UE_LOG(LogTemp, Error, TEXT("Invalid grid dimensions: %dx%d. Grid size must be positive."), GridSize.X, GridSize.Y);
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

	if (GridSlots.Num() > 0)
	{
		ControllerSelectedIndex = 0;
	}
}

void UINV_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	CloseActiveItemPopup();

	// If we have a hover item, try to place it
	if (!IsValid(HoverItem)) return;
	// Guard against same-frame duplicate click routing (observed with simulated controller mouse input).
	if ((FPlatformTime::Seconds() - LastHoverAssignedRealtimeSeconds) < 0.04)
	{
		return;
	}
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;

	// Resolve what action to perform for empty slot click
	const FINV_GridClickResult ActionResult = FINV_GridClickActionResolver::ResolveEmptySlotClick(
		HoverItem,
		CurrentQueryResult,
		GridSlots,
		GridIndex,
		ItemDropIndex);

	FINV_GridClickExecutionService::ExecuteGridSlotClick(ActionResult, BuildGridSlotClickCallbacks(MouseEvent));
}

void UINV_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	// Convert a hover item into a slotted item.
	UINV_InventoryItem* HoverInventoryItem = GetHoverInventoryItem();
	if (!IsValid(HoverInventoryItem)) return;
	PlaceItemAtIndex(HoverInventoryItem, Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

void UINV_InventoryGrid::ClearHoverItem()
{
	FINV_HoverItemManager::ClearHoverItem(HoverItem);
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UnHighlightBlockingItems();
	bHasLastTickInputs = false;
	LastHoveredSlottedIndex = INDEX_NONE;
	ShowCursor();
}

bool UINV_InventoryGrid::ReturnHoverItemToPreviousSlot()
{
	if (!IsValid(HoverItem)) return false;
	UINV_InventoryItem* HoveredInventoryItem = GetHoverInventoryItem();
	if (!IsValid(HoveredInventoryItem)) return false;

	const int32 PreviousGridIndex = HoverItem->GetPreviousGridIndex();
	if (GridSlots.IsValidIndex(PreviousGridIndex) && !GridSlots[PreviousGridIndex]->GetInventoryItem().IsValid())
	{
		PutDownOnIndex(PreviousGridIndex);
		return true;
	}

	const FINV_SlotAvailabilityResult Result = HasRoomForItem(HoveredInventoryItem);
	if (Result.SlotAvailabilities.Num() > 0)
	{
		AddItemToIndices(Result, HoveredInventoryItem);
		ClearHoverItem();
		return true;
	}

	return false;
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
		BuildGridSwapCallbacks());
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
	ControllerSelectedIndex = GridIndex;
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	
	UINV_GridSlot* GridSlot { GridSlots[GridIndex] };
	if (!GridSlot->GetAvailability()) return;
	GridSlot->SetOccupiedTexture();
} 

void UINV_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	
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

	FINV_GridPopupActions::ExecuteSplit(
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

	const int32 NewStackCount = FINV_GridPopupActions::ExecuteConsume(
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
	ControllerSelectedIndex = GridIndex;
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (LastHoveredSlottedIndex == GridIndex) return;

	const UINV_InventoryItem* HoveredInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	if (!IsValid(HoveredInventoryItem)) return;
	UINV_InventoryStatics::ItemHovered(GetOwningPlayer(), const_cast<UINV_InventoryItem*>(HoveredInventoryItem));
	ApplyItemFootprintVisualAtIndex(GridIndex, true);

	LastHoveredSlottedIndex = GridIndex;
}

void UINV_InventoryGrid::OnSlottedItemUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());

	const UINV_InventoryItem* HoveredInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	if (!IsValid(HoveredInventoryItem)) return;
	ApplyItemFootprintVisualAtIndex(GridIndex, false);

	if (LastHoveredSlottedIndex == GridIndex)
	{
		LastHoveredSlottedIndex = INDEX_NONE;
	}
}

void UINV_InventoryGrid::Pickup(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	// Convert a slotted item into a hover item.
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UINV_InventoryGrid::DropItem()
{
	UINV_InventoryItem* HoverInventoryItem = GetHoverInventoryItem();
	if (!IsValid(HoverInventoryItem)) return;
	
	InventoryComponent->Server_DropItem(HoverInventoryItem, HoverItem->GetStackCount());
	
	ClearHoverItem();
}

UINV_InventoryItem* UINV_InventoryGrid::GetHoverInventoryItem() const
{
	if (!IsValid(HoverItem)) return nullptr;
	return HoverItem->GetInventoryItem();
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
		this,
		TOptional<FVector2D>());

	FinalizeHoverItemAssignment();
}

void UINV_InventoryGrid::AssignHoverItem(UINV_InventoryItem* InventoryItem, const int32 GridIndex,
	const int32 PreviousGridIndex)
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
		this,
		ComputeSourceViewportCenterForGridIndex(GridIndex));
	FinalizeHoverItemAssignment();

	if (GridSlots.IsValidIndex(GridIndex) && IsValid(GridSlots[GridIndex]))
	{
		FINV_HoverItemManager::ConfigureHoverItemProperties(HoverItem, GridSlots[GridIndex], InventoryItem, PreviousGridIndex);
	}
}

void UINV_InventoryGrid::RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex)
{
	TObjectPtr<UINV_SlottedItem> PooledSlottedItem;
	if (SlottedItems.RemoveAndCopyValue(GridIndex, PooledSlottedItem))
	{
		ReleaseSlottedItem(PooledSlottedItem);
	}

	FINV_GridItemOperations::RemoveItemFromGrid(GridSlots, InventoryItem, GridIndex, GridSize.X);
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
			PlaceItemAtIndex(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

FINV_StackDetails UINV_InventoryGrid::CalculateStackDetails(int32 GridIndex, UINV_InventoryItem* ClickedInventoryItem)
{
	if (!GridSlots.IsValidIndex(GridIndex))
	{
		return FINV_StackDetails{};
	}

	return FINV_GridStackOperations::CalculateStackDetails(
		GridSlots[GridIndex],
		HoverItem,
		ClickedInventoryItem);
}

FINV_GridClickExecutionCallbacks UINV_InventoryGrid::BuildGridSlotClickCallbacks(const FPointerEvent& MouseEvent)
{
	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.RouteToSlottedClick = [this, MouseEvent](const int32 TargetIndex)
	{
		if (GridSlots.IsValidIndex(TargetIndex))
		{
			OnSlottedItemClicked(TargetIndex, MouseEvent);
		}
	};
	Callbacks.PlaceItem = [this](const int32 TargetIndex)
	{
		PutDownOnIndex(TargetIndex);
	};
	return Callbacks;
}

FINV_GridClickExecutionCallbacks UINV_InventoryGrid::BuildSlottedItemClickCallbacks()
{
	FINV_GridClickExecutionCallbacks Callbacks;
	Callbacks.Pickup = [this](UINV_InventoryItem* Item, const int32 Index)
	{
		Pickup(Item, Index);
	};
	Callbacks.CreatePopup = [this](const int32 Index)
	{
		CreateItemPopup(Index);
	};
	Callbacks.SwapStackCounts = [this](const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
	{
		SwapStackCounts(ClickedStackCount, HoveredStackCount, Index);
	};
	Callbacks.ConsumeHoverStacks = [this](const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
	{
		ConsumeHoverItemStacks(ClickedStackCount, HoveredStackCount, Index);
	};
	Callbacks.FillStack = [this](const int32 FillAmount, const int32 Remainder, const int32 Index)
	{
		FillInStack(FillAmount, Remainder, Index);
	};
	Callbacks.SwapItems = [this](UINV_InventoryItem* Item, const int32 Index)
	{
		SwapWithHoverItem(Item, Index);
	};
	return Callbacks;
}

FINV_GridSwapCallbacks UINV_InventoryGrid::BuildGridSwapCallbacks()
{
	FINV_GridSwapCallbacks Callbacks;
	Callbacks.AssignHoverItem = [this](UINV_InventoryItem* Item, const int32 NewGridIndex, const int32 PreviousGridIndex)
	{
		AssignHoverItem(Item, NewGridIndex, PreviousGridIndex);
	};
	Callbacks.RemoveItemFromGrid = [this](UINV_InventoryItem* Item, const int32 RemoveIndex)
	{
		RemoveItemFromGrid(Item, RemoveIndex);
	};
	Callbacks.AddItemAtIndex = [this](UINV_InventoryItem* Item, const int32 AddIndex, const bool bStackable, const int32 StackCount)
	{
		AddItemAtIndex(Item, AddIndex, bStackable, StackCount);
	};
	Callbacks.UpdateGridSlots = [this](UINV_InventoryItem* Item, const int32 UpdateIndex, const bool bStackable, const int32 StackCount)
	{
		UpdateGridSlots(Item, UpdateIndex, bStackable, StackCount);
	};
	Callbacks.RefreshGridSlotVisuals = [this]()
	{
		RefreshGridSlotVisualsFromAvailability();
	};
	return Callbacks;
}

void UINV_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_InventoryGrid_OnSlottedItemClicked);
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	LastHoveredSlottedIndex = INDEX_NONE;
	CloseActiveItemPopup();
	ControllerSelectedIndex = GridIndex;

	if (!GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}
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

	FINV_GridClickExecutionService::ExecuteSlottedItemClick(
		ActionResult,
		StackDetails,
		ClickedInventoryItem,
		GridIndex,
		BuildSlottedItemClickCallbacks());
}

bool UINV_InventoryGrid::HandleControllerConfirm()
{
	if (!EnsureControllerSelectionInitialized()) return false;
	if (TryHandleControllerConfirmOnOccupiedSelection()) return true;
	return TryHandleControllerConfirmHoverPlacement();
}

bool UINV_InventoryGrid::HandleControllerBack()
{
	if (HasOpenItemPopup())
	{
		CloseItemPopup();
		return true;
	}

	if (IsValid(HoverItem))
	{
		return ReturnHoverItemToPreviousSlot();
	}

	return false;
}

bool UINV_InventoryGrid::HandleControllerContext()
{
	if (!EnsureControllerSelectionInitialized()) return false;
	if (!GridSlots[ControllerSelectedIndex]->GetInventoryItem().IsValid()) return false;

	CreateItemPopup(ResolveControllerActionIndex(ControllerSelectedIndex));
	return true;
}

bool UINV_InventoryGrid::MoveControllerSelection(const FIntPoint& Delta)
{
	if (GridSlots.Num() == 0) return false;

	EnsureControllerSelectionInitialized();

	int32 CurrentX = ControllerSelectedIndex % GridSize.X;
	int32 CurrentY = ControllerSelectedIndex / GridSize.X;

	int32 MaxX = GridSize.X - 1;
	int32 MaxY = GridSize.Y - 1;
	GetControllerSelectionLimits(MaxX, MaxY);

	CurrentX = FMath::Clamp(CurrentX, 0, MaxX);
	CurrentY = FMath::Clamp(CurrentY, 0, MaxY);

	const int32 SpanX = MaxX + 1;
	const int32 SpanY = MaxY + 1;
	int32 TargetX = CurrentX + Delta.X;
	int32 TargetY = CurrentY + Delta.Y;

	// When navigating with controller and not holding an item, step over entire occupied footprints.
	if (!IsValid(HoverItem) && GridSlots.IsValidIndex(ControllerSelectedIndex) && GridSlots[ControllerSelectedIndex]->GetInventoryItem().IsValid())
	{
		const int32 AnchorIndex = ResolveControllerActionIndex(ControllerSelectedIndex);
		const int32 AnchorX = AnchorIndex % GridSize.X;
		const int32 AnchorY = AnchorIndex / GridSize.X;
		const UINV_InventoryItem* SelectedItem = GridSlots[AnchorIndex]->GetInventoryItem().Get();
		const FIntPoint Dimensions = GetItemDimensionsOrDefault(SelectedItem);
		const int32 CurrentOffsetX = FMath::Clamp(CurrentX - AnchorX, 0, FMath::Max(0, Dimensions.X - 1));
		const int32 CurrentOffsetY = FMath::Clamp(CurrentY - AnchorY, 0, FMath::Max(0, Dimensions.Y - 1));

		if (Delta.X > 0)
		{
			TargetX = AnchorX + Dimensions.X;
			TargetY = AnchorY + CurrentOffsetY;
		}
		else if (Delta.X < 0)
		{
			TargetX = AnchorX - 1;
			TargetY = AnchorY + CurrentOffsetY;
		}
		else if (Delta.Y > 0)
		{
			TargetY = AnchorY + Dimensions.Y;
			TargetX = AnchorX + CurrentOffsetX;
		}
		else if (Delta.Y < 0)
		{
			TargetY = AnchorY - 1;
			TargetX = AnchorX + CurrentOffsetX;
		}
	}

	int32 NewX = SpanX > 0 ? (TargetX % SpanX + SpanX) % SpanX : 0;
	int32 NewY = SpanY > 0 ? (TargetY % SpanY + SpanY) % SpanY : 0;

	// While dragging with controller, skip across blocking footprints so lateral navigation
	// does not require repeated presses through large occupied regions.
	if (IsValid(HoverItem))
	{
		const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
		for (int32 Attempt = 0; Attempt < 6; ++Attempt)
		{
			const FINV_SpaceQueryResult Query = FINV_GridPlacementEngine::CheckHoverPosition(
				GridSlots,
				GridSize,
				FIntPoint(NewX, NewY),
				HoverDimensions);
			if (Query.bHasSpace || Query.BlockingUpperLeftIndices.Num() == 0)
			{
				break;
			}

			int32 BlockerIndex = Query.BlockingUpperLeftIndices[0];
			for (const int32 CandidateIndex : Query.BlockingUpperLeftIndices)
			{
				if (!GridSlots.IsValidIndex(CandidateIndex) || !GridSlots.IsValidIndex(BlockerIndex))
				{
					continue;
				}

				if (Delta.X > 0 && CandidateIndex % GridSize.X > BlockerIndex % GridSize.X)
				{
					BlockerIndex = CandidateIndex;
				}
				else if (Delta.X < 0 && CandidateIndex % GridSize.X < BlockerIndex % GridSize.X)
				{
					BlockerIndex = CandidateIndex;
				}
				else if (Delta.Y > 0 && CandidateIndex / GridSize.X > BlockerIndex / GridSize.X)
				{
					BlockerIndex = CandidateIndex;
				}
				else if (Delta.Y < 0 && CandidateIndex / GridSize.X < BlockerIndex / GridSize.X)
				{
					BlockerIndex = CandidateIndex;
				}
			}
			if (!GridSlots.IsValidIndex(BlockerIndex))
			{
				break;
			}

			const int32 PrevX = NewX;
			const int32 PrevY = NewY;
			const int32 BlockerX = BlockerIndex % GridSize.X;
			const int32 BlockerY = BlockerIndex / GridSize.X;
			const UINV_InventoryItem* BlockingItem = GridSlots[BlockerIndex]->GetInventoryItem().Get();
			const FIntPoint BlockerDimensions = GetItemDimensionsOrDefault(BlockingItem);

			if (Delta.X > 0)
			{
				NewX = FMath::Clamp(BlockerX + BlockerDimensions.X, 0, MaxX);
			}
			else if (Delta.X < 0)
			{
				NewX = FMath::Clamp(BlockerX - HoverDimensions.X, 0, MaxX);
			}
			else if (Delta.Y > 0)
			{
				NewY = FMath::Clamp(BlockerY + BlockerDimensions.Y, 0, MaxY);
			}
			else if (Delta.Y < 0)
			{
				NewY = FMath::Clamp(BlockerY - HoverDimensions.Y, 0, MaxY);
			}

			if (PrevX == NewX && PrevY == NewY)
			{
				break;
			}
		}
	}

	SetControllerSelectedIndex(UINV_WidgetUtils::GetIndexFromPosition(FIntPoint(NewX, NewY), GridSize.X));
	return true;
}

void UINV_InventoryGrid::ApplyControllerSelectionVisual()
{
	if (IsValid(HoverItem))
	{
		RefreshControllerHoverPlacementVisual();
		return;
	}

	if (GridSlots.Num() == 0) return;
	if (!IsControllerSelectedIndexValid())
	{
		ControllerSelectedIndex = 0;
	}

	if (!GridSlots.IsValidIndex(ControllerSelectedIndex)) return;
	ApplyControllerVisualForSelectedIndex(true);
}

void UINV_InventoryGrid::ClearControllerSelectionVisual()
{
	if (IsValid(HoverItem))
	{
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
		UnHighlightBlockingItems();
		return;
	}

	if (!GridSlots.IsValidIndex(ControllerSelectedIndex)) return;
	ApplyControllerVisualForSelectedIndex(false);
}

void UINV_InventoryGrid::SetControllerSelectedIndex(int32 NewIndex)
{
	if (!GridSlots.IsValidIndex(NewIndex)) return;
	NewIndex = ClampControllerAnchorIndexForHover(NewIndex);
	if (ControllerSelectedIndex == NewIndex)
	{
		ApplyControllerSelectionVisual();
		return;
	}

	ClearControllerSelectionVisual();
	ControllerSelectedIndex = NewIndex;
	ApplyControllerSelectionVisual();
	PositionHoverItemAtGridIndex(ControllerSelectedIndex);
}

bool UINV_InventoryGrid::IsControllerSelectedIndexValid() const
{
	return GridSlots.IsValidIndex(ControllerSelectedIndex);
}

int32 UINV_InventoryGrid::GetControllerSelectedIndex() const
{
	return ControllerSelectedIndex;
}

bool UINV_InventoryGrid::EnsureControllerSelectionInitialized()
{
	if (GridSlots.Num() == 0)
	{
		return false;
	}

	if (!IsControllerSelectedIndexValid())
	{
		ControllerSelectedIndex = 0;
		ApplyControllerSelectionVisual();
	}

	return IsControllerSelectedIndexValid();
}

void UINV_InventoryGrid::GetControllerSelectionLimits(int32& OutMaxX, int32& OutMaxY) const
{
	OutMaxX = GridSize.X - 1;
	OutMaxY = GridSize.Y - 1;

	if (!IsValid(HoverItem))
	{
		return;
	}

	const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
	OutMaxX = FMath::Max(0, GridSize.X - HoverDimensions.X);
	OutMaxY = FMath::Max(0, GridSize.Y - HoverDimensions.Y);
}

bool UINV_InventoryGrid::TryHandleControllerConfirmOnOccupiedSelection()
{
	if (!IsControllerSelectedIndexValid()) return false;
	if (!GridSlots[ControllerSelectedIndex]->GetInventoryItem().IsValid()) return false;

	const int32 ActionGridIndex = ResolveControllerActionIndex(ControllerSelectedIndex);
	OnSlottedItemClicked(ActionGridIndex, MakeSimulatedLeftMouseClickEvent());
	ApplyControllerSelectionVisual();
	return true;
}

bool UINV_InventoryGrid::TryHandleControllerConfirmHoverPlacement()
{
	if (!IsValid(HoverItem)) return false;
	if (!IsControllerSelectedIndexValid()) return false;

	ControllerSelectedIndex = ClampControllerAnchorIndexForHover(ControllerSelectedIndex);

	// Use the selected slot as the drop anchor when using controller input.
	UINV_InventoryItem* HoverInventoryItem = GetHoverInventoryItem();
	const FIntPoint HoverDimensions = GetItemDimensionsOrDefault(HoverInventoryItem);
	const FIntPoint SelectedCoordinates(
		ControllerSelectedIndex % GridSize.X,
		ControllerSelectedIndex / GridSize.X);
	const FINV_SpaceQueryResult SpaceQueryResult = FINV_GridPlacementEngine::CheckHoverPosition(
		GridSlots,
		GridSize,
		SelectedCoordinates,
		HoverDimensions);
	if (!SpaceQueryResult.bHasSpace) return true;

	ItemDropIndex = ControllerSelectedIndex;
	PutDownOnIndex(ControllerSelectedIndex);
	ApplyControllerSelectionVisual();
	return true;
}

void UINV_InventoryGrid::FinalizeHoverItemAssignment()
{
	LastHoverAssignedRealtimeSeconds = FPlatformTime::Seconds();
	bHasLastTickInputs = false;
	LastHoveredSlottedIndex = INDEX_NONE;

	if (IsControllerSelectedIndexValid())
	{
		PositionHoverItemAtGridIndex(ControllerSelectedIndex);
	}

	if (const AINV_PlayerController* INVPC = Cast<AINV_PlayerController>(GetOwningPlayer()))
	{
		if (INVPC->WasLastInputGamepad())
		{
			RefreshControllerHoverPlacementVisual();
		}
	}
}

TOptional<FVector2D> UINV_InventoryGrid::ComputeSourceViewportCenterForGridIndex(const int32 GridIndex) const
{
	if (!GridSlots.IsValidIndex(GridIndex) || !IsValid(CanvasPanel))
	{
		return TOptional<FVector2D>();
	}

	const UCanvasPanelSlot* GridSlotCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlots[GridIndex]);
	if (!IsValid(GridSlotCanvasSlot))
	{
		return TOptional<FVector2D>();
	}

	const FVector2D CanvasViewportPosition = UINV_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D SlotCenterLocal = GridSlotCanvasSlot->GetPosition() + (GridSlotCanvasSlot->GetSize() * 0.5f);
	return CanvasViewportPosition + SlotCenterLocal;
}

int32 UINV_InventoryGrid::ResolveControllerActionIndex(const int32 GridIndex) const
{
	if (!GridSlots.IsValidIndex(GridIndex) || !IsValid(GridSlots[GridIndex]))
	{
		return GridIndex;
	}

	const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex();
	return GridSlots.IsValidIndex(UpperLeftIndex) ? UpperLeftIndex : GridIndex;
}

void UINV_InventoryGrid::ApplyControllerVisualForSelectedIndex(const bool bSelectedVisual)
{
	ApplyItemFootprintVisualAtIndex(ControllerSelectedIndex, bSelectedVisual);
}

void UINV_InventoryGrid::ApplyItemFootprintVisualAtIndex(const int32 GridIndex, const bool bSelectedVisual)
{
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UINV_GridSlot* SelectedGridSlot = GridSlots[GridIndex];
	if (!IsValid(SelectedGridSlot)) return;

	if (SelectedGridSlot->GetInventoryItem().IsValid())
	{
		const int32 UpperLeftIndex = ResolveControllerActionIndex(GridIndex);
		const UINV_InventoryItem* SelectedInventoryItem = GridSlots[UpperLeftIndex]->GetInventoryItem().Get();
		const FIntPoint Dimensions = GetItemDimensionsOrDefault(SelectedInventoryItem);
		FINV_GridIteration::ForEach2D(GridSlots, UpperLeftIndex, Dimensions, GridSize.X,
			[bSelectedVisual](UINV_GridSlot* GridSlot)
		{
			if (bSelectedVisual)
			{
				GridSlot->SetSelectedTexture();
			}
			else
			{
				GridSlot->SetOccupiedTexture();
			}
		});
		return;
	}

	if (bSelectedVisual)
	{
		SelectedGridSlot->SetSelectedTexture();
	}
	else
	{
		SelectedGridSlot->SetUnoccupiedTexture();
	}
}

void UINV_InventoryGrid::PositionHoverItemAtGridIndex(int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!IsValid(CanvasPanel)) return;

	const UCanvasPanelSlot* GridSlotCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlots[GridIndex]);
	if (!IsValid(GridSlotCanvasSlot)) return;

	const FVector2D CanvasViewportPosition = UINV_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D SlotCenterLocal = GridSlotCanvasSlot->GetPosition() + (GridSlotCanvasSlot->GetSize() * 0.5f);
	const FVector2D SlotCenterViewport = CanvasViewportPosition + SlotCenterLocal;
	const FVector2D HoverSize = HoverItem->GetCachedSize();
	const FVector2D GridBoundsViewportPos = CanvasViewportPosition;
	const FVector2D GridBoundsSize = FVector2D(GridSize.X * TileSize, GridSize.Y * TileSize);
	const FVector2D DesiredPos = SlotCenterViewport - (HoverSize / 2.0f);
	const FVector2D ClampedRelativePos = UINV_WidgetUtils::GetClampedWidgetPosition(
		GridBoundsSize,
		HoverSize,
		DesiredPos - GridBoundsViewportPos);
	const FVector2D InitialPosition = GridBoundsViewportPos + ClampedRelativePos;
	HoverItem->SetPositionInViewport(InitialPosition, false);
}

void UINV_InventoryGrid::RefreshControllerHoverPlacementVisual()
{
	if (!IsValid(HoverItem)) return;
	if (!IsControllerSelectedIndexValid()) return;

	const int32 ClampedAnchorIndex = ClampControllerAnchorIndexForHover(ControllerSelectedIndex);
	if (ClampedAnchorIndex != ControllerSelectedIndex)
	{
		ControllerSelectedIndex = ClampedAnchorIndex;
	}

	const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
	const FIntPoint AnchorCoordinates(
		ControllerSelectedIndex % GridSize.X,
		ControllerSelectedIndex / GridSize.X);
	ItemDropIndex = ControllerSelectedIndex;
	CurrentQueryResult = FINV_GridPlacementEngine::CheckHoverPosition(
		GridSlots,
		GridSize,
		AnchorCoordinates,
		HoverDimensions);
	ApplyHoverPlacementVisuals(HoverDimensions);

	PositionHoverItemAtGridIndex(ControllerSelectedIndex);
}

int32 UINV_InventoryGrid::ClampControllerAnchorIndexForHover(int32 CandidateIndex) const
{
	if (!GridSlots.IsValidIndex(CandidateIndex)) return CandidateIndex;
	if (!IsValid(HoverItem)) return CandidateIndex;

	const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
	const int32 MaxX = FMath::Max(0, GridSize.X - HoverDimensions.X);
	const int32 MaxY = FMath::Max(0, GridSize.Y - HoverDimensions.Y);
	const int32 CandidateX = CandidateIndex % GridSize.X;
	const int32 CandidateY = CandidateIndex / GridSize.X;
	const int32 ClampedX = FMath::Clamp(CandidateX, 0, MaxX);
	const int32 ClampedY = FMath::Clamp(CandidateY, 0, MaxY);
	return UINV_WidgetUtils::GetIndexFromPosition(FIntPoint(ClampedX, ClampedY), GridSize.X);
}

bool UINV_InventoryGrid::HasOpenItemPopup() const
{
	return IsValid(ItemPopUp) && ItemPopUp->GetVisibility() == ESlateVisibility::Visible;
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
	LastPopupOpenedRealtimeSeconds = FPlatformTime::Seconds();

	// Bind popup callbacks once per popup widget instance.
	BindItemPopupCallbacksIfNeeded();

	ItemPopUp->FocusDefaultAction();
}

void UINV_InventoryGrid::BindItemPopupCallbacksIfNeeded()
{
	if (!IsValid(ItemPopUp)) return;
	if (BoundItemPopUpForCallbacks.Get() == ItemPopUp) return;

	ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);
	ItemPopUp->OnInspect.BindDynamic(this, &ThisClass::OnPopUpMenuInspect);
	ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpMenuConsume);
	BoundItemPopUpForCallbacks = ItemPopUp;
}

bool UINV_InventoryGrid::MatchesCategory(const UINV_InventoryItem* Item) const
{
	if (!IsValid(Item)) return false;
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}

bool UINV_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize,
	const FVector2D& Loc)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = !FINV_GridCoordinateCalculator::IsOutsideBounds(BoundaryPos, BoundarySize, Loc);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
		UnHighlightBlockingItems();
		bHasLastTickInputs = false;
		return true;
	}
	return false;
}
