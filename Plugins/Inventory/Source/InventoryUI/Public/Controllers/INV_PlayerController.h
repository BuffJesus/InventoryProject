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
class INVENTORYUI_API AINV_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AINV_PlayerController();
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category="INV|Input")
	// Toggle the inventory UI.
	void ToggleInventory();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

private:
	// Evaluate view-change thresholds and execute trace when conditions are met.
	void AttemptTrace(float ElapsedSinceLastCheck);
	// Timer callback for interval-based tracing.
	void OnTraceTimerElapsed();
	// Interact with the item under the crosshair.
	void PrimaryInteract();
	// Create the HUD widget for this player.
	void CreateHUDWidget();
	// Trace forward to find highlightable items.
	void TraceForItem();
	
	// Cached inventory component.
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
	// How far to trace for items.
	double TraceLength { 500.f };

	UPROPERTY(EditDefaultsOnly, Category="INV|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.25"))
	// How often to run item trace in seconds (0 = every tick).
	float TraceIntervalSeconds { 0.05f };

	UPROPERTY(EditDefaultsOnly, Category="INV|Trace")
	// If enabled, only retrace when view position/rotation changed past thresholds.
	bool bOnlyTraceOnViewChange { true };

	UPROPERTY(EditDefaultsOnly, Category="INV|Trace", meta = (EditCondition = "bOnlyTraceOnViewChange", ClampMin = "0.0", UIMin = "0.0"))
	// Minimum camera location movement (cm) required to trigger a retrace.
	float MinViewLocationDeltaCm { 0.5f };

	UPROPERTY(EditDefaultsOnly, Category="INV|Trace", meta = (EditCondition = "bOnlyTraceOnViewChange", ClampMin = "0.0", UIMin = "0.0"))
	// Minimum camera rotation delta (deg) required to trigger a retrace.
	float MinViewRotationDeltaDeg { 0.1f };

	UPROPERTY(EditDefaultsOnly, Category="INV|Trace", meta = (EditCondition = "bOnlyTraceOnViewChange", ClampMin = "0.0", UIMin = "0.0"))
	// Force periodic retrace even without camera movement (0 disables).
	float MaxIdleRetraceSeconds { 0.25f };
	
	UPROPERTY(EditDefaultsOnly, Category="INV|Trace")
	// Collision channel used for item tracing.
	TEnumAsByte<ECollisionChannel> ItemTraceChannel { ECollisionChannel::ECC_GameTraceChannel1 };
	
	// Current and previous trace hits.
	TWeakObjectPtr<AActor> ThisActor { nullptr };
	TWeakObjectPtr<AActor> LastActor { nullptr };
	// Timer used for interval-based tracing.
	FTimerHandle TraceTimerHandle;
	// Time since last trace while view is unchanged.
	float IdleRetraceAccumulator { 0.f };
	// Last traced view state used for movement/rotation threshold checks.
	FVector LastTraceViewLocation { FVector::ZeroVector };
	FRotator LastTraceViewRotation { FRotator::ZeroRotator };
	bool bHasLastTraceView { false };
};
