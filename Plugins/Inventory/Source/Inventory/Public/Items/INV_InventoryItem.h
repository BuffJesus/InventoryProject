// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manifest/INV_ItemManifest.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_InventoryItem.generated.h"

struct FGameplayTag;
struct FINV_ItemManifest;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_InventoryItem : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	
	// Store the item manifest inside this object.
	FORCEINLINE void SetItemManifest(const FINV_ItemManifest& Manifest)
	{
		ItemManifest = FInstancedStruct::Make<FINV_ItemManifest>(Manifest);
	}
	// Access the stored manifest.
	const FINV_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FINV_ItemManifest>(); }
	FINV_ItemManifest& GetItemManifestMutable() { return ItemManifest.GetMutable<FINV_ItemManifest>(); }
	bool IsStackable() const;
	// Total stack count across all slots.
	FORCEINLINE int32 GetTotalStackCount() const { return TotalStackCount; }
	FORCEINLINE void SetTotalStackCount(int32 Count) { TotalStackCount = Count; }
	
private:
	// Instanced manifest (fragments + tags).
	UPROPERTY(VisibleAnywhere, Category = "INV|Inventory", meta = (BaseStruct = "/Script/Inventory.INV_ItemManifest"), Replicated)
	FInstancedStruct ItemManifest;
	
	// Total stacks stored for this item.
	UPROPERTY(Replicated) int32 TotalStackCount { 0 };
};

template <typename FragmentType>
const FragmentType* GetFragment(const UINV_InventoryItem* Item, const FGameplayTag& Tag)
{
	// Helper to fetch a fragment by tag from an item.
	if (!IsValid(Item)) return nullptr;
	
	const FINV_ItemManifest& Manifest { Item->GetItemManifest() };
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}
