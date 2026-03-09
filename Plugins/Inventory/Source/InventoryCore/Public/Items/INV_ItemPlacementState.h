// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"
#include "INV_ItemPlacementState.generated.h"

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_ItemPlacementState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Placement")
	EINV_ItemCategory ContainerCategory { EINV_ItemCategory::None };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Placement")
	int32 AnchorIndex { INDEX_NONE };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "INV|Placement")
	uint8 Rotation { 0 };

	bool IsValid() const
	{
		return ContainerCategory != EINV_ItemCategory::None && AnchorIndex != INDEX_NONE;
	}

	void Reset()
	{
		ContainerCategory = EINV_ItemCategory::None;
		AnchorIndex = INDEX_NONE;
		Rotation = 0;
	}
};
