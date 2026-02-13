// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Items/INV_InventoryItem.h"

void UINV_HoverItem::Clear()
{
	// Reset all hover state and visuals.
	SetInventoryItem(nullptr);
	SetIsStackable(false);
	SetPreviousGridIndex(INDEX_NONE);
	SetStackCount(0);
	SetImageBrush(FSlateNoResource());
}

void UINV_HoverItem::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
    
	// Keep the hover widget centered on the mouse.
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	SetPositionInViewport(MousePosition - (CachedSize / 2.f), false);
}

void UINV_HoverItem::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// Hover item should not block input.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UINV_HoverItem::SetImageBrush(const FSlateBrush& Brush) const
{
	// Apply the icon brush.
	Image_Icon->SetBrush(Brush);
}

void UINV_HoverItem::UpdateStackCount(const int32 Count)
{
	StackCount = Count;
	
	// Update stack count text.
	if (Count > 0)
	{
		Text_StackCount->SetText(FText::AsNumber(Count));
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FGameplayTag UINV_HoverItem::GetItemType() const
{
	// Pull the item tag from the backing item.
	if (InventoryItem.IsValid())
	{
		return InventoryItem->GetItemManifest().GetItemType();
	}
	return FGameplayTag::EmptyTag;
}

void UINV_HoverItem::SetIsStackable(bool bStacks)
{
	// Hide stack count when not stackable.
	bIsStackable = bStacks;
	if (!bStacks)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UINV_InventoryItem* UINV_HoverItem::GetInventoryItem() const
{
	return InventoryItem.Get();
}

void UINV_HoverItem::SetInventoryItem(UINV_InventoryItem* Item)
{
	InventoryItem = Item;
}
