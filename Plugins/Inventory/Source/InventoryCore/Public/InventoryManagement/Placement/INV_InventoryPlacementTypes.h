#pragma once

#include "CoreMinimal.h"
#include "INV_InventoryPlacementTypes.generated.h"

USTRUCT(BlueprintType)
struct INVENTORYCORE_API FINV_InventoryItemPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "INV|Placement")
	FIntPoint Anchor { INDEX_NONE, INDEX_NONE };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "INV|Placement")
	uint8 RotationQuarterTurns { 0 };

	bool IsValid() const
	{
		return Anchor.X >= 0 && Anchor.Y >= 0;
	}

	void Reset()
	{
		Anchor = FIntPoint(INDEX_NONE, INDEX_NONE);
		RotationQuarterTurns = 0;
	}

	FIntPoint ResolveFootprint(const FIntPoint& BaseFootprint) const
	{
		const bool bSwapAxes = (RotationQuarterTurns % 2) != 0;
		return bSwapAxes ? FIntPoint(BaseFootprint.Y, BaseFootprint.X) : BaseFootprint;
	}

	int32 ToIndex(const int32 GridWidth) const
	{
		return IsValid() && GridWidth > 0 ? (Anchor.Y * GridWidth) + Anchor.X : INDEX_NONE;
	}

	static FINV_InventoryItemPlacement FromIndex(const int32 Index, const int32 GridWidth, const uint8 InRotationQuarterTurns = 0)
	{
		FINV_InventoryItemPlacement Placement;
		if (Index < 0 || GridWidth <= 0)
		{
			return Placement;
		}

		Placement.Anchor = FIntPoint(Index % GridWidth, Index / GridWidth);
		Placement.RotationQuarterTurns = InRotationQuarterTurns;
		return Placement;
	}
};
