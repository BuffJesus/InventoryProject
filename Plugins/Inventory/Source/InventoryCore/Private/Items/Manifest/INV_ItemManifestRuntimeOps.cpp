#include "Items/Manifest/INV_ItemManifestRuntimeOps.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Engine/World.h"

UINV_InventoryItem* FINV_ItemManifestRuntimeOps::CreateItemFromManifest(FINV_ItemManifest& SourceManifest, UObject* NewOuter)
{
	if (!IsValid(NewOuter)) return nullptr;

	UINV_InventoryItem* Item = NewObject<UINV_InventoryItem>(NewOuter, UINV_InventoryItem::StaticClass());
	if (!IsValid(Item)) return nullptr;

	Item->SetItemManifest(SourceManifest);
	for (TInstancedStruct<FInv_ItemFragment>& Fragment : Item->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().InitializeRuntimeState();
	}

	if (const FINV_StackableFragment* StackableFragment = Item->GetCachedStackableFragment())
	{
		Item->SetTotalStackCount(FMath::Max(1, StackableFragment->GetStackCount()));
	}

	// Source pickup manifest is consumed after runtime item creation.
	SourceManifest.GetFragmentsMutable().Empty();
	return Item;
}

UINV_ItemComponent* FINV_ItemManifestRuntimeOps::SpawnPickupActorFromManifest(
	const FINV_ItemManifest& Manifest,
	const UObject* WorldContextObject,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	const ESpawnActorCollisionHandlingMethod SpawnCollisionHandling)
{
	if (!IsValid(WorldContextObject)) return nullptr;
	if (!Manifest.GetPickupActorClass()) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;
	AActor* SpawnedActor = WorldContextObject->GetWorld()->SpawnActor<AActor>(
		Manifest.GetPickupActorClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParams);
	if (!SpawnedActor) return nullptr;

	UINV_ItemComponent* ItemComp = SpawnedActor->FindComponentByClass<UINV_ItemComponent>();
	checkf(ItemComp, TEXT("Spawned actor does not have an item component"));
	ItemComp->InitItemManifest(Manifest);
	return ItemComp;
}
