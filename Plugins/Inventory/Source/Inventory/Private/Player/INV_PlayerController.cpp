// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/INV_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory.h"
#include "Interaction/INV_Highlightable.h"
#include "Items/INV_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/INV_HUDWidget.h"

AINV_PlayerController::AINV_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AINV_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TraceForItem();
}

void AINV_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem { ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) };
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		Subsystem->AddMappingContext(CurrentContext, 0);
	}
	
	CreateHUDWidget();
}

void AINV_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent { CastChecked<UEnhancedInputComponent>(InputComponent) };
	
	EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &ThisClass::PrimaryInteract);
}

void AINV_PlayerController::PrimaryInteract()
{
	UE_LOGFMT(LogInventory, Log, "Primary interact triggered");
}

void AINV_PlayerController::CreateHUDWidget()
{
	if (!IsLocalController()) return;
	
	if (IsValid(HUDWidgetClass))
	{
		HUDWidget = CreateWidget<UINV_HUDWidget>(GetWorld(), HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
}

void AINV_PlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) return;
	
	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);
	
	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	
	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
	}
	
	if (ThisActor == LastActor) return;
	
	if (ThisActor.IsValid())
	{
		if (UActorComponent* HighlightableComponent { ThisActor->FindComponentByInterface(UINV_Highlightable::StaticClass()) }; IsValid(HighlightableComponent))
		{
			IINV_Highlightable::Execute_Highlight(HighlightableComponent);
		}
			
		UINV_ItemComponent* ItemComponent { ThisActor->FindComponentByClass<UINV_ItemComponent>() };
		if (!IsValid(ItemComponent)) return;
		
		if (IsValid(HUDWidget)) HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}
	
	if (LastActor.IsValid())
	{
		if (UActorComponent* HighlightableComponent { LastActor->FindComponentByInterface(UINV_Highlightable::StaticClass()) }; IsValid(HighlightableComponent))
		{
			IINV_Highlightable::Execute_UnHighlight(HighlightableComponent);
		}
	}
}
