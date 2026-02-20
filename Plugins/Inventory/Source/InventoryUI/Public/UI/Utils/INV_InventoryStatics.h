// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "INV_InventoryStatics.generated.h"

class UINV_InventoryItem;
class UINV_ItemComponent;
class UINV_InventoryBase;
enum class EINV_ItemCategory : uint8;
class UINV_InventoryComponent;
/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_InventoryStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Helper to find the inventory component on a controller.
	UFUNCTION(BlueprintCallable, Category="INV|Statics")
	static UINV_InventoryComponent* GetInventoryComponent(const APlayerController* PlayerController);
	
	// Convenience for pulling category from an item component.
	UFUNCTION(BlueprintCallable, Category="INV|Statics")
	static EINV_ItemCategory GetItemCategoryFromItemComp(UINV_ItemComponent* ItemComponent);
	static bool TryGetInventoryBase(APlayerController* PC, UINV_InventoryBase*& OutInventoryBase);
	
	UFUNCTION(BlueprintCallable, Category="INV|Statics")
	static void ItemHovered(APlayerController* PC, UINV_InventoryItem* Item);
	
	UFUNCTION(BlueprintCallable, Category="INV|Statics")
	static void ItemUnhovered(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category="INV|Statics")
	static void ItemInspected(APlayerController* PC, UINV_InventoryItem* Item, const FVector2D& OpenPosition);
};
