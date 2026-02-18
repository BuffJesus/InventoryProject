// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "INV_DropLocationCalculator.generated.h"

class APawn;
class AActor;
class UWorld;

/**
 * Result of drop location calculation.
 */
USTRUCT(BlueprintType)
struct FINV_DropLocationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = false;
};

/**
 * Utility class for calculating safe drop locations for items.
 * Handles ground tracing, collision detection, and pawn clearance.
 */
UCLASS()
class INVENTORYCORE_API UINV_DropLocationCalculator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Calculate a safe drop location for an item near a pawn.
	 *
	 * @param World World context
	 * @param OwningPawn Pawn dropping the item
	 * @param OwnerActor Owner actor (usually inventory component owner)
	 * @param DropSpawnAngleMin Minimum spawn angle offset (degrees)
	 * @param DropSpawnAngleMax Maximum spawn angle offset (degrees)
	 * @param DropSpawnDistanceMin Minimum spawn distance from pawn
	 * @param DropSpawnDistanceMax Maximum spawn distance from pawn
	 * @param DropValidationRadius Capsule radius for collision validation
	 * @param DropValidationHalfHeight Capsule half-height for collision validation
	 * @param DropPlayerClearance Extra clearance distance from player
	 * @param RelativeSpawnElevation Elevation offset (negative = lower)
	 * @param DropGroundTraceStartHeight Height above spawn point to start ground trace
	 * @param DropGroundTraceDepth Depth below spawn point for ground trace
	 * @param DropSurfaceOffset Offset from surface after ground trace
	 * @return Drop location result with position, rotation, and validity
	 */
	static FINV_DropLocationResult CalculateDropLocation(
		UWorld* World,
		const APawn* OwningPawn,
		const AActor* OwnerActor,
		float DropSpawnAngleMin,
		float DropSpawnAngleMax,
		float DropSpawnDistanceMin,
		float DropSpawnDistanceMax,
		float DropValidationRadius,
		float DropValidationHalfHeight,
		float DropPlayerClearance,
		float RelativeSpawnElevation,
		float DropGroundTraceStartHeight,
		float DropGroundTraceDepth,
		float DropSurfaceOffset);

private:
	/**
	 * Check if a location is clear of collision for dropping an item.
	 */
	static bool IsDropLocationClear(
		const UWorld* World,
		const FVector& Location,
		const APawn* OwningPawn,
		const AActor* OwnerActor,
		float Radius,
		float HalfHeight);

	/**
	 * Enforce minimum clearance from pawn capsule.
	 */
	static FVector EnforcePawnClearance(
		const APawn* OwningPawn,
		const FVector& Location,
		const FVector& PreferredDirection,
		float ItemRadius,
		float ExtraClearance);
};
