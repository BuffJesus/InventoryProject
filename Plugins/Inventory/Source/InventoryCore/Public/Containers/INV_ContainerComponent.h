// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Containers/INV_ContainerTypes.h"
#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "InventoryManagement/Placement/INV_InventoryPlacementTypes.h"
#include "INV_ContainerComponent.generated.h"

class AController;
class UINV_InventoryItem;
class UINV_ItemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FINV_ContainerItemEvent, UINV_InventoryItem*, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYCORE_API UINV_ContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_ContainerComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	const FINV_ContainerMetadata& GetContainerMetadata() const { return ContainerMetadata; }

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	FText GetDisplayName() const { return ContainerMetadata.DisplayName; }

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	FName GetPersistentContainerId() const { return PersistentContainerId; }

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	const FIntPoint& GetGridSize() const { return GridSize; }

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	TArray<UINV_InventoryItem*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	bool HasItem(UINV_InventoryItem* Item) const;

	bool GetItemPlacement(const UINV_InventoryItem* Item, FINV_InventoryItemPlacement& OutPlacement) const;
	bool FindAvailablePlacementForItem(const UINV_InventoryItem* Item, FINV_InventoryItemPlacement& OutPlacement, const UINV_InventoryItem* IgnoredItem = nullptr) const;
	bool CanPlaceItemAt(const UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement, const UINV_InventoryItem* IgnoredItem = nullptr) const;
	bool SetItemPlacement(UINV_InventoryItem* Item, const FINV_InventoryItemPlacement& Placement);

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	bool IsEmpty() const;

	UFUNCTION(BlueprintCallable, Category = "INV|Container")
	bool CanAccess(AController* Controller) const;

	UFUNCTION(BlueprintNativeEvent, Category = "INV|Container")
	bool CanAccessContainer(AController* Controller) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "INV|Container")
	UINV_InventoryItem* AddItem(UINV_InventoryItem* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "INV|Container")
	UINV_InventoryItem* AddItemFromPickup(UINV_ItemComponent* ItemComponent);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "INV|Container")
	bool RemoveItem(UINV_InventoryItem* Item);

	UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType, bool bUseItemRarity, const FGameplayTag& ItemRarityTag);
	const UINV_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType, bool bUseItemRarity, const FGameplayTag& ItemRarityTag) const;

	UPROPERTY(BlueprintAssignable, Category = "INV|Container")
	FINV_ContainerItemEvent OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "INV|Container")
	FINV_ContainerItemEvent OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "INV|Container")
	FINV_ContainerItemEvent OnItemChanged;

	void NotifyItemChanged(UINV_InventoryItem* Item);

private:
	void ConfigureFastArrayCallbacks();
	void InitializeStarterContents();
	bool HasAuthorityOnOwner() const;
	bool AssignPlacementForItem(UINV_InventoryItem* Item);

	UPROPERTY(Replicated, EditAnywhere, Category = "INV|Container")
	FINV_ContainerMetadata ContainerMetadata;

	UPROPERTY(Replicated, EditAnywhere, Category = "INV|Container")
	FIntPoint GridSize { 6, 6 };

	UPROPERTY(EditAnywhere, Category = "INV|Container|Persistence")
	FName PersistentContainerId { NAME_None };

	UPROPERTY(Replicated)
	FINV_InventoryFastArray InventoryFastArray;

	UPROPERTY(EditAnywhere, Category = "INV|Container")
	bool bInitializeStarterContentsOnBeginPlay { true };

	UPROPERTY(EditAnywhere, Category = "INV|Container")
	TArray<FINV_ContainerStarterItem> StarterContents;

	bool bStarterContentsInitialized { false };
};
