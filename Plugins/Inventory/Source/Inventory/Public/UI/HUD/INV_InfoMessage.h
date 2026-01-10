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
	void MessageShow();
	
	UFUNCTION(BlueprintImplementableEvent, Category="INV|HUD")
	void MessageHide();
	
	void SetMessage(const FText& Message = FText::FromString("Message not set"));
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_Message;
	UPROPERTY(EditAnywhere, Category="INV|HUD") float MessageLifetime { 3.f };
	FTimerHandle MessageTimerHandle;
	bool bIsMessageActive { false };
};
