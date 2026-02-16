// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Composite/INV_Composite_Base.h"

void UINV_Composite_Base::Collapse()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UINV_Composite_Base::Expand()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UINV_Composite_Base::ApplyFunction(FuncType Function)
{
	(void)Function;
}
