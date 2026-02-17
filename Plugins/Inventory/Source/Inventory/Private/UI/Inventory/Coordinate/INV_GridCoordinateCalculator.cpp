// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Coordinate/INV_GridCoordinateCalculator.h"
#include "UI/INV_WidgetUtils.h"

FIntPoint UINV_GridCoordinateCalculator::CalculateHoverCoordinates(
	const FVector2D& CanvasPos,
	const FVector2D& MousePos,
	float TileSize)
{
	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePos.X - CanvasPos.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePos.Y - CanvasPos.Y) / TileSize))
	};
}

EINV_TileQuadrant UINV_GridCoordinateCalculator::CalculateTileQuadrant(
	const FVector2D& CanvasPos,
	const FVector2D& MousePos,
	float TileSize)
{
	// Calculate relative position within current tile
	const float TileLocalX = FMath::Fmod(MousePos.X - CanvasPos.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePos.Y - CanvasPos.Y, TileSize);

	// Determine quadrant mouse is in
	const bool bIsTop = TileLocalY < TileSize / 2.f;
	const bool bIsLeft = TileLocalX < TileSize / 2.f;

	EINV_TileQuadrant HoveredTileQuadrant = EINV_TileQuadrant::None;
	if (bIsTop && bIsLeft)
		HoveredTileQuadrant = EINV_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft)
		HoveredTileQuadrant = EINV_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft)
		HoveredTileQuadrant = EINV_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft)
		HoveredTileQuadrant = EINV_TileQuadrant::BottomRight;

	return HoveredTileQuadrant;
}

bool UINV_GridCoordinateCalculator::IsOutsideBounds(
	const FVector2D& BoundaryPos,
	const FVector2D& BoundarySize,
	const FVector2D& MousePos)
{
	return !UINV_WidgetUtils::IsWithinBounds(BoundaryPos, BoundarySize, MousePos);
}
