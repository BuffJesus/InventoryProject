// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/INV_WidgetUtils.h"

int32 UINV_WidgetUtils::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns)
{
	return Position.X + (Position.Y * Columns);
}
