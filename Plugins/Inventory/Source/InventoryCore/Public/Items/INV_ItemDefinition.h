// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "INV_ItemDefinition.generated.h"

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemDefinitionHandle
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Definition")
	FString DefinitionKey;

	bool IsValid() const
	{
		return !DefinitionKey.IsEmpty();
	}

	void Reset()
	{
		DefinitionKey.Reset();
	}

	bool operator==(const FINV_ItemDefinitionHandle& Other) const
	{
		return DefinitionKey == Other.DefinitionKey;
	}
};

UCLASS()
class INVENTORYCORE_API UINV_ItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	void InitializeFromManifest(const FINV_ItemManifest& Manifest, const FINV_ItemDefinitionHandle& InHandle);

	const FINV_ItemManifest& GetItemManifest() const
	{
		return ItemManifest.Get<FINV_ItemManifest>();
	}

	const FINV_ItemDefinitionHandle& GetDefinitionHandle() const
	{
		return DefinitionHandle;
	}

	static FINV_ItemDefinitionHandle BuildHandleFromManifest(const FINV_ItemManifest& Manifest);
	static UINV_ItemDefinition* FindOrCreateSharedDefinition(const FINV_ItemManifest& Manifest, UObject* OuterContext);

private:
	UPROPERTY(VisibleAnywhere, Category = "INV|Definition", meta = (BaseStruct = "/Script/InventoryCore.INV_ItemManifest"))
	FInstancedStruct ItemManifest;

	UPROPERTY(VisibleAnywhere, Category = "INV|Definition")
	FINV_ItemDefinitionHandle DefinitionHandle;
};
