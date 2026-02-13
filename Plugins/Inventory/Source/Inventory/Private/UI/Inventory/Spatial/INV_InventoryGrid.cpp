// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_FragmentTags.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UI/INV_WidgetUtils.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const UINV_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const FINV_ItemManifest& Manifest)
{
	// Walk the grid and compute how much space we can fill.
	FINV_SlotAvailabilityResult Result;
	
	// Determine if item is stackable
	const FINV_StackableFragment* StackableFragment { Manifest.GetFragmentOfType<FINV_StackableFragment>() };
	Result.bStackable = StackableFragment != nullptr;
	
	// Determine how many stacks to add
	const int32 MaxStackSize { StackableFragment ? StackableFragment->GetMaxStackSize() : 1 };
	int32 AmountToFill { StackableFragment ? StackableFragment->GetStackCount() : 1 };
	
	TSet<int32> CheckedIndices;
	
	// For stackable items, top off existing matching stacks before searching for empty slots.
	if (Result.bStackable)
	{
		for (const auto& SlottedItemPair : SlottedItems)
		{
			const TObjectPtr<UINV_SlottedItem> SlottedItem = SlottedItemPair.Value;
			if (!IsValid(SlottedItem)) continue;
			
			const UINV_InventoryItem* ExistingItem = SlottedItem->GetInventoryItem();
			if (!IsValid(ExistingItem)) continue;
			if (!ExistingItem->GetItemManifest().GetItemType().MatchesTagExact(Manifest.GetItemType())) continue;
			
			const int32 GridIndex = SlottedItemPair.Key;
			if (!GridSlots.IsValidIndex(GridIndex)) continue;
			const UINV_GridSlot* GridSlot = GridSlots[GridIndex];
			if (!IsValid(GridSlot)) continue;
			
			const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
			if (AmountToFillInSlot <= 0) continue;
			
			CheckedIndices.Add(GridIndex);
			
			Result.TotalRoomToFill += AmountToFillInSlot;
			Result.SlotAvailabilities.Emplace(
				FINV_SlotAvailability{
					HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetTileIndex(),
					Result.bStackable ? AmountToFillInSlot : 0,
					HasValidItem(GridSlot)
				}
			);
			
			AmountToFill -= AmountToFillInSlot;
			Result.Remainder = AmountToFill;
			
			if (AmountToFill == 0) return Result;
		}
	}
	
	// For each GridSlot:
	for (const auto& GridSlot : GridSlots)
	{
	    // if no more to fill, break from loop
	    if (AmountToFill == 0) break;
		
	    // is index claimed?
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetTileIndex())) continue;
		
		// is the item inside grid bounds?
		if (!IsInGridBounds(GridSlot->GetTileIndex(), GetItemDimensions(Manifest))) continue;
		
	    // can item fit (i.e., out of bounds)?
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot, GetItemDimensions(Manifest), CheckedIndices, TentativelyClaimed, Manifest.GetItemType(), MaxStackSize))
		{
			continue;
		}
		
	    // how much to fill?
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		if (AmountToFillInSlot == 0) continue;
		
		CheckedIndices.Append(TentativelyClaimed);
		
	    // update amount left to fill
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FINV_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetTileIndex(), 
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}	
		);
		
		AmountToFill -= AmountToFillInSlot;
		
		// How much remaining?
		Result.Remainder = AmountToFill;
		
		if (AmountToFill == 0) return Result;
	}
	return Result;
}

bool UINV_InventoryGrid::HasRoomAtIndex(const UINV_GridSlot* GridSlot, 
	const FIntPoint& Dimensions, 
	const TSet<int32>& CheckedIndices, 
	TSet<int32>& OutTentativelyClaimed,
	const FGameplayTag& ItemType,
	const int32 MaxStackSize)
{
	// is there room at index (i.e., other items in the way)?
	bool bHasRoomAtIndex = true;
	UINV_InventoryStatics::ForEach2D(GridSlots, GridSlot->GetTileIndex(), Dimensions, GridSize.X, 
		[&](const UINV_GridSlot* SubGridSlot)
		{
			if (CheckSlotConstraints(GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
			{
				OutTentativelyClaimed.Add(SubGridSlot->GetTileIndex());
			}
			else
			{
				bHasRoomAtIndex = false;
			}
		});
	
	return bHasRoomAtIndex;
}

FIntPoint UINV_InventoryGrid::GetItemDimensions(const FINV_ItemManifest& Manifest) const
{
	const FINV_GridFragment* GridFragment { Manifest.GetFragmentOfType<FINV_GridFragment>() };
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
}

bool UINV_InventoryGrid::CheckSlotConstraints(const UINV_GridSlot* GridSlot, 
	const UINV_GridSlot* SubGridSlot, 
	const TSet<int32>& CheckedIndices, 
	TSet<int32> OutTentativelyClaimed, 
	const FGameplayTag& ItemType, 
	const int32 MaxStackSize) const
{
	// index claimed?
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetTileIndex())) return false;
	
	// has valid item?
	if (!HasValidItem(SubGridSlot)) return true;
	
	// is this grid slot upper left slot?
	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;
	
	// if yes, is stackable?
	const UINV_InventoryItem* SubItem { SubGridSlot->GetInventoryItem().Get() };
	if (!SubItem->IsStackable()) return false;
	
	// is item same type as the item trying to add?
	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
	
	// if yes, is slot a max stack size already?
	if (GridSlot->GetStackCount() >= MaxStackSize) return false;
	
	return true;
}

bool UINV_InventoryGrid::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const
{
	return CheckedIndices.Contains(Index);
}

bool UINV_InventoryGrid::HasValidItem(const UINV_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UINV_InventoryGrid::IsUpperLeftSlot(const UINV_GridSlot* GridSlot, const UINV_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetTileIndex();
}

bool UINV_InventoryGrid::DoesItemTypeMatch(const UINV_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UINV_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex < 0 || StartIndex >= GridSlots.Num()) return false;
	const int32 EndColumn = (StartIndex % GridSize.X) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / GridSize.X) + ItemDimensions.Y;
	return EndColumn <= GridSize.X && EndRow <= GridSize.Y;
}

int32 UINV_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize,
	const int32 AmountToFill, const UINV_GridSlot* GridSlot) const
{
	// calculate room in the slot
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	
	// if stackable, need min between amount to fill and room in slot
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UINV_InventoryGrid::GetStackAmount(const UINV_GridSlot* GridSlot) const
{
	int32 CurrentSlotStackCount { GridSlot->GetStackCount() };
	// if we are at a slot that doesn't hold stack count, must get actual stack count
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		UINV_GridSlot* UpperLeftGridSlot { GridSlots[UpperLeftIndex] };
		CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
	}
	return CurrentSlotStackCount;
}

FINV_SlotAvailabilityResult UINV_InventoryGrid::HasRoomForItem(const UINV_InventoryItem* Item)
{
	return HasRoomForItem(Item->GetItemManifest());
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

void UINV_InventoryGrid::OnTileParamsUpdated(const FINV_TileParams& Params)
{
	if (!IsValid(HoverItem)) return;
	
	// Get hover item dimensions
	const FIntPoint Dimensions { HoverItem->GetGridDimensions() };
	
	// Calculate starting coord for highlighting
	const FIntPoint StartingCoord { CalculateStartingCoordinate(Params.TileCoordinates, Dimensions, Params.TileQuadrant) };
	ItemDropIndex = UINV_WidgetUtils::GetIndexFromPosition(StartingCoord, GridSize.X);
	
	CurrentQueryResult = CheckHoverPosition(StartingCoord, Dimensions);
	
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
	if (IsInGridBounds(ItemDropIndex, Dimensions))
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
	if (!bMouseWithinCanvas) return;
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UINV_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, GridSize.X, 
		[&](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();		
	});
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void UINV_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UINV_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, GridSize.X, 
		[&](UINV_GridSlot* GridSlot)
	{
		if (GridSlot->GetAvailability())
		{
			GridSlot->SetUnoccupiedTexture();
		}
		else
		{
			GridSlot->SetOccupiedTexture();
		}	
	});
}

void UINV_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions, EINV_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
	UINV_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, GridSize.X, [State = GridSlotState](UINV_GridSlot* GridSlot)
	{
		switch (State)
		{
			case EINV_GridSlotState::Occupied: GridSlot->SetOccupiedTexture(); break;
			case EINV_GridSlotState::Unoccupied: GridSlot->SetUnoccupiedTexture(); break;
			case EINV_GridSlotState::GrayedOut: GridSlot->SetGrayedOutTexture(); break;
			case EINV_GridSlotState::Selected: GridSlot->SetSelectedTexture(); break;
		}
	});
	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

FINV_SpaceQueryResult UINV_InventoryGrid::CheckHoverPosition(const FIntPoint& Pos, const FIntPoint& Dimensions) 
{
	FINV_SpaceQueryResult Result;
	
	// in grid bounds?
	if (!IsInGridBounds(UINV_WidgetUtils::GetIndexFromPosition(Pos, GridSize.X), Dimensions)) return Result;
	
	Result.bHasSpace = true;
	
	// if more than one of the indices is occupied with the same item, need to check if all have same upper left index
	TSet<int32> OccupiedUpperLeftIndices;
	UINV_InventoryStatics::ForEach2D(GridSlots, UINV_WidgetUtils::GetIndexFromPosition(Pos, GridSize.X), 
		Dimensions, GridSize.X, [&](const UINV_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});
	
	// if yes, only one item in the way? (can we swap?)
	if (OccupiedUpperLeftIndices.Num() == 1) // single item at position, valid for swapping/combining
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}

	Result.BlockingUpperLeftIndices = OccupiedUpperLeftIndices.Array();
	
	return Result;
}

void UINV_InventoryGrid::HighlightBlockingItems(const TArray<int32>& BlockingUpperLeftIndices)
{
	UnHighlightBlockingItems();
	LastHighlightedIndex = INDEX_NONE;
	LastHighlightedDimensions = FIntPoint::ZeroValue;
	
	for (const int32 UpperLeftIndex : BlockingUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(UpperLeftIndex)) continue;
		const UINV_InventoryItem* BlockingItem = GridSlots[UpperLeftIndex]->GetInventoryItem().Get();
		if (!IsValid(BlockingItem)) continue;
		
		const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(BlockingItem, FragmentTags::GridFragment) };
		if (!GridFragment) continue;
		
		LastGrayedOutUpperLeftIndices.Add(UpperLeftIndex);
		UINV_InventoryStatics::ForEach2D(GridSlots, UpperLeftIndex, GridFragment->GetGridSize(), GridSize.X,
			[](UINV_GridSlot* GridSlot)
		{
			GridSlot->SetGrayedOutTexture();
		});
	}
}

void UINV_InventoryGrid::UnHighlightBlockingItems()
{
	for (const int32 UpperLeftIndex : LastGrayedOutUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(UpperLeftIndex)) continue;
		const UINV_InventoryItem* BlockingItem = GridSlots[UpperLeftIndex]->GetInventoryItem().Get();
		if (!IsValid(BlockingItem)) continue;
		
		const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(BlockingItem, FragmentTags::GridFragment) };
		if (!GridFragment) continue;
		
		UINV_InventoryStatics::ForEach2D(GridSlots, UpperLeftIndex, GridFragment->GetGridSize(), GridSize.X,
			[](UINV_GridSlot* GridSlot)
		{
			if (GridSlot->GetAvailability())
			{
				GridSlot->SetUnoccupiedTexture();
			}
			else
			{
				GridSlot->SetOccupiedTexture();
			}
		});
	}
	LastGrayedOutUpperLeftIndices.Reset();
}

void UINV_InventoryGrid::RefreshGridSlotVisualsFromAvailability()
{
	for (UINV_GridSlot* GridSlot : GridSlots)
	{
		if (!IsValid(GridSlot)) continue;
		if (GridSlot->GetAvailability())
		{
			GridSlot->SetUnoccupiedTexture();
		}
		else
		{
			GridSlot->SetOccupiedTexture();
		}
	}
}

FIntPoint UINV_InventoryGrid::CalculateStartingCoordinate(const FIntPoint& Coord, const FIntPoint& Dimensions,
	const EINV_TileQuadrant Quadrant) const
{
	const int32 HasEvenWidth = Dimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = Dimensions.Y % 2 == 0 ? 1 : 0;
	
	FIntPoint StartingCoord;
	switch (Quadrant)
	{
	case EINV_TileQuadrant::TopLeft:
			StartingCoord.X = Coord.X - FMath::FloorToInt(Dimensions.X * 0.5f);
			StartingCoord.Y = Coord.Y - FMath::FloorToInt(Dimensions.Y * 0.5f);
		break;
		case EINV_TileQuadrant::TopRight:
			StartingCoord.X = Coord.X - FMath::FloorToInt(Dimensions.X * 0.5f) + HasEvenWidth;
			StartingCoord.Y = Coord.Y - FMath::FloorToInt(Dimensions.Y * 0.5f);
		break;
		case EINV_TileQuadrant::BottomLeft:
			StartingCoord.X = Coord.X - FMath::FloorToInt(Dimensions.X * 0.5f);
			StartingCoord.Y = Coord.Y - FMath::FloorToInt(Dimensions.Y * 0.5f) + HasEvenHeight;
		break;
		case EINV_TileQuadrant::BottomRight:
			StartingCoord.X = Coord.X - FMath::FloorToInt(Dimensions.X * 0.5f) + HasEvenWidth;
			StartingCoord.Y = Coord.Y - FMath::FloorToInt(Dimensions.Y * 0.5f) + HasEvenHeight;
		break;
	default:
		UE_LOG(LogInventory, Error, TEXT("Invalid tile quadrant: %d"), static_cast<int32>(Quadrant));
		return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

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
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	FVector2D IconSize { GridFragment->GetGridSize() * IconTileWidth };
	return IconSize;
}

void UINV_InventoryGrid::SetSlottedItemImageBrush(const FINV_GridFragment* GridFragment, 
	const FINV_ImageFragment* ImageFragment, const UINV_SlottedItem* SlottedItem) const
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(Brush);
}

UINV_SlottedItem* UINV_InventoryGrid::CreateSlottedItem(UINV_InventoryItem* Item, const bool bStackable, const int32 StackAmount, 
	const FINV_GridFragment* GridFragment, const FINV_ImageFragment* ImageFragment, const int32 Index)
{
	UINV_SlottedItem* SlottedItem { CreateWidget<UINV_SlottedItem>(GetOwningPlayer(), SlottedItemClass) };
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImageBrush(GridFragment, ImageFragment, SlottedItem);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	
	return SlottedItem;
}

void UINV_InventoryGrid::AddItemAtIndex(UINV_InventoryItem* Item, const int32 Index, const bool bStackable,
                                        const int32 StackAmount)
{
	// Build the slotted widget and place it on the canvas.
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(Item, FragmentTags::GridFragment) };
	const FINV_ImageFragment* ImageFragment { GetFragment<FINV_ImageFragment>(Item, FragmentTags::IconFragment) };
	if (!GridFragment || !ImageFragment) return;
	
	UINV_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);
	
	SlottedItems.Add(Index, SlottedItem);
}

void UINV_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FINV_GridFragment* GridFragment,
	UINV_SlottedItem* SlottedItem)
{
	CanvasPanel->AddChild(SlottedItem);
	UCanvasPanelSlot* CanvasSlot { UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem) };
	CanvasSlot->SetSize(GetDrawSize(GridFragment));
	const FVector2D DrawPos = UINV_WidgetUtils::GetPositionFromIndex(Index, GridSize.X) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());
	CanvasSlot->SetPosition(DrawPosWithPadding);
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
	// Create grid slots and place them on the canvas.
	GridSlots.Reserve(GridSize.X * GridSize.Y);
	
	for (int32 j = 0; j < GridSize.Y; ++j)
	{
		for (int32 i = 0; i < GridSize.X; ++i)
		{
			UINV_GridSlot* GridSlot { CreateWidget<UINV_GridSlot>(this, SlotClass) };
			CanvasPanel->AddChild(GridSlot);
			
			const FIntPoint TilePosition { i, j };
			GridSlot->SetTileIndex(UINV_WidgetUtils::GetIndexFromPosition(TilePosition, GridSize.X));
			
			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(TilePosition * TileSize);
			
			GridSlots.Add(GridSlot);
			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		}
	}
}

void UINV_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
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

void UINV_InventoryGrid::SetCursorWidget(UUserWidget* CursorWidget)
{
	if (!IsValid(GetOwningPlayer())) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, CursorWidget);
}

void UINV_InventoryGrid::SwapWithHoverItem(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;
	if (!IsValid(ClickedInventoryItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid()) return;

	UINV_InventoryItem* HoverInventoryItem = HoverItem->GetInventoryItem();
	const FINV_GridFragment* HoverGridFragment { GetFragment<FINV_GridFragment>(HoverInventoryItem, FragmentTags::GridFragment) };
	if (!HoverGridFragment) return;

	// Prefer the computed hover drop index, but fall back to clicked index if needed.
	const int32 TargetDropIndex = GridSlots.IsValidIndex(ItemDropIndex) ? ItemDropIndex : GridIndex;
	if (!IsInGridBounds(TargetDropIndex, HoverGridFragment->GetGridSize())) return;

	// Gather all unique overlapped items under the hovered footprint.
	TSet<int32> OverlappedUpperLeftIndices;
	UINV_InventoryStatics::ForEach2D(GridSlots, TargetDropIndex, HoverGridFragment->GetGridSize(), GridSize.X,
		[&](const UINV_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OverlappedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
		}
	});

	if (!OverlappedUpperLeftIndices.Contains(GridIndex))
	{
		// Only allow swap when the clicked item is one of the overlapped blockers.
		return;
	}

	struct FDisplacedItemPlan
	{
		UINV_InventoryItem* Item { nullptr };
		int32 SourceIndex { INDEX_NONE };
		int32 TargetIndex { INDEX_NONE };
		int32 StackCount { 0 };
		bool bStackable { false };
		FIntPoint Dimensions { 1, 1 };
	};

	TArray<FDisplacedItemPlan> DisplacedItems;
	DisplacedItems.Reserve(OverlappedUpperLeftIndices.Num());

	// Build candidate list for all overlapped blockers except the clicked one (which becomes new hover item).
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (OverlappedIndex == GridIndex) continue;
		if (!GridSlots.IsValidIndex(OverlappedIndex)) return;

		UINV_GridSlot* SourceSlot = GridSlots[OverlappedIndex];
		UINV_InventoryItem* SourceItem = SourceSlot->GetInventoryItem().Get();
		if (!IsValid(SourceItem)) return;

		const FINV_GridFragment* SourceGridFragment { GetFragment<FINV_GridFragment>(SourceItem, FragmentTags::GridFragment) };
		if (!SourceGridFragment) return;

		DisplacedItems.Add(FDisplacedItemPlan{
			SourceItem,
			OverlappedIndex,
			INDEX_NONE,
			SourceSlot->GetStackCount(),
			SourceItem->IsStackable(),
			SourceGridFragment->GetGridSize()
		});
	}

	// Build occupancy simulation so we can validate all relocations before mutating UI state.
	TArray<bool> SimulatedOccupied;
	SimulatedOccupied.Init(false, GridSlots.Num());
	for (int32 Index = 0; Index < GridSlots.Num(); ++Index)
	{
		SimulatedOccupied[Index] = GridSlots[Index]->GetInventoryItem().IsValid();
	}

	auto MarkFootprint = [&](const int32 StartIndex, const FIntPoint& Dimensions, const bool bOccupied)
	{
		UINV_InventoryStatics::ForEach2D(GridSlots, StartIndex, Dimensions, GridSize.X,
			[&](const UINV_GridSlot* GridSlotRef)
		{
			SimulatedOccupied[GridSlotRef->GetTileIndex()] = bOccupied;
		});
	};

	// Free all overlapped blockers (clicked + others), then reserve target footprint for hovered item.
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(OverlappedIndex)) return;
		UINV_InventoryItem* SourceItem = GridSlots[OverlappedIndex]->GetInventoryItem().Get();
		if (!IsValid(SourceItem)) return;

		const FINV_GridFragment* SourceGridFragment { GetFragment<FINV_GridFragment>(SourceItem, FragmentTags::GridFragment) };
		if (!SourceGridFragment) return;

		MarkFootprint(OverlappedIndex, SourceGridFragment->GetGridSize(), false);
	}
	MarkFootprint(TargetDropIndex, HoverGridFragment->GetGridSize(), true);

	auto CanFitAt = [&](const int32 StartIndex, const FIntPoint& Dimensions) -> bool
	{
		if (!IsInGridBounds(StartIndex, Dimensions)) return false;

		bool bCanFit = true;
		UINV_InventoryStatics::ForEach2D(GridSlots, StartIndex, Dimensions, GridSize.X,
			[&](const UINV_GridSlot* GridSlotRef)
		{
			if (SimulatedOccupied[GridSlotRef->GetTileIndex()])
			{
				bCanFit = false;
			}
		});
		return bCanFit;
	};

	// Plan destinations for all displaced blockers using first-fit.
	for (FDisplacedItemPlan& Plan : DisplacedItems)
	{
		for (int32 CandidateIndex = 0; CandidateIndex < GridSlots.Num(); ++CandidateIndex)
		{
			if (!CanFitAt(CandidateIndex, Plan.Dimensions)) continue;

			Plan.TargetIndex = CandidateIndex;
			MarkFootprint(CandidateIndex, Plan.Dimensions, true);
			break;
		}

		if (Plan.TargetIndex == INDEX_NONE)
		{
			// Not enough room to relocate all blockers safely.
			return;
		}
	}

	const int32 TempStackCount { HoverItem->GetStackCount() };
	const bool bTempIsStackable { HoverItem->IsStackable() };
	const int32 HoverPreviousIndex = HoverItem->GetPreviousGridIndex();

	// Keep same previous grid index for the newly hovered clicked item.
	AssignHoverItem(ClickedInventoryItem, GridIndex, HoverPreviousIndex);

	// Remove all overlapped blockers now that we know relocation is possible.
	for (const int32 OverlappedIndex : OverlappedUpperLeftIndices)
	{
		if (!GridSlots.IsValidIndex(OverlappedIndex)) continue;
		UINV_InventoryItem* SourceItem = GridSlots[OverlappedIndex]->GetInventoryItem().Get();
		if (!IsValid(SourceItem)) continue;
		RemoveItemFromGrid(SourceItem, OverlappedIndex);
	}

	// Place hovered item at destination.
	AddItemAtIndex(HoverInventoryItem, TargetDropIndex, bTempIsStackable, TempStackCount);
	UpdateGridSlots(HoverInventoryItem, TargetDropIndex, bTempIsStackable, TempStackCount);

	// Re-home displaced blockers.
	for (const FDisplacedItemPlan& Plan : DisplacedItems)
	{
		AddItemAtIndex(Plan.Item, Plan.TargetIndex, Plan.bStackable, Plan.StackCount);
		UpdateGridSlots(Plan.Item, Plan.TargetIndex, Plan.bStackable, Plan.StackCount);
	}

	// Ensure no stale hover-highlight textures remain after relocation.
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

void UINV_InventoryGrid::Pickup(UINV_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	// Convert a slotted item into a hover item.
	AssignHoverItem(ClickedInventoryItem, GridIndex, GridIndex);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
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
	checkf(GridSlots.IsValidIndex(GridIndex), TEXT("Index out of bounds!"));
	UINV_InventoryItem* ClickedInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	
	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		Pickup(ClickedInventoryItem, GridIndex);
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
	// Swap with hover item
	SwapWithHoverItem(ClickedInventoryItem, GridIndex);
}

bool UINV_InventoryGrid::MatchesCategory(const UINV_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}

bool UINV_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UINV_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
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
	const bool bIsSameItem = ClickedInventoryItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = HoverItem->IsStackable();
	return bIsSameItem && bIsStackable;
}

bool UINV_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount,
	const int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

bool UINV_InventoryGrid::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot) const
{
	return RoomInClickedSlot >= HoveredStackCount;
}

bool UINV_InventoryGrid::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}
