// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "INV_PlayerController.generated.h"

class UINV_InventoryComponent;
class UINV_HUDWidget;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class INVENTORY_API AINV_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AINV_PlayerController();
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category="INV|Input")
	void ToggleInventory();
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void PrimaryInteract();
	void CreateHUDWidget();
	void TraceForItem();
	
	TWeakObjectPtr<UINV_InventoryComponent> InventoryComponent { nullptr };
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultIMCs;
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Input")
	TObjectPtr<UInputAction> PrimaryInteractAction;
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;
	
	UPROPERTY(EditDefaultsOnly, Category="INV|HUD")
	TSubclassOf<UINV_HUDWidget> HUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UINV_HUDWidget> HUDWidget;
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Trace")
	double TraceLength { 500.f };
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Trace")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel { ECollisionChannel::ECC_GameTraceChannel1 };
	
	TWeakObjectPtr<AActor> ThisActor { nullptr };
	TWeakObjectPtr<AActor> LastActor { nullptr };
};
