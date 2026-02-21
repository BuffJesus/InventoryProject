// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_SpatialInventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/INV_InventoryComponent.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Utils/INV_InventoryStatics.h"
#include "UI/Utils/INV_WidgetUtils.h"
#include "Items/INV_InventoryItem.h"
#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "UI/Description/INV_ItemDescription.h"
#include "UI/Utils/INV_ItemPresentationUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"

void UINV_SpatialInventory::ShowEquippableGrid()
{
	SetActiveGrid(Grid_Equippable, Button_Equippable);
}

void UINV_SpatialInventory::ShowConsumableGrid()
{
	SetActiveGrid(Grid_Consumable, Button_Consumable);
}

void UINV_SpatialInventory::ShowCraftableGrid()
{
	SetActiveGrid(Grid_Craftable, Button_Craftable);
}

void UINV_SpatialInventory::DisableButton(UButton* Button) const
{
	Button_Equippable->SetIsEnabled(true);
	Button_Consumable->SetIsEnabled(true);
	Button_Craftable->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UINV_SpatialInventory::SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button)
{
	if (ActiveGrid.IsValid()) ActiveGrid->HideCursor();
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid()) ActiveGrid->ShowCursor();
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}

void UINV_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::ShowEquippableGrid);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::ShowConsumableGrid);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::ShowCraftableGrid);
	
	Grid_Equippable->SetOwningCanvas(CanvasPanel);
	Grid_Consumable->SetOwningCanvas(CanvasPanel);
	Grid_Craftable->SetOwningCanvas(CanvasPanel);
	
	ShowEquippableGrid();
	
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UINV_EquippedGridSlot* EquippedGridSlot { Cast<UINV_EquippedGridSlot>(Widget) }; IsValid(EquippedGridSlot))
		{
			EquippedGridSlots.Add(EquippedGridSlot);
			EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	});
}

void UINV_SpatialInventory::EquippedGridSlotClicked(UINV_EquippedGridSlot* EquippedGridSlot,
	const FGameplayTag& EquipmentTypeTag)
{
	// Check if the hover item is equippable
	if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag)) return;
	
	UINV_HoverItem& HoverItem { *GetHoverItem() };
	
	// Create the equipped slotted item and add it to the equipped grid slot (call EquippedGridSlot->OnItemEquipped)
	const float TileSize = UINV_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize();
	UINV_EquippedSlottedItem* EquippedSlottedItem { EquippedGridSlot->OnItemEquipped(
		HoverItem.GetInventoryItem(), 
		EquipmentTypeTag, 
		TileSize) };
	
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	
	// Inform the server that we've equipped an item (potentially unequipping an item as well)
	UINV_InventoryComponent* InventoryComponent { UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer()) };
	checkf(InventoryComponent, TEXT("Inventory component is invalid"));
	
	InventoryComponent->Server_EquipSlotClicked(HoverItem.GetInventoryItem(), nullptr);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(HoverItem.GetInventoryItem());
	}
	
	// Clear the hover item
	Grid_Equippable->ClearHoverItem();
}

void UINV_SpatialInventory::EquippedSlottedItemClicked(UINV_EquippedSlottedItem* EquippedSlottedItem, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (!IsValid(EquippedSlottedItem)) return;
		UINV_InventoryItem* RightClickedItem { EquippedSlottedItem->GetInventoryItem() };
		if (!IsValid(RightClickedItem)) return;
		UINV_InventoryStatics::ItemInspected(GetOwningPlayer(), RightClickedItem, UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
		return;
	}

	// Remove the Item Description
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	if (IsValid(GetHoverItem()) && GetHoverItem()->IsStackable()) return;
	
	// Get the item to equip
	UINV_InventoryItem* ItemToEquip { IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr };
	
	// Get the item to unequip
	UINV_InventoryItem* ItemToUnequip { EquippedSlottedItem->GetInventoryItem() };
	
	// Get the Equipped Grid Slot holding this item
	UINV_EquippedGridSlot* EquippedGridSlot { FindSlotWithEquippedItem(ItemToUnequip) };
	
	// Clear the equipped grid slot of this item (set its inventory item to nullptr)
	ClearSlotOfItem(EquippedGridSlot);
	
	// Assign the previously equipped item as the hover item
	Grid_Equippable->AssignHoverItem(ItemToUnequip);
	
	// Removal of the equipped slotted item from the equipped grid slot 
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	
	// Make a new equipped slotted item (for the item we held in HoverItem)
	MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
	
	// Broadcast delegate for OnItemEquipped/OnItemUnequipped (from the IC)
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

FReply UINV_SpatialInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ActiveGrid->DropItem();
	return FReply::Handled();
}

FINV_SlotAvailabilityResult UINV_SpatialInventory::HasRoomForItem(UINV_ItemComponent* ItemComponent) const
{
	switch (UINV_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{
		case EINV_ItemCategory::Equippable: return Grid_Equippable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Consumable: return Grid_Consumable->HasRoomForItem(ItemComponent);
		case EINV_ItemCategory::Craftable: return Grid_Craftable->HasRoomForItem(ItemComponent);
		default:
		UE_LOG(LogTemp, Error, TEXT("Invalid item category for inventory slot availability check"));
		return FINV_SlotAvailabilityResult();
	}
}

void UINV_SpatialInventory::OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	OpenItemDescription(Item, OpenPosition);
}

bool UINV_SpatialInventory::HasHoverItem() const
{
	if (Grid_Equippable->HasHoverItem()) return true;
	if (Grid_Consumable->HasHoverItem()) return true;
	if (Grid_Craftable->HasHoverItem()) return true;
	return false;
}

void UINV_SpatialInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CloseDescriptionIfCursorExited();
}

UINV_HoverItem* UINV_SpatialInventory::GetHoverItem() const
{
	if (!ActiveGrid.IsValid()) return nullptr;
	return ActiveGrid->GetHoverItem();
}

float UINV_SpatialInventory::GetTileSize() const
{
	return Grid_Equippable->GetTileSize();
}

void UINV_SpatialInventory::SetItemDescriptionSizeAndPosition(UINV_ItemDescription* Description,
                                                              UCanvasPanel* Canvas, const FVector2D& OpenPosition) const
{
	UCanvasPanelSlot* ItemDescriptionCPS { UWidgetLayoutLibrary::SlotAsCanvasSlot(Description) };
	if (!IsValid(ItemDescriptionCPS)) return;

	// Ensure the first-open layout is valid before querying the desired size.
	Description->ForceLayoutPrepass();
	Canvas->ForceLayoutPrepass();

	FVector2D ItemDescriptionSize = Description->GetBoxSize();
	if (ItemDescriptionSize.IsNearlyZero())
	{
		ItemDescriptionSize = Description->GetDesiredSize();
		if (ItemDescriptionSize.IsNearlyZero())
		{
			// Last resort to keep first inspect visible if desired size is not ready yet.
			ItemDescriptionSize = FVector2D(200.f, 100.f);
		}
	}
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);

	// Canvas slot positions are canvas-local, so convert the mouse viewport position to canvas-local space first.
	const FVector2D CanvasViewportPos = UINV_WidgetUtils::GetWidgetPosition(Canvas);
	const FVector2D LocalOpenPosition = OpenPosition - CanvasViewportPos;
	FVector2D ClampedPos { UINV_WidgetUtils::GetCenteredClampedWidgetPosition(
		UINV_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		LocalOpenPosition) };
	
	ItemDescriptionCPS->SetPosition(ClampedPos);
}

void UINV_SpatialInventory::ClearSlotOfItem(UINV_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
	}
}

void UINV_SpatialInventory::RemoveEquippedSlottedItem(UINV_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	
	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UINV_SpatialInventory::MakeEquippedSlottedItem(UINV_EquippedSlottedItem* EquippedSlottedItem,
	UINV_EquippedGridSlot* EquippedGridSlot, UINV_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot)) return;
	
	UINV_EquippedSlottedItem* SlottedItem { EquippedGridSlot->OnItemEquipped(
		ItemToEquip, 
		EquippedSlottedItem->GetEquipmentTypeTag(), 
		UINV_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize()) };
	
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	
	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UINV_SpatialInventory::BroadcastSlotClickedDelegates(UINV_InventoryItem* ItemToEquip,
	UINV_InventoryItem* ItemToUnequip)
{
	UINV_InventoryComponent* InventoryComponent { UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer()) };
	checkf(InventoryComponent, TEXT("Inventory component is invalid"));
	InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(ItemToEquip);
		InventoryComponent->OnItemUnequipped.Broadcast(ItemToUnequip);
	}
}

UINV_EquippedGridSlot* UINV_SpatialInventory::FindSlotWithEquippedItem(UINV_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot { EquippedGridSlots.FindByPredicate([EquippedItem](const TObjectPtr<UINV_EquippedGridSlot>& GridSlotPtr)
	{
		return IsValid(GridSlotPtr) && GridSlotPtr->GetInventoryItem() == EquippedItem;
	}) };
	
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

bool UINV_SpatialInventory::CanEquipHoverItem(UINV_EquippedGridSlot* EquippedGridSlot,
                                              const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;
	
	UINV_HoverItem* HoverItem { GetHoverItem() };
	if (!IsValid(HoverItem)) return false;
	
	UINV_InventoryItem* HeldItem { HoverItem->GetInventoryItem() };
	if (!IsValid(HeldItem)) return false;
	
	return HasHoverItem() && IsValid(HeldItem) && 
		!HoverItem->IsStackable() && 
			HeldItem->GetItemManifest().GetItemCategory() == EINV_ItemCategory::Equippable &&
			HeldItem->GetItemManifest().GetItemType().MatchesTag(EquipmentTypeTag);
}

UINV_ItemDescription* UINV_SpatialInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UINV_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
		if (UCanvasPanelSlot* DescriptionSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemDescription))
		{
			DescriptionSlot->SetZOrder(1000);
		}
	}
	return ItemDescription;
}

void UINV_SpatialInventory::OpenItemDescription(UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	if (!IsValid(Item) || !IsValid(CanvasPanel.Get())) return;
	const auto& Manifest { Item->GetItemManifest() };

	UINV_ItemDescription* DescriptionWidget = GetItemDescription();
	if (!IsValid(DescriptionWidget)) return;

	DescriptionWidget->Collapse();
	const FGameplayTag ItemRarityTag = Item->IsItemRarityEnabled() ? Item->GetItemRarityTag() : FGameplayTag::EmptyTag;
	DescriptionWidget->ApplyFunction([ItemRarityTag](auto* Widget)
	{
		Widget->SetItemRarityTag(ItemRarityTag);
	});
	FINV_ItemPresentationUtils::AssimilateInventoryFragments(Manifest, DescriptionWidget);
	
	SetItemDescriptionSizeAndPosition(DescriptionWidget, CanvasPanel, OpenPosition);
	DescriptionWidget->SetVisibility(ESlateVisibility::Visible);
	bSkipDescriptionCloseThisTick = true;
}

void UINV_SpatialInventory::CloseDescriptionIfCursorExited()
{
	if (!IsValid(ItemDescription)) return;
	if (ItemDescription->GetVisibility() == ESlateVisibility::Collapsed) return;
	if (bSkipDescriptionCloseThisTick)
	{
		bSkipDescriptionCloseThisTick = false;
		return;
	}

	// Avoid evaluating exit logic before Slate has a valid layout for this widget.
	if (ItemDescription->GetCachedGeometry().GetLocalSize().IsNearlyZero())
	{
		return;
	}

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	if (ItemDescription->GetCachedGeometry().IsUnderLocation(CursorPosition)) return;
	ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
}
