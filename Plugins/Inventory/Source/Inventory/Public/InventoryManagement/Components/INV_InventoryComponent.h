// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "INV_InventoryComponent.generated.h"

struct FINV_SlotAvailabilityResult;
struct FINV_InventoryFastArray;
class UINV_ItemComponent;
class UINV_InventoryBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, UINV_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNoRoomInInventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStackChange, const FINV_SlotAvailabilityResult&, Result);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UINV_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_InventoryComponent();
	// Shows or hides the inventory UI widget.
	void ToggleInventoryMenu();
	// Registers a UObject for replication when using the subobject list.
	void AddRepSubObj(UObject* SubObj);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(UINV_ItemComponent* ItemComponent, int32 StackCount);
	
	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(UINV_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder);
	
	UFUNCTION(BlueprintCallable, Category="INV|Inventory", BlueprintAuthorityOnly)
	// Entry point for adding an item to the inventory.
	void TryAddItem(UINV_ItemComponent* ItemComponent);
	
	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;
	FNoRoomInInventory OnNoRoomInInventory;
	FStackChange OnStackChange;

protected:
	virtual void BeginPlay() override;

private:
	// Owning controller used for UI input mode and widget creation.
	TWeakObjectPtr<APlayerController> OwningController { nullptr };
	
	// Creates the inventory UI widget for local controllers.
	void ConstructInventory();
	// Sets UI visibility and input mode.
	void HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen);
	
	// Replicated list of inventory items.
	UPROPERTY(Replicated)
	FINV_InventoryFastArray InventoryFastArray;

	// Widget class used to build the inventory UI.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<UINV_InventoryBase> InventoryClass { nullptr };
	
	// Actual widget instance for this component.
	UPROPERTY()
	TObjectPtr<UINV_InventoryBase> Inventory { nullptr };
	
	// Tracks menu state for toggle logic.
	bool bInventoryMenuOpen = false;
};
