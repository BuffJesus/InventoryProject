// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Types/INV_GridTypes.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_ItemManifest.generated.h"

struct FInv_ItemFragment;

// The item manifest contains all necessary data to create a new inventory item

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemManifest
{
	GENERATED_BODY()
	// Mutable access to raw fragment storage (used when constructing runtime items).
	TArray<TInstancedStruct<FInv_ItemFragment>>& GetFragmentsMutable() { return Fragments; }
	// High-level category for UI placement.
	EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	// Gameplay tag that identifies the item type.
	FGameplayTag GetItemType() const { return ItemType; }
	// Pickup actor class for world spawn flow.
	TSubclassOf<AActor> GetPickupActorClass() const { return PickupActorClass; }
	
	// Find the first fragment of T that matches the provided fragment tag.
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;
	
	// Find the first fragment of type T.
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfType() const;
	
	// Find the first mutable fragment of type T.
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	T* GetFragmentOfTypeMutable();
	
	// Collect all fragments of type T.
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	TArray<const T*> GetFragmentsOfType() const;
	
private:
	// Item fragments that describe behavior and UI.
	UPROPERTY(EditAnywhere,Category="Inventory",meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FInv_ItemFragment>> Fragments;
	
	// Item category used by grids.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	EINV_ItemCategory ItemCategory { EINV_ItemCategory::None };
	
	// Gameplay tag for item type.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory", meta = (Categories = "GameItems"))
	FGameplayTag ItemType;
	
	// Pickup actor class used when dropping/spawning this item in the world.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<AActor> PickupActorClass;
	
};

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
const T* FINV_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr { Fragment.GetPtr<T>() })
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTagExact(FragmentTag)) continue;
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
const T* FINV_ItemManifest::GetFragmentOfType() const
{
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr { Fragment.GetPtr<T>() })
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
T* FINV_ItemManifest::GetFragmentOfTypeMutable()
{
	for (TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (T* FragmentPtr { Fragment.GetMutablePtr<T>() })
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
TArray<const T*> FINV_ItemManifest::GetFragmentsOfType() const
{
	TArray<const T*> Results;
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr { Fragment.GetPtr<T>() })
		{
			Results.Add(FragmentPtr);
		}
	}
	return Results;
}




