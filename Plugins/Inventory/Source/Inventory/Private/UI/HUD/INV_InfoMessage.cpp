// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/INV_InfoMessage.h"
#include "Components/TextBlock.h"

void UINV_InfoMessage::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Text_Message->SetText(FText::GetEmpty());
	MessageHide();
}

void UINV_InfoMessage::SetMessage(const FText& Message)
{
	Text_Message->SetText(Message);
	
	if (!bIsMessageActive) { MessageShow(); }
	bIsMessageActive = true;
	
	GetWorld()->GetTimerManager().SetTimer(MessageTimerHandle, [this]()
	{
		MessageHide();
		bIsMessageActive = false;
	}, MessageLifetime, false);
}
