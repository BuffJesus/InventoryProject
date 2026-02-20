// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_HUDWidget.generated.h"

class UINV_InfoMessage;
/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void OnNoRoom();
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|Inventory")
	// Show pickup prompt in the HUD.
	void ShowPickupMessage(const FString& Message);	
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|Inventory")
	// Hide pickup prompt in the HUD.
	void HidePickupMessage();
	
private:
	// Message widget used for info text.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InfoMessage> InfoMessage;
};
