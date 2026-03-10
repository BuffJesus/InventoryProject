// Fill out your copyright notice in the Description page of Project Settings.

#include "Containers/INV_ContainerComponent.h"

#include "Items/INV_InventoryItem.h"
#include "Items/INV_ItemComponent.h"
#include "Items/Fragments/INV_ItemFragment.h"
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

void UINV_ContainerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	InventoryFastArray.ClearCallbacks();
	Super::EndPlay(EndPlayReason);
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

bool UINV_ContainerComponent::HasItem(UINV_InventoryItem* Item) const
{
	return IsValid(Item) && InventoryFastArray.ContainsItem(Item);
}

bool UINV_ContainerComponent::GetItemPlacement(const UINV_InventoryItem* Item, FINV_InventoryItemPlacement& OutPlacement) const
{
	return InventoryFastArray.GetItemPlacement(Item, OutPlacement);
}

bool UINV_ContainerComponent::FindAvailablePlacementForItem(
	const UINV_InventoryItem* Item,
	FINV_InventoryItemPlacement& OutPlacement,
	const UINV_InventoryItem* IgnoredItem) const
{
	return InventoryFastArray.TryFindFirstAvailablePlacement(
		Item,
		GridSize,
		OutPlacement,
		[](const UINV_InventoryItem*) { return true; },
		IgnoredItem);
}

bool UINV_ContainerComponent::CanPlaceItemAt(
	const UINV_InventoryItem* Item,
	const FINV_InventoryItemPlacement& Placement,
	const UINV_InventoryItem* IgnoredItem) const
{
	return InventoryFastArray.CanPlaceItemAt(
		Item,
		Placement,
		GridSize,
		[](const UINV_InventoryItem*) { return true; },
		IgnoredItem);
}

bool UINV_ContainerComponent::SetItemPlacement(UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement)
{
	return InventoryFastArray.SetItemPlacement(Item, Placement);
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
	if (!IsValid(AddedItem))
	{
		return nullptr;
	}

	if (!AssignPlacementForItem(AddedItem))
	{
		InventoryFastArray.RemoveEntry(AddedItem);
		return nullptr;
	}

	OnItemAdded.Broadcast(AddedItem);
	return AddedItem;
}

UINV_InventoryItem* UINV_ContainerComponent::AddItemAtPlacement(UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement)
{
	if (!HasAuthorityOnOwner() || !IsValid(Item) || !CanPlaceItemAt(Item, Placement))
	{
		return nullptr;
	}

	UINV_InventoryItem* AddedItem = InventoryFastArray.AddEntry(Item);
	if (!IsValid(AddedItem))
	{
		return nullptr;
	}

	if (!InventoryFastArray.SetItemPlacement(AddedItem, Placement))
	{
		InventoryFastArray.RemoveEntry(AddedItem);
		return nullptr;
	}

	OnItemAdded.Broadcast(AddedItem);
	return AddedItem;
}

UINV_InventoryItem* UINV_ContainerComponent::AddItemFromPickup(UINV_ItemComponent* ItemComponent)
{
	if (!HasAuthorityOnOwner() || !IsValid(ItemComponent))
	{
		return nullptr;
	}

	UINV_InventoryItem* AddedItem = InventoryFastArray.AddEntry(ItemComponent);
	if (!IsValid(AddedItem))
	{
		return nullptr;
	}

	if (!AssignPlacementForItem(AddedItem))
	{
		InventoryFastArray.RemoveEntry(AddedItem);
		return nullptr;
	}

	OnItemAdded.Broadcast(AddedItem);
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

void UINV_ContainerComponent::NotifyItemChanged(UINV_InventoryItem* Item)
{
	if (IsValid(Item))
	{
		OnItemChanged.Broadcast(Item);
	}
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
	Callbacks.OnItemChanged = [this](UINV_InventoryItem* Item)
	{
		OnItemChanged.Broadcast(Item);
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
			if (FINV_StackableFragment* StackableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FINV_StackableFragment>())
			{
				StackableFragment->SetStackCount(StarterItem.StackCount);
			}
		}

		AddItem(Item);
	}
}

bool UINV_ContainerComponent::HasAuthorityOnOwner() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) && OwnerActor->HasAuthority();
}

bool UINV_ContainerComponent::AssignPlacementForItem(UINV_InventoryItem* Item)
{
	return InventoryFastArray.TryAutoPlaceItem(
		Item,
		GridSize,
		[](const UINV_InventoryItem*) { return true; },
		Item);
}
