// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/INV_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputKeyEventArgs.h"
#include "Containers/INV_ContainerComponent.h"
#include "Interaction/INV_Highlightable.h"
#include "Interaction/INV_ContainerProvider.h"
#include "Components/INV_InventoryComponent.h"
#include "Items/INV_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"
#include "UI/Widgets/HUD/INV_HUDWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

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
	UpdateVirtualCursor(DeltaTime);

	// Interval-based tracing is handled by timer callback.
	if (TraceIntervalSeconds > 0.f) return;

	AttemptTrace(DeltaTime);
}

void AINV_PlayerController::AttemptTrace(const float ElapsedSinceLastCheck)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_PlayerController_AttemptTrace);
	if (InventoryComponent.IsValid() && InventoryComponent->IsInventoryMenuOpen()) return;

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
				IdleRetraceAccumulator += ElapsedSinceLastCheck;
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
	if (InventoryComponent->IsInventoryMenuOpen())
	{
		ClearCurrentTraceTarget();
	}
}

void AINV_PlayerController::SetHUDWidgetVisible(const bool bVisible)
{
	if (!IsValid(HUDWidget)) return;
	HUDWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void AINV_PlayerController::ClearCurrentTraceTarget()
{
	if (ThisActor.IsValid())
	{
		if (UActorComponent* HighlightableComponent { ThisActor->FindComponentByInterface(UINV_Highlightable::StaticClass()) }; IsValid(HighlightableComponent))
		{
			IINV_Highlightable::Execute_UnHighlight(HighlightableComponent);
		}
	}

	ThisActor = nullptr;
	LastActor = nullptr;
	IdleRetraceAccumulator = 0.f;
	bHasLastTraceView = false;

	if (IsValid(HUDWidget))
	{
		HUDWidget->HidePickupMessage();
	}
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
	IdleRetraceAccumulator = 0.f;
	bHasLastTraceView = false;

	if (!IsLocalController()) return;

	if (TraceIntervalSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			TraceTimerHandle,
			this,
			&ThisClass::OnTraceTimerElapsed,
			TraceIntervalSeconds,
			true);
		OnTraceTimerElapsed();
	}
}

void AINV_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TraceTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TraceTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AINV_PlayerController::OnTraceTimerElapsed()
{
	AttemptTrace(TraceIntervalSeconds > 0.f ? TraceIntervalSeconds : 0.f);
}

void AINV_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Bind input actions.
	UEnhancedInputComponent* EnhancedInputComponent { CastChecked<UEnhancedInputComponent>(InputComponent) };
	
	EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &ThisClass::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
}

bool AINV_PlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	const bool bIsGamepadInput = Params.Key.IsGamepadKey();
	const bool bIsKeyboardInput =
		Params.Key.IsDigital() &&
		!Params.Key.IsGamepadKey() &&
		!Params.Key.IsMouseButton() &&
		!Params.Key.IsTouch();
	const bool bIsMouseOrKeyboardInput = Params.Key.IsMouseButton() || bIsKeyboardInput;
	const bool bPreviousInputWasGamepad = bLastInputWasGamepad;

	if (bIsGamepadInput || bIsMouseOrKeyboardInput)
	{
		bLastInputWasGamepad = bIsGamepadInput;
	}

	const bool bInputMethodChanged = bPreviousInputWasGamepad != bLastInputWasGamepad;
	if (bInputMethodChanged && ThisActor.IsValid() && IsValid(HUDWidget))
	{
		if (UINV_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UINV_ItemComponent>(); IsValid(ItemComponent))
		{
			HUDWidget->ShowPickupMessage(BuildPickupPromptForCurrentInput(ItemComponent->GetPickupMessage()));
		}
	}
	if (bInputMethodChanged && InventoryComponent.IsValid())
	{
		InventoryComponent->OnInputMethodChanged(bLastInputWasGamepad);
	}

	// While inventory is open, map gamepad face buttons to mouse clicks.
	if (InventoryComponent.IsValid() && InventoryComponent->IsInventoryMenuOpen())
	{
		if (Params.Key == EKeys::Gamepad_FaceButton_Bottom || Params.Key == EKeys::Gamepad_FaceButton_Left)
		{
			const FKey MappedMouseKey = Params.Key == EKeys::Gamepad_FaceButton_Bottom
				? EKeys::LeftMouseButton
				: EKeys::RightMouseButton;
			if (SimulateMouseButtonFromGamepad(MappedMouseKey, Params.Event))
			{
				if (Params.Event == IE_Pressed || Params.Event == IE_Released || Params.Event == IE_DoubleClick)
				{
					const FInputKeyEventArgs SimulatedMouseArgs = FInputKeyEventArgs::CreateSimulated(
						MappedMouseKey,
						Params.Event,
						Params.AmountDepressed,
						Params.NumSamples,
						Params.InputDevice,
						false,
						Params.Viewport);
					Super::InputKey(SimulatedMouseArgs);
				}
				return true;
			}

			return true;
		}
	}

	return Super::InputKey(Params);
}

void AINV_PlayerController::UpdateVirtualCursor(const float DeltaTime)
{
	if (!InventoryComponent.IsValid()) return;
	if (!InventoryComponent->IsInventoryMenuOpen()) return;
	if (!bLastInputWasGamepad) return;
	if (VirtualCursorSpeed <= 0.f) return;

	const float LeftX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float LeftY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	const FVector2D StickInput(LeftX, LeftY);
	const float InputMagnitude = StickInput.Size();
	if (InputMagnitude < VirtualCursorDeadzone)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return;
	}

	if (!GetMousePosition(MouseX, MouseY))
	{
		MouseX = static_cast<float>(ViewportWidth) * 0.5f;
		MouseY = static_cast<float>(ViewportHeight) * 0.5f;
	}

	// Apply deadzone remap and curved response for better fine control at low stick tilt.
	const float NormalizedMagnitude = FMath::Clamp(
		(InputMagnitude - VirtualCursorDeadzone) / FMath::Max(0.001f, 1.0f - VirtualCursorDeadzone),
		0.0f,
		1.0f);
	const float ResponseScale = NormalizedMagnitude * NormalizedMagnitude;
	const FVector2D Direction = StickInput.GetSafeNormal();

	const float DeltaX = Direction.X * ResponseScale * VirtualCursorSpeed * DeltaTime;
	const float DeltaY = -Direction.Y * ResponseScale * VirtualCursorSpeed * DeltaTime;
	const int32 NextX = FMath::Clamp(FMath::RoundToInt(MouseX + DeltaX), 0, ViewportWidth - 1);
	const int32 NextY = FMath::Clamp(FMath::RoundToInt(MouseY + DeltaY), 0, ViewportHeight - 1);
	SetMouseLocation(NextX, NextY);
}

bool AINV_PlayerController::SimulateMouseButtonFromGamepad(const FKey& MouseButton, const EInputEvent InputEvent)
{
	FSlateApplication& SlateApp = FSlateApplication::Get();
	const FVector2D CursorScreenPos = SlateApp.GetCursorPos();
	const uint32 PointerIndex = 0u;
	const uint32 UserIndex = 0u;

	auto MakePointerEvent = [this, &CursorScreenPos, PointerIndex, UserIndex](const FKey& EffectingButton)
	{
		return FPointerEvent(
			UserIndex,
			PointerIndex,
			CursorScreenPos,
			CursorScreenPos,
			SimulatedMouseButtonsDown,
			EffectingButton,
			0.0f,
			FModifierKeysState());
	};

	switch (InputEvent)
	{
	case IE_Pressed:
	case IE_DoubleClick:
	{
		if (!SimulatedMouseButtonsDown.Contains(MouseButton))
		{
			SimulatedMouseButtonsDown.Add(MouseButton);
		}

		const FPointerEvent PointerDownEvent = MakePointerEvent(MouseButton);
		const TSharedPtr<SWindow> ActiveWindow = SlateApp.GetActiveTopLevelWindow();
		const TSharedPtr<FGenericWindow> NativeWindow = ActiveWindow.IsValid() ? ActiveWindow->GetNativeWindow() : nullptr;
		SlateApp.ProcessMouseButtonDownEvent(NativeWindow, PointerDownEvent);
		return true;
	}

	case IE_Released:
	{
		SimulatedMouseButtonsDown.Remove(MouseButton);
		const FPointerEvent PointerUpEvent = MakePointerEvent(MouseButton);
		SlateApp.ProcessMouseButtonUpEvent(PointerUpEvent);
		return true;
	}

	case IE_Repeat:
		return true;

	default:
		return false;
	}
}

void AINV_PlayerController::PrimaryInteract()
{
	// Try to pick up the item we are looking at.
	if (!ThisActor.IsValid()) return;
	UINV_ItemComponent* ItemComponent { ThisActor->FindComponentByClass<UINV_ItemComponent>() };
	if (!InventoryComponent.IsValid()) return;

	if (IsValid(ItemComponent))
	{
		InventoryComponent->TryAddItem(ItemComponent);
		return;
	}

	if (UINV_ContainerComponent* ContainerComponent = ResolveContainerForActor(ThisActor.Get()); IsValid(ContainerComponent))
	{
		InventoryComponent->RequestOpenContainer(ThisActor.Get());
	}
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
	if (InventoryComponent.IsValid() && InventoryComponent->IsInventoryMenuOpen()) return;

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
		if (IsValid(ItemComponent))
		{
			if (IsValid(HUDWidget))
			{
				HUDWidget->ShowPickupMessage(BuildPickupPromptForCurrentInput(ItemComponent->GetPickupMessage()));
			}
		}
		else if (UINV_ContainerComponent* ContainerComponent = ResolveContainerForActor(ThisActor.Get()); IsValid(ContainerComponent))
		{
			if (IsValid(HUDWidget))
			{
				const FString DisplayName = ContainerComponent->GetDisplayName().ToString();
				HUDWidget->ShowPickupMessage(BuildPickupPromptForCurrentInput(FString::Printf(TEXT("Open %s"), *DisplayName)));
			}
		}
	}
	
	if (LastActor.IsValid())
	{
		if (UActorComponent* HighlightableComponent { LastActor->FindComponentByInterface(UINV_Highlightable::StaticClass()) }; IsValid(HighlightableComponent))
		{
			IINV_Highlightable::Execute_UnHighlight(HighlightableComponent);
		}
	}
}

FString AINV_PlayerController::BuildPickupPromptForCurrentInput(const FString& RawPickupMessage) const
{
	const FString InteractKeyLabel = bLastInputWasGamepad
		? GamepadInteractKeyLabel.ToString()
		: KeyboardInteractKeyLabel.ToString();

	FString Prompt = RawPickupMessage;
	const FString AlternateLabel = bLastInputWasGamepad
		? KeyboardInteractKeyLabel.ToString()
		: GamepadInteractKeyLabel.ToString();

	// Preferred template token support in pickup message text.
	if (Prompt.ReplaceInline(TEXT("{InteractKey}"), *InteractKeyLabel, ESearchCase::IgnoreCase) > 0)
	{
		return Prompt;
	}

	// Common legacy phrase support.
	if (Prompt.ReplaceInline(TEXT("Press E"), *FString::Printf(TEXT("Press %s"), *InteractKeyLabel), ESearchCase::IgnoreCase) > 0)
	{
		return Prompt;
	}
	if (Prompt.ReplaceInline(TEXT("Press X"), *FString::Printf(TEXT("Press %s"), *InteractKeyLabel), ESearchCase::IgnoreCase) > 0)
	{
		return Prompt;
	}

	// Generic token swap when the message contains only the alternate key label.
	if (!AlternateLabel.IsEmpty() && !InteractKeyLabel.IsEmpty() &&
		Prompt.Contains(AlternateLabel, ESearchCase::IgnoreCase) &&
		!Prompt.Contains(InteractKeyLabel, ESearchCase::IgnoreCase))
	{
		Prompt.ReplaceInline(*AlternateLabel, *InteractKeyLabel, ESearchCase::IgnoreCase);
		return Prompt;
	}

	// If explicit key text already exists, keep message as authored to avoid duplicate "Press X Press E" prompts.
	if (Prompt.Contains(TEXT("Press"), ESearchCase::IgnoreCase))
	{
		return Prompt;
	}

	// Fallback when the message has no explicit key text.
	return FString::Printf(TEXT("Press %s %s"), *InteractKeyLabel, *RawPickupMessage);
}

UINV_ContainerComponent* AINV_PlayerController::ResolveContainerForActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (Actor->GetClass()->ImplementsInterface(UINV_ContainerProvider::StaticClass()))
	{
		return IINV_ContainerProvider::Execute_GetContainerComponent(Actor);
	}

	return Actor->FindComponentByClass<UINV_ContainerComponent>();
}
