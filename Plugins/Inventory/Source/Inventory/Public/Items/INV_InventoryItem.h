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
	
	FORCEINLINE void SetItemManifest(const FINV_ItemManifest& Manifest)
	{
		ItemManifest = FInstancedStruct::Make<FINV_ItemManifest>(Manifest);
	}
	const FINV_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FINV_ItemManifest>(); }
	FINV_ItemManifest& GetItemManifestMutable() { return ItemManifest.GetMutable<FINV_ItemManifest>(); }
	bool IsStackable() const;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "INV|Inventory", meta = (BaseStruct = "/Script/Inventory.INV_ItemManifest"), Replicated)
	FInstancedStruct ItemManifest;
};

template <typename FragmentType>
const FragmentType* GetFragment(const UINV_InventoryItem* Item, const FGameplayTag& Tag)
{
	if (!IsValid(Item)) return nullptr;
	
	const FINV_ItemManifest& Manifest { Item->GetItemManifest() };
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}