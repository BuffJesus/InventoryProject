// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "INV_InfoMessage.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_InfoMessage : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|HUD")
	// Blueprint animation hook for showing message.
	void MessageShow();
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|HUD")
	// Blueprint animation hook for hiding message.
	void MessageHide();
	
	// Set message text and start the timer.
	void SetMessage(const FText& Message = FText::FromString("Message not set"));
	
private:
	// Text widget bound in UMG.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Message;
	// How long the message stays on screen.
	UPROPERTY(EditAnywhere, Category="INV|HUD") float MessageLifetime { 3.f };
	// Timer handle for hiding the message.
	FTimerHandle MessageTimerHandle;
	// Tracks message visibility.
	bool bIsMessageActive { false };
};
