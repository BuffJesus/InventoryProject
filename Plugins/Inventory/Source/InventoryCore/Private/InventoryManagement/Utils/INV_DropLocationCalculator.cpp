// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/Utils/INV_DropLocationCalculator.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

FINV_DropLocationResult UINV_DropLocationCalculator::CalculateDropLocation(
	UWorld* World,
	const APawn* OwningPawn,
	const AActor* OwnerActor,
	const float DropSpawnAngleMin,
	const float DropSpawnAngleMax,
	const float DropSpawnDistanceMin,
	const float DropSpawnDistanceMax,
	const float DropValidationRadius,
	const float DropValidationHalfHeight,
	const float DropPlayerClearance,
	const float RelativeSpawnElevation,
	const float DropGroundTraceStartHeight,
	const float DropGroundTraceDepth,
	const float DropSurfaceOffset)
{
	FINV_DropLocationResult Result;
	Result.Rotation = FRotator::ZeroRotator;

	if (!IsValid(World) || !IsValid(OwningPawn))
	{
		return Result;
	}

	// Calculate random forward direction with angle variation
	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(
		FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax),
		FVector::UpVector);

	FVector Forward2D = RotatedForward;
	Forward2D.Z = 0.0f;
	Forward2D = Forward2D.GetSafeNormal();

	if (Forward2D.IsNearlyZero())
	{
		Forward2D = OwningPawn->GetActorForwardVector().GetSafeNormal2D();
		if (Forward2D.IsNearlyZero())
		{
			Forward2D = FVector::ForwardVector;
		}
	}

	// Calculate drop distance accounting for pawn size and clearance
	float RequiredDropDistance = DropValidationRadius + DropPlayerClearance;
	if (const UCapsuleComponent* PawnCapsule = OwningPawn->FindComponentByClass<UCapsuleComponent>())
	{
		RequiredDropDistance += PawnCapsule->GetScaledCapsuleRadius();
	}

	const float RequestedDropDistance = FMath::FRandRange(DropSpawnDistanceMin, DropSpawnDistanceMax);
	const float DropDistance = FMath::Max(RequestedDropDistance, RequiredDropDistance);

	// Calculate base spawn location
	const FVector BaseSpawnLocation = OwningPawn->GetActorLocation() + Forward2D * DropDistance;
	FVector SpawnLocation = BaseSpawnLocation;
	SpawnLocation.Z -= RelativeSpawnElevation;

	// Setup collision query params
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(INV_DropGroundTrace), false);
	QueryParams.AddIgnoredActor(OwningPawn);
	if (IsValid(OwnerActor))
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	// Snap to ground surface to avoid embedding
	const FVector TraceStart = SpawnLocation + FVector::UpVector * DropGroundTraceStartHeight;
	const FVector TraceEnd = SpawnLocation - FVector::UpVector * DropGroundTraceDepth;
	FHitResult GroundHit;

	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams)
		|| World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldDynamic, QueryParams))
	{
		SpawnLocation = GroundHit.ImpactPoint + GroundHit.ImpactNormal * DropSurfaceOffset;
	}

	// Enforce pawn clearance
	SpawnLocation = EnforcePawnClearance(OwningPawn, SpawnLocation, Forward2D, DropValidationRadius, DropPlayerClearance);

	// Validate item volume; if blocked try to nudge out once along hit normal
	const FCollisionShape DropShape = FCollisionShape::MakeCapsule(DropValidationRadius, DropValidationHalfHeight);
	FHitResult SweepHit;

	if (World->SweepSingleByChannel(SweepHit, SpawnLocation, SpawnLocation, FQuat::Identity, ECC_WorldStatic, DropShape, QueryParams)
		|| World->SweepSingleByChannel(SweepHit, SpawnLocation, SpawnLocation, FQuat::Identity, ECC_WorldDynamic, DropShape, QueryParams))
	{
		const FVector PushNormal = SweepHit.Normal.IsNearlyZero() ? FVector::UpVector : SweepHit.Normal;
		const FVector AdjustedLocation = SpawnLocation + PushNormal * (DropValidationRadius + DropSurfaceOffset);

		if (IsDropLocationClear(World, AdjustedLocation, OwningPawn, OwnerActor, DropValidationRadius, DropValidationHalfHeight))
		{
			SpawnLocation = AdjustedLocation;
		}
	}

	// Final pawn clearance enforcement
	SpawnLocation = EnforcePawnClearance(OwningPawn, SpawnLocation, Forward2D, DropValidationRadius, DropPlayerClearance);

	Result.Location = SpawnLocation;
	Result.bIsValid = true;

	return Result;
}

bool UINV_DropLocationCalculator::IsDropLocationClear(
	const UWorld* World,
	const FVector& Location,
	const APawn* OwningPawn,
	const AActor* OwnerActor,
	const float Radius,
	const float HalfHeight)
{
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(INV_DropLocationClear), false);
	if (IsValid(OwningPawn))
	{
		QueryParams.AddIgnoredActor(OwningPawn);
	}
	if (IsValid(OwnerActor))
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	return !World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, CollisionShape, QueryParams)
		&& !World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_WorldDynamic, CollisionShape, QueryParams);
}

FVector UINV_DropLocationCalculator::EnforcePawnClearance(
	const APawn* OwningPawn,
	const FVector& Location,
	const FVector& PreferredDirection,
	const float ItemRadius,
	const float ExtraClearance)
{
	if (!IsValid(OwningPawn))
	{
		return Location;
	}

	const UCapsuleComponent* PawnCapsule = OwningPawn->FindComponentByClass<UCapsuleComponent>();
	if (!IsValid(PawnCapsule))
	{
		return Location;
	}

	const float MinHorizontalDistance = PawnCapsule->GetScaledCapsuleRadius() + ItemRadius + ExtraClearance;
	const FVector PawnLocation = OwningPawn->GetActorLocation();
	FVector Delta2D = Location - PawnLocation;
	Delta2D.Z = 0.0f;

	const float CurrentDistance = Delta2D.Size();
	if (CurrentDistance >= MinHorizontalDistance)
	{
		return Location;
	}

	FVector PushDirection = Delta2D.GetSafeNormal();
	if (PushDirection.IsNearlyZero())
	{
		FVector FallbackDirection = PreferredDirection;
		FallbackDirection.Z = 0.0f;
		PushDirection = FallbackDirection.GetSafeNormal();

		if (PushDirection.IsNearlyZero())
		{
			PushDirection = FVector::ForwardVector;
		}
	}

	const FVector New2DLocation = PawnLocation + PushDirection * MinHorizontalDistance;
	return FVector(New2DLocation.X, New2DLocation.Y, Location.Z);
}
