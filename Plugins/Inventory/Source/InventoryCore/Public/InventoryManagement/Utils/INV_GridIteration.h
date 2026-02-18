#pragma once

#include "CoreMinimal.h"

class FINV_GridIteration
{
public:
	static FORCEINLINE int32 GetIndexFromPosition(const FIntPoint& Position, int32 Columns)
	{
		return Position.X + (Position.Y * Columns);
	}

	static FORCEINLINE FIntPoint GetPositionFromIndex(int32 Index, int32 Columns)
	{
		return FIntPoint(Index % Columns, Index / Columns);
	}

	template<typename TArrayType, typename FuncT>
	static void ForEach2D(TArrayType& Array, int32 Index, const FIntPoint& Range2D, int32 GridColumns, FuncT&& Function)
	{
		for (int32 j = 0; j < Range2D.Y; ++j)
		{
			for (int32 i = 0; i < Range2D.X; ++i)
			{
				const FIntPoint Coordinates = GetPositionFromIndex(Index, GridColumns) + FIntPoint(i, j);
				const int32 TileIndex = GetIndexFromPosition(Coordinates, GridColumns);
				if (Array.IsValidIndex(TileIndex))
				{
					Function(Array[TileIndex]);
				}
			}
		}
	}
};
