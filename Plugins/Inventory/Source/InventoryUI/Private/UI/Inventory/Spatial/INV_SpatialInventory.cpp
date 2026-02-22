// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_SpatialInventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/INV_InventoryComponent.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "UI/Utils/INV_InventoryStatics.h"
#include "UI/Utils/INV_WidgetUtils.h"
#include "Items/INV_InventoryItem.h"
#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "UI/Description/INV_ItemDescription.h"
#include "UI/Utils/INV_ItemPresentationUtils.h"
#include "UI/Inventory/Services/INV_EquippedSlotService.h"
#include "UI/Inventory/Services/INV_SpatialCategoryService.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Inventory/GridSlots/INV_EquippedGridSlot.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Inventory/SlottedItems/INV_EquippedSlottedItem.h"
#include "InputCoreTypes.h"

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
	ForEachCategoryButton([](UButton* CategoryButton)
	{
		if (IsValid(CategoryButton))
		{
			CategoryButton->SetIsEnabled(true);
		}
	});

	if (IsValid(Button))
	{
		Button->SetIsEnabled(false);
	}
}

void UINV_SpatialInventory::ForEachInventoryGrid(TFunctionRef<void(UINV_InventoryGrid* Grid)> Visitor) const
{
	Visitor(Grid_Equippable);
	Visitor(Grid_Consumable);
	Visitor(Grid_Craftable);
}

UINV_InventoryGrid* UINV_SpatialInventory::GetGridForCategory(const EINV_ItemCategory Category) const
{
	switch (Category)
	{
	case EINV_ItemCategory::Equippable:
		return Grid_Equippable;
	case EINV_ItemCategory::Consumable:
		return Grid_Consumable;
	case EINV_ItemCategory::Craftable:
		return Grid_Craftable;
	default:
		return nullptr;
	}
}

void UINV_SpatialInventory::ForEachCategoryButton(TFunctionRef<void(UButton* Button)> Visitor) const
{
	Visitor(Button_Equippable);
	Visitor(Button_Consumable);
	Visitor(Button_Craftable);
}

void UINV_SpatialInventory::SwitchCategoryByDirection(int32 Direction)
{
	if (Direction == 0) return;

	TArray<UINV_InventoryGrid*> OrderedGrids;
	TArray<UButton*> OrderedButtons;
	BuildCategoryViews(OrderedGrids, OrderedButtons);
	if (OrderedGrids.Num() != OrderedButtons.Num()) return;

	int32 CurrentIndex = OrderedGrids.Find(ActiveGrid.Get());
	const int32 NewIndex = FINV_SpatialCategoryService::ResolveNextCategoryIndex(
		CurrentIndex,
		Direction,
		OrderedGrids.Num());
	if (NewIndex == INDEX_NONE) return;

	SetActiveGrid(OrderedGrids[NewIndex], OrderedButtons[NewIndex]);
}

void UINV_SpatialInventory::BuildCategoryViews(TArray<UINV_InventoryGrid*>& OutGrids, TArray<UButton*>& OutButtons) const
{
	OutGrids = { Grid_Equippable, Grid_Consumable, Grid_Craftable };
	OutButtons = { Button_Equippable, Button_Consumable, Button_Craftable };
}

void UINV_SpatialInventory::SetActiveGrid(UINV_InventoryGrid* Grid, UButton* Button)
{
	ReturnActiveHoverItemToSource();
	if (ActiveGrid.IsValid()) ActiveGrid->HideCursor();
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid()) ActiveGrid->ShowCursor();
	DisableButton(Button);
	if (IsValid(Switcher) && IsValid(Grid))
	{
		Switcher->SetActiveWidget(Grid);
	}
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->SetKeyboardFocus();
	}
}

bool UINV_SpatialInventory::IsGridNavigationInput(const FKey& PressedKey) const
{
	return FINV_SpatialCategoryService::IsGridNavigationInput(PressedKey);
}

bool UINV_SpatialInventory::TryFocusActiveGridForNavigation(const FKey& PressedKey)
{
	if (!IsGridNavigationInput(PressedKey)) return false;
	if (!ActiveGrid.IsValid()) return false;
	if (ActiveGrid->HasKeyboardFocus()) return false;
	if (ActiveGrid->HasOpenItemPopup()) return false;

	ActiveGrid->SetKeyboardFocus();
	return true;
}

FReply UINV_SpatialInventory::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	const FKey PressedKey = InKeyEvent.GetKey();

	if (TryFocusActiveGridForNavigation(PressedKey))
	{
		return FReply::Handled();
	}

	if (PressedKey == EKeys::Gamepad_LeftShoulder)
	{
		SwitchCategoryByDirection(-1);
		return FReply::Handled();
	}
	if (PressedKey == EKeys::Gamepad_RightShoulder)
	{
		SwitchCategoryByDirection(1);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool UINV_SpatialInventory::TryEquipHoveredItemFromController()
{
	if (ActiveGrid.Get() != Grid_Equippable) return false;
	if (!IsValid(Grid_Equippable) || !Grid_Equippable->HasHoverItem()) return false;

	for (UINV_EquippedGridSlot* EquippedGridSlot : EquippedGridSlots)
	{
		if (!IsValid(EquippedGridSlot)) continue;
		const FGameplayTag EquipmentTypeTag = EquippedGridSlot->GetEquipmentTypeTag();
		if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag)) continue;
		EquippedGridSlotClicked(EquippedGridSlot, EquipmentTypeTag);
		return true;
	}

	return false;
}

void UINV_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);

	if (!IsValid(Button_Equippable) || !IsValid(Button_Consumable) || !IsValid(Button_Craftable) || !IsValid(Switcher))
	{
		UE_LOG(LogTemp, Error, TEXT("Spatial inventory is missing required category widgets."));
		return;
	}
	
	Button_Equippable->OnClicked.AddDynamic(this, &ThisClass::ShowEquippableGrid);
	Button_Consumable->OnClicked.AddDynamic(this, &ThisClass::ShowConsumableGrid);
	Button_Craftable->OnClicked.AddDynamic(this, &ThisClass::ShowCraftableGrid);
	
	ForEachInventoryGrid([this](UINV_InventoryGrid* Grid)
	{
		if (IsValid(Grid))
		{
			Grid->SetOwningCanvas(CanvasPanel);
		}
	});
	
	ShowEquippableGrid();
	EnsureControllerHintWidget();
	
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UINV_EquippedGridSlot* EquippedGridSlot { Cast<UINV_EquippedGridSlot>(Widget) }; IsValid(EquippedGridSlot))
		{
			EquippedGridSlots.Add(EquippedGridSlot);
			EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	});
}

void UINV_SpatialInventory::EnsureControllerHintWidget()
{
	if (!IsValid(CanvasPanel) || IsValid(ControllerHintsText)) return;

	ControllerHintsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_ControllerHintsRuntime"));
	if (!IsValid(ControllerHintsText)) return;

	ControllerHintsText->SetText(ControllerHintsLabel);
	ControllerHintsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f, 0.9f)));
	ControllerHintsText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	ControllerHintsText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));

	CanvasPanel->AddChild(ControllerHintsText);
	if (UCanvasPanelSlot* HintSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ControllerHintsText))
	{
		HintSlot->SetAutoSize(true);
		HintSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		HintSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		HintSlot->SetPosition(FVector2D(16.0f, -14.0f));
		HintSlot->SetZOrder(900);
	}
}

void UINV_SpatialInventory::EquippedGridSlotClicked(UINV_EquippedGridSlot* EquippedGridSlot,
	const FGameplayTag& EquipmentTypeTag)
{
	// Check if the hover item is equippable
	if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag)) return;
	
	UINV_HoverItem* HoverItem { GetHoverItem() };
	if (!IsValid(HoverItem)) return;
	UINV_InventoryItem* HoveredInventoryItem = HoverItem->GetInventoryItem();
	if (!IsValid(HoveredInventoryItem)) return;
	
	// Create the equipped slotted item and add it to the equipped grid slot (call EquippedGridSlot->OnItemEquipped)
	const float TileSize = ResolveInventoryTileSize();
	if (TileSize <= 0.f) return;

	FINV_EquippedSlotCallbacks SlotCallbacks;
	BuildEquippedSlotCallbacks(SlotCallbacks);
	FINV_EquippedSlotService::EquipItemInSlot(
		EquippedGridSlot,
		HoveredInventoryItem,
		EquipmentTypeTag,
		TileSize,
		SlotCallbacks);
	if (!IsValid(EquippedGridSlot->GetEquippedSlottedItem()))
	{
		return;
	}
	
	// Inform the server that we've equipped an item (potentially unequipping an item as well)
	UINV_InventoryComponent* InventoryComponent = ResolveInventoryComponent();
	checkf(InventoryComponent, TEXT("Inventory component is invalid"));
	BroadcastEquipState(InventoryComponent, HoveredInventoryItem, nullptr);
	
	// Clear the hover item
	if (IsValid(Grid_Equippable))
	{
		Grid_Equippable->ClearHoverItem();
	}
}

void UINV_SpatialInventory::EquippedSlottedItemClicked(UINV_EquippedSlottedItem* EquippedSlottedItem, const FPointerEvent& MouseEvent)
{
	const EINV_EquippedClickAction ClickAction = FINV_EquippedSlotService::ResolveClickAction(
		MouseEvent,
		IsValid(EquippedSlottedItem),
		IsValid(GetHoverItem()) && GetHoverItem()->IsStackable());

	switch (ClickAction)
	{
	case EINV_EquippedClickAction::Inspect:
		TryHandleEquippedItemInspect(EquippedSlottedItem, MouseEvent);
		return;
	case EINV_EquippedClickAction::Swap:
		SwapEquippedItemWithHover(EquippedSlottedItem);
		return;
	case EINV_EquippedClickAction::None:
	default:
		return;
	}
}

bool UINV_SpatialInventory::TryHandleEquippedItemInspect(
	UINV_EquippedSlottedItem* EquippedSlottedItem,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::RightMouseButton) return false;

	if (!IsValid(EquippedSlottedItem)) return true;

	UINV_InventoryItem* RightClickedItem { EquippedSlottedItem->GetInventoryItem() };
	if (!IsValid(RightClickedItem)) return true;
	UINV_InventoryStatics::ItemInspected(
		GetOwningPlayer(),
		RightClickedItem,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	return true;
}

void UINV_SpatialInventory::SwapEquippedItemWithHover(UINV_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	if (!IsValid(Grid_Equippable)) return;

	// Remove the Item Description
	UINV_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	
	// Get the item to equip
	UINV_InventoryItem* ItemToEquip { IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr };
	
	// Get the item to unequip
	UINV_InventoryItem* ItemToUnequip { EquippedSlottedItem->GetInventoryItem() };
	if (!IsValid(ItemToUnequip)) return;
	const FGameplayTag EquipmentTypeTag = EquippedSlottedItem->GetEquipmentTypeTag();
	
	// Get the Equipped Grid Slot holding this item
	UINV_EquippedGridSlot* EquippedGridSlot { FindSlotWithEquippedItem(ItemToUnequip) };
	
	// Clear the equipped grid slot of this item (set its inventory item to nullptr)
	FINV_EquippedSlotService::ClearSlot(EquippedGridSlot);
	
	// Assign the previously equipped item as the hover item
	Grid_Equippable->AssignHoverItem(ItemToUnequip);
	
	// Removal of the equipped slotted item from the equipped grid slot 
	FINV_EquippedSlotCallbacks SlotCallbacks;
	BuildEquippedSlotCallbacks(SlotCallbacks);
	FINV_EquippedSlotService::RemoveSlottedItem(EquippedSlottedItem, SlotCallbacks);
	
	// Make a new equipped slotted item (for the item we held in HoverItem)
	FINV_EquippedSlotService::EquipItemInSlot(
		EquippedGridSlot,
		ItemToEquip,
		EquipmentTypeTag,
		ResolveInventoryTileSize(),
		SlotCallbacks);
	
	// Broadcast delegate for OnItemEquipped/OnItemUnequipped (from the IC)
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

FReply UINV_SpatialInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->DropItem();
	}
	return FReply::Handled();
}

FINV_SlotAvailabilityResult UINV_SpatialInventory::HasRoomForItem(UINV_ItemComponent* ItemComponent) const
{
	if (!IsValid(ItemComponent))
	{
		return FINV_SlotAvailabilityResult();
	}

	const EINV_ItemCategory ItemCategory = UINV_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent);
	UINV_InventoryGrid* TargetGrid = GetGridForCategory(ItemCategory);
	if (!IsValid(TargetGrid))
	{
		return FINV_SlotAvailabilityResult();
	}

	return TargetGrid->HasRoomForItem(ItemComponent);
}

void UINV_SpatialInventory::OnItemInspected(UINV_InventoryItem* Item, const FVector2D& OpenPosition)
{
	OpenItemDescription(Item, OpenPosition);
}

bool UINV_SpatialInventory::HasHoverItem() const
{
	bool bHasHoverItem = false;
	ForEachInventoryGrid([&bHasHoverItem](UINV_InventoryGrid* Grid)
	{
		if (bHasHoverItem || !IsValid(Grid)) return;
		bHasHoverItem = Grid->HasHoverItem();
	});
	return bHasHoverItem;
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
	return IsValid(Grid_Equippable) ? Grid_Equippable->GetTileSize() : 0.f;
}

void UINV_SpatialInventory::ReturnActiveHoverItemToSource()
{
	UINV_InventoryGrid* HoverSourceGrid = FindGridWithHoverItem();
	if (!IsValid(HoverSourceGrid)) return;
	HoverSourceGrid->ReturnHoverItemToPreviousSlot();
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

float UINV_SpatialInventory::ResolveInventoryTileSize() const
{
	const UINV_InventoryBase* InventoryWidget = UINV_InventoryStatics::GetInventoryWidget(GetOwningPlayer());
	return IsValid(InventoryWidget) ? InventoryWidget->GetTileSize() : 0.f;
}

void UINV_SpatialInventory::BuildEquippedSlotCallbacks(FINV_EquippedSlotCallbacks& OutCallbacks)
{
	OutCallbacks.BindEquippedSlottedItemDelegates = [this](UINV_EquippedSlottedItem* SlottedItem)
	{
		BindEquippedSlottedItemDelegates(SlottedItem);
	};
	OutCallbacks.UnbindEquippedSlottedItemDelegates = [this](UINV_EquippedSlottedItem* SlottedItem)
	{
		UnbindEquippedSlottedItemDelegates(SlottedItem);
	};
}

void UINV_SpatialInventory::BroadcastSlotClickedDelegates(UINV_InventoryItem* ItemToEquip,
	UINV_InventoryItem* ItemToUnequip)
{
	UINV_InventoryComponent* InventoryComponent = ResolveInventoryComponent();
	checkf(InventoryComponent, TEXT("Inventory component is invalid"));
	BroadcastEquipState(InventoryComponent, ItemToEquip, ItemToUnequip);
}

UINV_InventoryComponent* UINV_SpatialInventory::ResolveInventoryComponent() const
{
	return UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
}

void UINV_SpatialInventory::BroadcastEquipState(
	UINV_InventoryComponent* InventoryComponent,
	UINV_InventoryItem* ItemToEquip,
	UINV_InventoryItem* ItemToUnequip) const
{
	if (!IsValid(InventoryComponent)) return;
	const APlayerController* OwningPlayer = GetOwningPlayer();
	if (!IsValid(OwningPlayer)) return;

	InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
	if (OwningPlayer->GetNetMode() == NM_DedicatedServer) return;

	InventoryComponent->OnItemEquipped.Broadcast(ItemToEquip);
	if (IsValid(ItemToUnequip))
	{
		InventoryComponent->OnItemUnequipped.Broadcast(ItemToUnequip);
	}
}

void UINV_SpatialInventory::BindEquippedSlottedItemDelegates(UINV_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
}

void UINV_SpatialInventory::UnbindEquippedSlottedItemDelegates(UINV_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
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

UINV_InventoryGrid* UINV_SpatialInventory::FindGridWithHoverItem() const
{
	UINV_InventoryGrid* GridWithHoverItem = nullptr;
	ForEachInventoryGrid([&GridWithHoverItem](UINV_InventoryGrid* Grid)
	{
		if (IsValid(GridWithHoverItem) || !IsValid(Grid)) return;
		if (Grid->HasHoverItem())
		{
			GridWithHoverItem = Grid;
		}
	});
	return GridWithHoverItem;
}

bool UINV_SpatialInventory::CanEquipHoverItem(UINV_EquippedGridSlot* EquippedGridSlot,
                                              const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;
	
	UINV_HoverItem* HoverItem { GetHoverItem() };
	if (!IsValid(HoverItem)) return false;
	
	UINV_InventoryItem* HeldItem { HoverItem->GetInventoryItem() };
	if (!IsValid(HeldItem)) return false;
	
	return !HoverItem->IsStackable() &&
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
