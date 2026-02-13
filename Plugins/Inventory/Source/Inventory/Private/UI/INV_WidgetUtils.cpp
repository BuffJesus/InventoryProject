// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/INV_WidgetUtils.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Widget.h"

FVector2D UINV_WidgetUtils::GetWidgetPosition(UWidget* Widget)
{
	if (!IsValid(Widget)) return FVector2D::ZeroVector;
	// Convert local widget space to viewport space.
	const FGeometry Geometry = Widget->GetCachedGeometry();
	FVector2D PixelPos;
	FVector2D ViewportPos;
	USlateBlueprintLibrary::LocalToViewport(Widget, Geometry, USlateBlueprintLibrary::GetLocalTopLeft(Geometry), PixelPos, ViewportPos);
	return ViewportPos;
}

FVector2D UINV_WidgetUtils::GetWidgetSize(UWidget* Widget)
{
	if (!IsValid(Widget)) return FVector2D::ZeroVector;
	// Return the widget's local size.
	const FGeometry Geometry = Widget->GetCachedGeometry();
	return Geometry.GetLocalSize();
}

bool UINV_WidgetUtils::IsWithinBounds(const FVector2D& BoundaryPos, const FVector2D& WidgetSize,
                                      const FVector2D& MousePos)
{
	// Simple rect bounds check.
	return MousePos.X >= BoundaryPos.X && MousePos.X <= (BoundaryPos.X + WidgetSize.X) &&
		MousePos.Y >= BoundaryPos.Y && MousePos.Y <= (BoundaryPos.Y + WidgetSize.Y);
}

int32 UINV_WidgetUtils::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns)
{
	// Flatten 2D coords into a linear index.
	return Position.X + (Position.Y * Columns);
}

FIntPoint UINV_WidgetUtils::GetPositionFromIndex(const int32 Index, const int32 Columns)
{
	// Expand a linear index into 2D coords.
	return FIntPoint(Index % Columns, Index / Columns);
}
