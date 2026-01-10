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
class INVENTORY_API UINV_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|Inventory")
	void ShowPickupMessage(const FString& Message);	
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|Inventory")
	void HidePickupMessage();
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UINV_InfoMessage> InfoMessage;
	UFUNCTION() void OnNoRoom();
};
