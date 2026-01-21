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
	FINV_SlotAvailabilityResult Result;
	
	// Determine if item is stackable
	const FINV_StackableFragment* StackableFragment { Manifest.GetFragmentOfType<FINV_StackableFragment>() };
	Result.bStackable = StackableFragment != nullptr;
	
	// Determine how many stacks to add
	const int32 MaxStackSize { StackableFragment ? StackableFragment->GetMaxStackSize() : 1 };
	int32 AmountToFill { StackableFragment ? StackableFragment->GetStackCount() : 1 };
	
	TSet<int32> CheckedIndices;
	// For each Gridslot:
	for (const auto& GridSlot : GridSlots)
	{
	    // if no more to fill, break from loop
	    if (AmountToFill == 0) break;
		
	    // is index claimed
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetTileIndex())) continue;
		
		// is item inside of grid bounds
		if (!IsInGridBounds(GridSlot->GetTileIndex(), GetItemDimensions(Manifest))) continue;
		
	    // can item fit (ie out of bounds)
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot, GetItemDimensions(Manifest), CheckedIndices, TentativelyClaimed, Manifest.GetItemType(), MaxStackSize))
		{
			continue;
		}
		
	    // how much to fill
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
		
		// How much remaining
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
	// is there room at index (ie other items in way)
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
	// index claimed
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetTileIndex())) return false;
	
	// has valid item
	if (!HasValidItem(SubGridSlot)) return true;
	
	// is this grid slot upper left slot
	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;
	
	// if yes, is stackable
	const UINV_InventoryItem* SubItem { SubGridSlot->GetInventoryItem().Get() };
	if (!SubItem->IsStackable()) return false;
	
	// is item same type as item trying to add
	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
	
	// if yes, is slot a mack stack size already
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
	// calculate room in slot
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	
	// if stackable, need min btwn amounttofill and roominslot
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
	
	ConstructGrid();
	
	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
	InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
}

void UINV_InventoryGrid::AddItem(UINV_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	
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
		GridSlots[Index]->SetStackCount(StackAmount);
	}
	
	const FINV_GridFragment* GridFragment { GetFragment<FINV_GridFragment>(NewItem, FragmentTags::GridFragment) };
	if (!GridFragment) return;
	
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	
	UINV_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, GridSize.X, 
		[&](UINV_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);	
		GridSlot->SetUpperLeftIndex(Index);	
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailability(false);	
	});
}

void UINV_InventoryGrid::ConstructGrid()
{
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
		}
	}
}

void UINV_InventoryGrid::AddStacks(const FINV_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;
	
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (Availability.bItemAtIndex)
		{
			const auto& GridSlot { GridSlots[Availability.Index] };
			const auto& SlottedItem { SlottedItems.FindChecked(Availability.Index) };
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

void UINV_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	checkf(GridSlots.IsValidIndex(GridIndex), TEXT("Index out of bounds!"));
	UINV_InventoryItem* ClickedInventoryItem { GridSlots[GridIndex]->GetInventoryItem().Get() };
	
	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		// TODO: Pickup - Assign hover item, remove slotted item from grid
	}
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
