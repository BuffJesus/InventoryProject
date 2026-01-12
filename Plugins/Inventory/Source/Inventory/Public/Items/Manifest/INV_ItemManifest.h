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
	
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;
	
private:
	UPROPERTY(EditAnywhere,Category="Inventory",meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FInv_ItemFragment>> Fragments;
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory", meta = (Categories = "GameItems"))
	FGameplayTag ItemType;
};

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
const T* FINV_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr { Fragment.GetPtr<>() })
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTagExact(FragmentTag)) continue;
			return FragmentPtr;
		}
	}
	return nullptr;
}


