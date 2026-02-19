// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

/**
 * Handles coordinate transformations and tile calculations for grid inventory.
 * Provides pure mathematical functions for converting between screen space and grid space.
 */
class INVENTORYUI_API FINV_GridCoordinateCalculator
{
public:
	/**
	 * Calculate grid coordinates from mouse position.
	 *
	 * @param CanvasPos Position of the canvas on screen
	 * @param MousePos Mouse position on screen
	 * @param TileSize Size of each grid tile
	 * @return Grid coordinates (column, row)
	 */
	static FIntPoint CalculateHoverCoordinates(
		const FVector2D& CanvasPos,
		const FVector2D& MousePos,
		float TileSize);

	/**
	 * Calculate which quadrant of a tile the mouse is in.
	 *
	 * @param CanvasPos Position of the canvas on screen
	 * @param MousePos Mouse position on screen
	 * @param TileSize Size of each grid tile
	 * @return Tile quadrant (TopLeft, TopRight, BottomLeft, BottomRight, or None)
	 */
	static EINV_TileQuadrant CalculateTileQuadrant(
		const FVector2D& CanvasPos,
		const FVector2D& MousePos,
		float TileSize);

	/**
	 * Check if mouse cursor has exited the canvas bounds.
	 *
	 * @param BoundaryPos Canvas position on screen
	 * @param BoundarySize Canvas size
	 * @param MousePos Mouse position on screen
	 * @return True if mouse is outside canvas bounds
	 */
	static bool IsOutsideBounds(
		const FVector2D& BoundaryPos,
		const FVector2D& BoundarySize,
		const FVector2D& MousePos);
};
