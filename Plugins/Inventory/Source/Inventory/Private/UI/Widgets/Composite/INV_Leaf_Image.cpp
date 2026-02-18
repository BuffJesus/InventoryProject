// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Composite/INV_Leaf_Image.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

void UINV_Leaf_Image::SetImage(UTexture2D* Texture) const
{
	if (IsValid(Image_Icon))
	{
		Image_Icon->SetBrushFromTexture(Texture);
	}
}

void UINV_Leaf_Image::SetBoxSize(const FVector2D& Size) const
{
	if (IsValid(SizeBox_Icon))
	{
		SizeBox_Icon->SetWidthOverride(Size.X);
		SizeBox_Icon->SetHeightOverride(Size.Y);
	}
}

void UINV_Leaf_Image::SetImageSize(const FVector2D& Size) const
{
	if (IsValid(Image_Icon))
	{
		Image_Icon->SetDesiredSizeOverride(Size);
	}
}

FVector2D UINV_Leaf_Image::GetBoxSize() const
{
	return IsValid(SizeBox_Icon) ? SizeBox_Icon->GetDesiredSize() : FVector2D::ZeroVector;
}
