// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Manifest/INV_ItemManifest.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"

UINV_InventoryItem* FINV_ItemManifest::CreateItem(UObject* NewOuter) const
{
	// Create a new inventory item and copy this manifest into it.
	UINV_InventoryItem* Item { NewObject<UINV_InventoryItem>(NewOuter, UINV_InventoryItem::StaticClass()) };
	Item->SetItemManifest(*this);
	
	return Item;
}

void FINV_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation,
	const FRotator& SpawnRotation, ESpawnActorCollisionHandlingMethod SpawnCollisionHandling)
{
	if (!PickupActorClass || !IsValid(WorldContextObject)) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;
	AActor* SpawnedActor { WorldContextObject->GetWorld()->SpawnActor<AActor>(PickupActorClass, SpawnLocation, SpawnRotation, SpawnParams) };
	if (!SpawnedActor) return;
	
	// Set item manifest, category, item type, etc
	UINV_ItemComponent* ItemComp { SpawnedActor->FindComponentByClass<UINV_ItemComponent>() };
	checkf(ItemComp, TEXT("Spawned actor does not have an item component"));
	
	ItemComp->InitItemManifest(*this);
}
	
	
