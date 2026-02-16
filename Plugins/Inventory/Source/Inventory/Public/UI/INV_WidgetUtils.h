// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "INV_WidgetUtils.generated.h"

class UWidget;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_WidgetUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Get widget position in viewport space.
	UFUNCTION(BlueprintCallable, Category="INV|Widgets")
	static FVector2D GetWidgetPosition(UWidget* Widget);
	
	// Get widget size in local space.
	UFUNCTION(BlueprintCallable, Category="INV|Widgets")
	static FVector2D GetWidgetSize(UWidget* Widget);
	
	// Check if a point is inside a rectangle.
	UFUNCTION(BlueprintCallable, Category="INV|Widgets")
	static bool IsWithinBounds(const FVector2D& BoundaryPos, const FVector2D& WidgetSize, const FVector2D& MousePos);
	
	static FVector2D GetClampedWidgetPosition(const FVector2D& Boundary, const FVector2D& WidgetSize, const FVector2D& MousePos);
	
	// Convert grid position to linear index.
	static int32 GetIndexFromPosition(const FIntPoint& Position, const int32 Columns);
	// Convert linear index to grid position.
	static FIntPoint GetPositionFromIndex(const int32 Index, const int32 Columns);
};
