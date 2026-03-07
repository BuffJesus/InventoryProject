// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "INV_ItemComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYCORE_API UINV_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_ItemComponent();
	virtual void OnRegister() override;
	// UI text shown when the item can be picked up.
	FORCEINLINE const FString& GetPickupMessage() const { return PickupMessage; }
	// Static item data for this pickup.
	FORCEINLINE const FINV_ItemManifest& GetItemManifest() const { return ItemManifest; }
	// Mutable access for updating fragments (e.g. stack counts).
	FORCEINLINE FINV_ItemManifest& GetItemManifestMutable() { return ItemManifest; }
	// Whether rarity should be used for this item.
	FORCEINLINE bool IsItemRarityEnabled() const { return bUseItemRarity; }
	// Item rarity tag used for UI styling.
	FORCEINLINE const FGameplayTag& GetItemRarityTag() const { return ItemRarityTag; }
	// Runtime setter used when dropping items back into the world.
	void SetItemRarityOptions(bool bEnabled, const FGameplayTag& InItemRarityTag);
	// Whether the actor should be destroyed on pickup.
	FORCEINLINE bool GetDestroyOnPickup() const { return bIsDestroyedOnPickup; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// Called when the item is picked up.
	void PickedUp();
	// Copy-initialize pickup manifest data (usually from dropped/runtime items).
	void InitItemManifest(const FINV_ItemManifest& InManifest);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="INV|Inventory")
	void OnPickedUp();
	
private:
	// Item data to create inventory entries.
	UPROPERTY(Replicated, EditAnywhere, Category="INV|Inventory")
	FINV_ItemManifest ItemManifest;
	
	// Message shown by the HUD when aimed at.
	UPROPERTY(EditAnywhere, Category="INV|Inventory")
	FString PickupMessage { TEXT("Null Message") };

	// Enables per-item rarity styling support.
	UPROPERTY(Replicated, EditAnywhere, Category="INV|Inventory|Rarity")
	bool bUseItemRarity { false };

	// Rarity tag used when bUseItemRarity is enabled.
	UPROPERTY(Replicated, EditAnywhere, Category="INV|Inventory|Rarity", meta = (Categories = "FragmentTags.Rarity", EditCondition = "bUseItemRarity", EditConditionHides))
	FGameplayTag ItemRarityTag { FGameplayTag::EmptyTag };
	
	// If true, destroy the actor after pickup.
	UPROPERTY(EditAnywhere, Category="INV|Inventory")
	bool bIsDestroyedOnPickup { false };
	
};
