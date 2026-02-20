// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/INV_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interaction/INV_Highlightable.h"
#include "Components/INV_InventoryComponent.h"
#include "Items/INV_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Widgets/HUD/INV_HUDWidget.h"

AINV_PlayerController::AINV_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AINV_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_PlayerController_Tick);

	// Interaction highlight/prompt is local-only UI behavior.
	if (!IsLocalController()) return;

	if (TraceIntervalSeconds <= 0.f)
	{
		TraceIntervalAccumulator = 0.f;
	}
	else
	{
		TraceIntervalAccumulator += DeltaTime;
		if (TraceIntervalAccumulator < TraceIntervalSeconds) return;
		TraceIntervalAccumulator = 0.f;
	}

	if (bOnlyTraceOnViewChange)
	{
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		GetPlayerViewPoint(ViewLocation, ViewRotation);

		bool bShouldTrace = !bHasLastTraceView;
		if (!bShouldTrace)
		{
			const float LocationDeltaSq = FVector::DistSquared(ViewLocation, LastTraceViewLocation);
			const float RequiredLocationDeltaSq = FMath::Square(MinViewLocationDeltaCm);
			const FRotator DeltaRotation = (ViewRotation - LastTraceViewRotation).GetNormalized();
			const float MaxRotationDelta = FMath::Max3(
				FMath::Abs(DeltaRotation.Pitch),
				FMath::Abs(DeltaRotation.Yaw),
				FMath::Abs(DeltaRotation.Roll));

			bShouldTrace = LocationDeltaSq >= RequiredLocationDeltaSq || MaxRotationDelta >= MinViewRotationDeltaDeg;

			if (!bShouldTrace && MaxIdleRetraceSeconds > 0.f)
			{
				IdleRetraceAccumulator += (TraceIntervalSeconds > 0.f) ? TraceIntervalSeconds : DeltaTime;
				bShouldTrace = IdleRetraceAccumulator >= MaxIdleRetraceSeconds;
			}
		}

		if (!bShouldTrace) return;
		LastTraceViewLocation = ViewLocation;
		LastTraceViewRotation = ViewRotation;
		bHasLastTraceView = true;
		IdleRetraceAccumulator = 0.f;
	}

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
	if (InventoryComponent.IsValid() && IsValid(HUDWidget))
	{
		InventoryComponent->OnNoRoomInInventory.AddUniqueDynamic(HUDWidget, &UINV_HUDWidget::OnNoRoom);
	}
	TraceIntervalAccumulator = TraceIntervalSeconds;
	IdleRetraceAccumulator = 0.f;
	bHasLastTraceView = false;
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
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_PlayerController_TraceForItem);

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
