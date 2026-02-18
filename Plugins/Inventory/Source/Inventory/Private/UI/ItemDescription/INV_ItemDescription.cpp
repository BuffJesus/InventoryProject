// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ItemDescription/INV_ItemDescription.h"
#include "Components/SizeBox.h"

FVector2D UINV_ItemDescription::GetBoxSize() const
{
	return IsValid(SizeBox) ? SizeBox->GetDesiredSize() : FVector2D::ZeroVector;
}
