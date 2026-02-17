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
	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePos.X - CanvasPos.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePos.Y - CanvasPos.Y) / TileSize))
	};
}

EINV_TileQuadrant UINV_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPos, const FVector2D& MousePos) const
{
	// Calculate relative pos within current tile
	const float TileLocalX = FMath::Fmod(MousePos.X - CanvasPos.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePos.Y - CanvasPos.Y, TileSize);
	
	// Determine quadrant mouse is in
	const bool bIsTop = TileLocalY < TileSize / 2.f; // Top if Y is in the upper half
	const bool bIsLeft = TileLocalX < TileSize / 2.f; // Left if X is in the left half
	
	EINV_TileQuadrant HoveredTileQuadrant { EINV_TileQuadrant::None };
	if (bIsTop && bIsLeft) HoveredTileQuadrant = EINV_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft) HoveredTileQuadrant = EINV_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft) HoveredTileQuadrant = EINV_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft) HoveredTileQuadrant = EINV_TileQuadrant::BottomRight;
	
	return HoveredTileQuadrant;
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
	if (!IsValid(ItemPopUp)) return;

	const APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController)) return;

	const bool bClickedThisFrame =
		OwningPlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton) ||
		OwningPlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton) ||
		OwningPlayerController->WasInputKeyJustPressed(EKeys::MiddleMouseButton);

	if (!bClickedThisFrame) return;

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	if (ItemPopUp->GetCachedGeometry().IsUnderLocation(CursorPosition)) return;

	ItemPopUp->RemoveFromParent();
	ItemPopUp = nullptr;
}

void UINV_InventoryGrid::CloseActiveItemPopup()
{
	if (!IsValid(ItemPopUp)) return;
	ItemPopUp->RemoveFromParent();
	ItemPopUp = nullptr;
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
	
	if (bStackableItem)
	{
		// Store stack count on the upper-left slot.
		GridSlots[Index]->SetStackCount(StackAmount);
	}
	
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(NewItem, FragmentTags::GridFragment) };
	if (!GridFragment) return;
	
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	
	UINV_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, GridSize.X, 
		[&](UINV_GridSlot* GridSlot)
	{
		// Mark all slots as occupied by this item.
		GridSlot->SetInventoryItem(NewItem);	
		GridSlot->SetUpperLeftIndex(Index);	
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailability(false);	
	});
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
	
	// If we have a hover item, try to place it.
	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;
	
	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		OnSlottedItemClicked(CurrentQueryResult.UpperLeftIndex, MouseEvent);
		return;
	}

	// If the hovered footprint blocks exactly one item, allow swap even when the clicked tile is empty.
	if (CurrentQueryResult.BlockingUpperLeftIndices.Num() == 1)
	{
		const int32 BlockingUpperLeftIndex = CurrentQueryResult.BlockingUpperLeftIndices[0];
		if (GridSlots.IsValidIndex(BlockingUpperLeftIndex) &&
			GridSlots[BlockingUpperLeftIndex]->GetInventoryItem().IsValid())
		{
			OnSlottedItemClicked(BlockingUpperLeftIndex, MouseEvent);
			return;
		}
	}

	// Multi-blocker case: if clicked tile is empty, choose a best-fit anchor from overlapped blockers.
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid() && CurrentQueryResult.BlockingUpperLeftIndices.Num() > 1)
	{
		int32 BestAnchorIndex = INDEX_NONE;
		int32 BestScore = TNumericLimits<int32>::Lowest();
		
		const UINV_InventoryItem* HoverInventoryItem = HoverItem->GetInventoryItem();
		const FGameplayTag HoverItemType = IsValid(HoverInventoryItem)
			? HoverInventoryItem->GetItemManifest().GetItemType()
			: FGameplayTag();
		
		for (const int32 BlockingUpperLeftIndex : CurrentQueryResult.BlockingUpperLeftIndices)
		{
			if (!GridSlots.IsValidIndex(BlockingUpperLeftIndex)) continue;
			
			const UINV_InventoryItem* BlockingItem = GridSlots[BlockingUpperLeftIndex]->GetInventoryItem().Get();
			if (!IsValid(BlockingItem)) continue;
			
			const FINV_GridFragment* BlockingGridFragment { GetFragment<FINV_GridFragment>(BlockingItem, FragmentTags::GridFragment) };
			const FIntPoint BlockingDimensions = BlockingGridFragment ? BlockingGridFragment->GetGridSize() : FIntPoint(1, 1);
			const int32 BlockingArea = BlockingDimensions.X * BlockingDimensions.Y;
			
			// Prefer larger anchors; bias toward same-type so "same item" swaps behave intuitively.
			const bool bSameTypeAsHover = HoverItemType.IsValid() &&
				BlockingItem->GetItemManifest().GetItemType().MatchesTagExact(HoverItemType);
			const int32 Score = BlockingArea + (bSameTypeAsHover ? 1000 : 0);
			
			if (Score > BestScore)
			{
				BestScore = Score;
				BestAnchorIndex = BlockingUpperLeftIndex;
			}
		}
		
		if (GridSlots.IsValidIndex(BestAnchorIndex) &&
			GridSlots[BestAnchorIndex]->GetInventoryItem().IsValid())
		{
			OnSlottedItemClicked(BestAnchorIndex, MouseEvent);
			return;
		}
	}

	// Multi-item overlap case: if clicked tile belongs to an item, route through slotted-item swap logic.
	if (GridSlots[GridIndex]->GetInventoryItem().IsValid())
	{
		const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex();
		if (GridSlots.IsValidIndex(UpperLeftIndex))
		{
			OnSlottedItemClicked(UpperLeftIndex, MouseEvent);
		}
		return;
	}

	// Only place directly when the whole hovered footprint is actually free.
	if (CurrentQueryResult.bHasSpace)
	{
		PutDownOnIndex(ItemDropIndex);
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
	if (!IsValid(HoverItem)) return;
	
	// Remove hover widget from the viewport.
	HoverItem->Clear();
	HoverItem->RemoveFromParent();
	HoverItem = nullptr;
	
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
	if (!IsValid(GetOwningPlayer())) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, CursorWidget);
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
	UINV_GridSlot* GridSlot { GridSlots[Index] };
	GridSlot->SetStackCount(HoveredStackCount);
	
	UINV_SlottedItem* ClickedSlottedItem { SlottedItems.FindChecked(Index) };
	ClickedSlottedItem->UpdateStackCount(HoveredStackCount);
	
	HoverItem->UpdateStackCount(ClickedStackCount);
}

void UINV_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;
	
	GridSlots[Index]->SetStackCount(NewClickedStackCount);
	SlottedItems.FindChecked(Index)->UpdateStackCount(NewClickedStackCount);
	ClearHoverItem();
	ShowCursor();
	
	const FINV_GridFragment* GridFragment { GridSlots[Index]->GetInventoryItem()->GetItemManifest().GetFragmentOfType<FINV_GridFragment>() };
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	HighlightSlots(Index, Dimensions);
}

void UINV_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UINV_GridSlot* GridSlot { GridSlots[Index] };
	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;
	
	GridSlot->SetStackCount(NewStackCount);
	
	UINV_SlottedItem* ClickedSlottedItem { SlottedItems.FindChecked(Index) };
	ClickedSlottedItem->UpdateStackCount(NewStackCount);
	
	HoverItem->UpdateStackCount(Remainder);
}

UUserWidget* UINV_InventoryGrid::GetCursorWidget(TObjectPtr<UUserWidget>& CachedWidget,
                                                 const TSubclassOf<UUserWidget>& WidgetClass)
{
	if (!IsValid(GetOwningPlayer()) || WidgetClass == nullptr) return nullptr;
	if (!IsValid(CachedWidget))
	{
		CachedWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), WidgetClass);
	}
	return CachedWidget;
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
	// Create and configure the hover widget.
	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UINV_HoverItem>(GetOwningPlayer(), HoverItemClass);
	}
	
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(InventoryItem, FragmentTags::GridFragment) };
	const FINV_ImageFragment* ImageFragment { GetFragment<FINV_ImageFragment>(InventoryItem, FragmentTags::IconFragment) };
	if (!GridFragment || !ImageFragment) return;

	const FVector2D DrawSize { GetDrawSize(GridFragment) };
	
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = DrawSize * UWidgetLayoutLibrary::GetViewportScale(this);
	
	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetGridDimensions(GridFragment->GetGridSize());
	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());
	
	HoverItem->SetDesiredSizeInViewport(IconBrush.ImageSize);
	HoverItem->SetCachedSize(IconBrush.ImageSize);
	HoverItem->AddToViewport();
}

void UINV_InventoryGrid::AssignHoverItem(UINV_InventoryItem* InventoryItem, const int32 GridIndex,
	const int32 PreviousGridIndex)
{
	AssignHoverItem(InventoryItem);
	
	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
}

void UINV_InventoryGrid::RemoveItemFromGrid(const UINV_InventoryItem* InventoryItem, const int32 GridIndex)
{
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(InventoryItem, FragmentTags::GridFragment) };
	if (!GridFragment) return;
	
	UINV_InventoryStatics::ForEach2D(GridSlots, GridIndex, GridFragment->GetGridSize(), GridSize.X, [&](UINV_GridSlot* GridSlot)
	{
		// Clear slot state.
		GridSlot->SetInventoryItem(nullptr);	
		GridSlot->SetUpperLeftIndex(INDEX_NONE);	
		GridSlot->SetUnoccupiedTexture();	
		GridSlot->SetAvailability(true);
		GridSlot->SetStackCount(0);	
	});
	
	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UINV_SlottedItem> FoundSlottedItem;
		SlottedItems.RemoveAndCopyValue(GridIndex, FoundSlottedItem);
		FoundSlottedItem->RemoveFromParent();
	}
}

void UINV_InventoryGrid::AddStacks(const FINV_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;
	
	// Apply stack changes to existing slots.
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (Availability.bItemAtIndex)
		{
			if (!GridSlots.IsValidIndex(Availability.Index)) continue;
			TObjectPtr<UINV_SlottedItem>* SlottedItemPtr = SlottedItems.Find(Availability.Index);
			if (!SlottedItemPtr) continue;
			UINV_SlottedItem* SlottedItem = SlottedItemPtr->Get();
			if (!IsValid(SlottedItem)) continue;
			
			const auto& GridSlot { GridSlots[Availability.Index] };
			SlottedItem->UpdateStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
		}
		else
		{
			AddItemAtIndex(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
			UpdateGridSlots(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

FINV_StackDetails UINV_InventoryGrid::CalculateStackDetails(int32 GridIndex, UINV_InventoryItem* ClickedInventoryItem)
{
	const int32 ClickedStackCount { GridSlots[GridIndex]->GetStackCount() };
	const FINV_StackableFragment* StackableFragment { ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FINV_StackableFragment>() };
	const int32 MaxStackSize { StackableFragment->GetMaxStackSize() };
	const int32 RoomInClickedSlot { MaxStackSize - ClickedStackCount };
	const int32 HoveredStackCount { HoverItem->GetStackCount() };

	return FINV_StackDetails { ClickedStackCount, RoomInClickedSlot, HoveredStackCount, MaxStackSize };
}

void UINV_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	
	CloseActiveItemPopup();
	
	checkf(GridSlots.IsValidIndex(GridIndex), TEXT("Index out of bounds!"));
	UINV_InventoryItem* ClickedInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };

	if (!IsValid(ClickedInventoryItem))
	{
		return;
	}
	
	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		Pickup(ClickedInventoryItem, GridIndex);
		return;
	}
	
	if (IsRightClick(MouseEvent))
	{
		if (IsValid(HoverItem)) return;
		CreateItemPopup(GridIndex);
		return;
	}

	// Do the hovered and clicked item share type, are they stackable?
	if (IsSameStackable(ClickedInventoryItem))
	{
		const FINV_StackDetails StackDetails = CalculateStackDetails(GridIndex, ClickedInventoryItem);

		// Should we swap stack counts? (Room in clicked slot == 0 && HoveredStackCount < MaxStackSize)
		if (ShouldSwapStackCounts(StackDetails.RoomInClickedSlot, StackDetails.HoveredStackCount, StackDetails.MaxStackSize))
		{
			SwapStackCounts(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
			return;
		}
		
		// Should we consume hover item's stacks? (Room in clicked slot >= HoveredStackCount)
		if (ShouldConsumeHoverItemStacks(StackDetails.HoveredStackCount, StackDetails.RoomInClickedSlot))
		{
			ConsumeHoverItemStacks(StackDetails.ClickedStackCount, StackDetails.HoveredStackCount, GridIndex);
			return;
		}
		
		// Should we fill in the stacks of the clicked item? (and not consume hover item)
		if (ShouldFillInStack(StackDetails.RoomInClickedSlot, StackDetails.HoveredStackCount))
		{
			FillInStack(StackDetails.RoomInClickedSlot, StackDetails.HoveredStackCount - StackDetails.RoomInClickedSlot, GridIndex);
			return;
		}
		
		// Clicked slot already full, do nothing (maybe play sound?)
		if (StackDetails.RoomInClickedSlot == 0)
		{
			// If this is the exact same slot the hover came from, keep existing no-op behavior.
			if (HoverItem->GetPreviousGridIndex() == GridIndex)
			{
				return;
			}
			
			// No stack operation possible; fall back to regular swap/displacement behavior.
			SwapWithHoverItem(ClickedInventoryItem, GridIndex);
			return;
		}
		
		return;
	}

	// For spatial placement (especially partial-overlap cases), prefer swap/displacement over stack logic.
	if (IsValid(HoverItem))
	{
		const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
		const FINV_GridFragment* ClickedGridFragment { GetFragment<FINV_GridFragment>(ClickedInventoryItem, FragmentTags::GridFragment) };
		const FIntPoint ClickedDimensions = ClickedGridFragment ? ClickedGridFragment->GetGridSize() : FIntPoint(1, 1);
		const bool bDirectSameSlotDrop = GridSlots.IsValidIndex(ItemDropIndex) && ItemDropIndex == GridIndex;
		const bool bIsSingleTileInteraction = HoverDimensions == FIntPoint(1, 1) && ClickedDimensions == FIntPoint(1, 1);
		
		if (!bDirectSameSlotDrop || !bIsSingleTileInteraction)
		{
			SwapWithHoverItem(ClickedInventoryItem, GridIndex);
			return;
		}
	}

	// Swap with hover item
	SwapWithHoverItem(ClickedInventoryItem, GridIndex);
}

void UINV_InventoryGrid::CreateItemPopup(const int32 GridIndex)
{
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UINV_InventoryItem* RightClickedItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;
	if (!ItemPopUpClass) return;
	if (!OwningCanvasPanel.IsValid()) return;
	
	ItemPopUp = CreateWidget<UINV_ItemPopUp>(this, ItemPopUpClass);
	GridSlots[GridIndex]->SetItemPopUp(ItemPopUp);
	if (!IsValid(ItemPopUp)) return;
	
	OwningCanvasPanel->AddChild(ItemPopUp);
	UCanvasPanelSlot* CanvasSlot { UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp) };
	if (!IsValid(CanvasSlot)) return;

	const FVector2D PopUpSize = ItemPopUp->GetBoxSize();
	CanvasSlot->SetSize(PopUpSize);

	const FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	const FVector2D ClampedPosition = UINV_WidgetUtils::GetCenteredClampedWidgetPosition(
		UINV_WidgetUtils::GetWidgetSize(OwningCanvasPanel.Get()),
		PopUpSize,
		MousePos);
	CanvasSlot->SetPosition(ClampedPosition);
	
	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}
	
	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);
	ItemPopUp->OnInspect.BindDynamic(this, &ThisClass::OnPopUpMenuInspect);
	
	if (RightClickedItem->IsConsumable())
	{
		ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpMenuConsume);
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
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
	bMouseWithinCanvas = UINV_WidgetUtils::IsWithinBounds(BoundaryPos, BoundarySize, Loc);
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
	if (!IsValid(HoverItem) || !IsValid(ClickedInventoryItem))
	{
		return false;
	}

	const bool bIsSameItem = ClickedInventoryItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = HoverItem->IsStackable();
	return bIsSameItem && bIsStackable;
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
