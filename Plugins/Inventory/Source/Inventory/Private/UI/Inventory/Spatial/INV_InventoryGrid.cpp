// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Spatial/INV_InventoryGrid.h"
#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
#include "InventoryManagement/Utils/INV_InventoryStatics.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UI/INV_WidgetUtils.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"

void UINV_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ConstructGrid();
	
	InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
}

void UINV_InventoryGrid::AddItem(UINV_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Adding item to inventory grid"));
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

bool UINV_InventoryGrid::MatchesCategory(const UINV_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}
