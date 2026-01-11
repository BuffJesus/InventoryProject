// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_InventoryItem.generated.h"

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
	
private:
	UPROPERTY(VisibleAnywhere, Category = "INV|Inventory", meta = (BaseStruct = "/Script/Inventory.INV_ItemManifest"), Replicated)
	FInstancedStruct ItemManifest;
};
