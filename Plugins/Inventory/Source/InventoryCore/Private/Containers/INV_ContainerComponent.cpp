// Fill out your copyright notice in the Description page of Project Settings.

#include "Containers/INV_ContainerComponent.h"

#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Manifest/INV_ItemManifestRuntimeOps.h"
#include "Net/UnrealNetwork.h"

UINV_ContainerComponent::UINV_ContainerComponent() : InventoryFastArray(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UINV_ContainerComponent::OnRegister()
{
	Super::OnRegister();
	ConfigureFastArrayCallbacks();
}

void UINV_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFastArrayCallbacks();

	if (bInitializeStarterContentsOnBeginPlay && HasAuthorityOnOwner())
	{
		InitializeStarterContents();
	}
}

void UINV_ContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ContainerMetadata);
	DOREPLIFETIME(ThisClass, GridSize);
	DOREPLIFETIME(ThisClass, InventoryFastArray);
}

TArray<UINV_InventoryItem*> UINV_ContainerComponent::GetAllItems() const
{
	return InventoryFastArray.GetAllItems();
}

bool UINV_ContainerComponent::IsEmpty() const
{
	return InventoryFastArray.GetAllItems().Num() == 0;
}

bool UINV_ContainerComponent::CanAccess(AController* Controller) const
{
	return CanAccessContainer(Controller);
}

bool UINV_ContainerComponent::CanAccessContainer_Implementation(AController* Controller) const
{
	return true;
}

UINV_InventoryItem* UINV_ContainerComponent::AddItem(UINV_InventoryItem* Item)
{
	if (!HasAuthorityOnOwner() || !IsValid(Item))
	{
		return nullptr;
	}

	UINV_InventoryItem* AddedItem = InventoryFastArray.AddEntry(Item);
	if (IsValid(AddedItem))
	{
		OnItemAdded.Broadcast(AddedItem);
	}
	return AddedItem;
}

UINV_InventoryItem* UINV_ContainerComponent::AddItemFromPickup(UINV_ItemComponent* ItemComponent)
{
	if (!HasAuthorityOnOwner() || !IsValid(ItemComponent))
	{
		return nullptr;
	}

	UINV_InventoryItem* AddedItem = InventoryFastArray.AddEntry(ItemComponent);
	if (IsValid(AddedItem))
	{
		OnItemAdded.Broadcast(AddedItem);
	}
	return AddedItem;
}

bool UINV_ContainerComponent::RemoveItem(UINV_InventoryItem* Item)
{
	if (!HasAuthorityOnOwner() || !IsValid(Item))
	{
		return false;
	}

	InventoryFastArray.RemoveEntry(Item);
	OnItemRemoved.Broadcast(Item);
	return true;
}

UINV_InventoryItem* UINV_ContainerComponent::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag)
{
	return InventoryFastArray.FindFirstItemByType(ItemType, bUseItemRarity, ItemRarityTag);
}

const UINV_InventoryItem* UINV_ContainerComponent::FindFirstItemByType(const FGameplayTag& ItemType, const bool bUseItemRarity,
	const FGameplayTag& ItemRarityTag) const
{
	return InventoryFastArray.FindFirstItemByType(ItemType, bUseItemRarity, ItemRarityTag);
}

void UINV_ContainerComponent::ConfigureFastArrayCallbacks()
{
	FINV_FastArrayCallbacks Callbacks;
	Callbacks.CreateItemFromPickup = [](UINV_ItemComponent* ItemComponent, AActor* OwningActor) -> UINV_InventoryItem*
	{
		if (!IsValid(ItemComponent) || !IsValid(OwningActor))
		{
			return nullptr;
		}

		UINV_InventoryItem* Item = FINV_ItemManifestRuntimeOps::CreateItemFromManifest(
			ItemComponent->GetItemManifestMutable(),
			OwningActor);
		if (!IsValid(Item))
		{
			return nullptr;
		}

		Item->SetItemRarityOptions(ItemComponent->IsItemRarityEnabled(), ItemComponent->GetItemRarityTag());
		return Item;
	};
	Callbacks.MatchesItemByTypeAndRarity = [](const UINV_InventoryItem* Item, const FGameplayTag& ItemType, const bool bUseItemRarity, const FGameplayTag& ItemRarityTag)
	{
		return UINV_InventoryItem::MatchesTypeAndRarity(Item, ItemType, bUseItemRarity, ItemRarityTag);
	};
	Callbacks.OnItemAdded = [this](UINV_InventoryItem* Item)
	{
		OnItemAdded.Broadcast(Item);
	};
	Callbacks.OnItemRemoved = [this](UINV_InventoryItem* Item)
	{
		OnItemRemoved.Broadcast(Item);
	};
	Callbacks.RegisterReplicatedSubObject = [this](UObject* SubObj)
	{
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
		{
			AddReplicatedSubObject(SubObj);
		}
	};
	Callbacks.UnregisterReplicatedSubObject = [this](UObject* SubObj)
	{
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
		{
			RemoveReplicatedSubObject(SubObj);
		}
	};
	Callbacks.CanUseReplicationSubObjectList = [this]()
	{
		return IsUsingRegisteredSubObjectList() && IsReadyForReplication();
	};
	InventoryFastArray.SetCallbacks(MoveTemp(Callbacks));
}

void UINV_ContainerComponent::InitializeStarterContents()
{
	if (bStarterContentsInitialized)
	{
		return;
	}

	bStarterContentsInitialized = true;

	for (const FINV_ContainerStarterItem& StarterItem : StarterContents)
	{
		FINV_ItemManifest ManifestCopy = StarterItem.ItemManifest;
		UINV_InventoryItem* Item = FINV_ItemManifestRuntimeOps::CreateItemFromManifest(ManifestCopy, GetOwner());
		if (!IsValid(Item))
		{
			continue;
		}

		Item->SetItemRarityOptions(StarterItem.bUseItemRarity, StarterItem.ItemRarityTag);
		if (StarterItem.StackCount > 0)
		{
			Item->SetTotalStackCount(StarterItem.StackCount);
		}

		AddItem(Item);
	}
}

bool UINV_ContainerComponent::HasAuthorityOnOwner() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) && OwnerActor->HasAuthority();
}
