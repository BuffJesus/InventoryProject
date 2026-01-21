// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Items/INV_InventoryItem.h"

void UINV_HoverItem::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PositionTimerHandle,
			this,
			&ThisClass::UpdatePosition,
			PositionUpdateRate,
			true // looping
		);
	}
}

void UINV_HoverItem::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PositionTimerHandle);
	}
    
	Super::NativeDestruct();
}

void UINV_HoverItem::UpdatePosition()
{
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this) - (GetDesiredSize() / 2);
	SetPositionInViewport(MousePosition, true);
}

void UINV_HoverItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UINV_HoverItem::UpdateStackCount(const int32 Count) const
{
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
	if (InventoryItem.IsValid())
	{
		return InventoryItem->GetItemManifest().GetItemType();
	}
	return FGameplayTag::EmptyTag;
}

void UINV_HoverItem::SetIsStackable(bool bStacks)
{
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
