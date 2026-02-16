// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Types/INV_GridTypes.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_ItemManifest.generated.h"

class UINV_Composite_Base;
struct FInv_ItemFragment;
class UINV_InventoryItem;

// The item manifest contains all necessary data to create a new inventory item

USTRUCT(BlueprintType)
struct INVENTORY_API FINV_ItemManifest
{
	GENERATED_BODY()
	// Create an inventory item object from this manifest.
	UINV_InventoryItem* CreateItem(UObject* NewOuter) const;
	// High-level category for UI placement.
	EINV_ItemCategory GetItemCategory() const { return ItemCategory; }
	// Gameplay tag that identifies the item type.
	FGameplayTag GetItemType() const { return ItemType; }
	
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;
	
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfType() const;
	
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	T* GetFragmentOfTypeMutable();
	
	template <typename T> requires std::derived_from<T, FInv_ItemFragment>
	TArray<const T*> GetFragmentsOfType() const;
	
	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation,
		ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	void AssimilateInventoryFragments(UINV_Composite_Base* Composite) const;
	
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




