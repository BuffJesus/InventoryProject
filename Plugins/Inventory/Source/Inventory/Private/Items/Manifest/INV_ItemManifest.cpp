// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Manifest/INV_ItemManifest.h"
#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Composite/INV_Composite_Base.h"

UINV_InventoryItem* FINV_ItemManifest::CreateItem(UObject* NewOuter)
{
	// Create a new inventory item and copy this manifest into it.
	UINV_InventoryItem* Item { NewObject<UINV_InventoryItem>(NewOuter, UINV_InventoryItem::StaticClass()) };
	Item->SetItemManifest(*this);
	for (auto& Fragment : Item->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().InitializeRuntimeState();
	}
	// Control flow: this manifest instance is consumed after item creation.
	ClearFragments();

	return Item;
}

UINV_ItemComponent* FINV_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation,
	const FRotator& SpawnRotation, ESpawnActorCollisionHandlingMethod SpawnCollisionHandling) const
{
	if (!PickupActorClass || !IsValid(WorldContextObject)) return nullptr;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;
	AActor* SpawnedActor { WorldContextObject->GetWorld()->SpawnActor<AActor>(PickupActorClass, SpawnLocation, SpawnRotation, SpawnParams) };
	if (!SpawnedActor) return nullptr;
	
	// Set item manifest, category, item type, etc
	UINV_ItemComponent* ItemComp { SpawnedActor->FindComponentByClass<UINV_ItemComponent>() };
	checkf(ItemComp, TEXT("Spawned actor does not have an item component"));
	
	ItemComp->InitItemManifest(*this);
	return ItemComp;
}

void FINV_ItemManifest::AssimilateInventoryFragments(UINV_Composite_Base* Composite) const
{
	const auto& InventoryFragments { GetFragmentsOfType<FINV_InventoryItemFragment>() };
	for (const auto* Fragment : InventoryFragments)
	{
		Composite->ApplyFunction([Fragment](UINV_Composite_Base* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}

void FINV_ItemManifest::ClearFragments()
{
	// Empty() will destroy all elements, no need to Reset() first
	Fragments.Empty();
}
	
	
