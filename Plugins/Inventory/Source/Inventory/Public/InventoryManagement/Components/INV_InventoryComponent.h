// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "INV_InventoryComponent.generated.h"

class UINV_InventoryBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, UINV_InventoryItem*, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UINV_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_InventoryComponent();
	void ToggleInventoryMenu();
	
	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<APlayerController> OwningController { nullptr };
	
	void ConstructInventory();
	void HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen);

	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<UINV_InventoryBase> InventoryClass { nullptr };
	
	UPROPERTY()
	TObjectPtr<UINV_InventoryBase> Inventory { nullptr };
	
	bool bInventoryMenuOpen = false;
};
