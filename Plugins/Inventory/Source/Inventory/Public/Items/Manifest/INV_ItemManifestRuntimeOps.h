#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class UINV_InventoryItem;
class UINV_ItemComponent;
struct FINV_ItemManifest;

class INVENTORY_API FINV_ItemManifestRuntimeOps
{
public:
	static UINV_InventoryItem* CreateItemFromManifest(FINV_ItemManifest& SourceManifest, UObject* NewOuter);

	static UINV_ItemComponent* SpawnPickupActorFromManifest(
		const FINV_ItemManifest& Manifest,
		const UObject* WorldContextObject,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation,
		ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
};
