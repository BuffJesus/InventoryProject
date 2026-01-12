// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_ItemManifest.generated.h"

struct FInv_ItemFragment;
class UINV_InventoryItem;

// The item manifest contains all necessary data to create a new inventory item

USTRUCT(BlueprintType)
struct INVENTORY_API FINV_ItemManifest
{
	GENERATED_BODY()
	UINV_InventoryItem* CreateItem(UObject* NewOuter);
	EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	FGameplayTag GetItemType() const { return ItemType; }
	
private:
	UPROPERTY(EditAnywhere,Category="Inventory",meta = (ExcludeBaseStruct, BaseStruct = "/Script/Inventory.Inv_ItemFragment"))
	TArray<TInstancedStruct<FInv_ItemFragment>> Fragments;
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory", meta = (Categories = "GameItems"))
	FGameplayTag ItemType;
};

