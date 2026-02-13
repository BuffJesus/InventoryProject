// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/INV_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory.h"
#include "Interaction/INV_Highlightable.h"
#include "InventoryManagement/Components/INV_InventoryComponent.h"
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
	
	// Continuously trace for items in front of the camera.
	TraceForItem();
}

void AINV_PlayerController::ToggleInventory()
{
	if (!InventoryComponent.IsValid()) return;
	InventoryComponent->ToggleInventoryMenu();
}

void AINV_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Apply input mapping contexts.
	UEnhancedInputLocalPlayerSubsystem* Subsystem { ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) };
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		Subsystem->AddMappingContext(CurrentContext, 0);
	}
	
	InventoryComponent = FindComponentByClass<UINV_InventoryComponent>();
	CreateHUDWidget();
}

void AINV_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Bind input actions.
	UEnhancedInputComponent* EnhancedInputComponent { CastChecked<UEnhancedInputComponent>(InputComponent) };
	
	EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &ThisClass::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
}

void AINV_PlayerController::PrimaryInteract()
{
	// Try to pick up the item we are looking at.
	if (!ThisActor.IsValid()) return;
	UINV_ItemComponent* ItemComponent { ThisActor->FindComponentByClass<UINV_ItemComponent>() };
	if (!IsValid(ItemComponent) || !InventoryComponent.IsValid()) return;
	
	InventoryComponent->TryAddItem(ItemComponent);
}

void AINV_PlayerController::CreateHUDWidget()
{
	if (!IsLocalController()) return;
	
	// Create and show the HUD widget.
	if (IsValid(HUDWidgetClass))
	{
		HUDWidget = CreateWidget<UINV_HUDWidget>(GetWorld(), HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
}

void AINV_PlayerController::TraceForItem()
{
	// Trace from the center of the viewport.
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) return;
	
	// Line trace forward.
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
