// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_ItemComponent.h"

#include "Net/UnrealNetwork.h"

UINV_ItemComponent::UINV_ItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UINV_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate static manifest data for this pickup.
	DOREPLIFETIME(ThisClass, ItemManifest);
}

void UINV_ItemComponent::InitItemManifest(const FINV_ItemManifest& InManifest)
{
	ItemManifest = InManifest;
}

void UINV_ItemComponent::SetItemRarityOptions(bool bEnabled, const FGameplayTag& InItemRarityTag)
{
	bUseItemRarity = bEnabled;
	ItemRarityTag = bUseItemRarity ? InItemRarityTag : FGameplayTag::EmptyTag;
}

void UINV_ItemComponent::PickedUp()
{
	// Notify BP and optionally destroy the actor.
	OnPickedUp();
	if (GetDestroyOnPickup() == true)
	{
		GetOwner()->Destroy();
	}
}
