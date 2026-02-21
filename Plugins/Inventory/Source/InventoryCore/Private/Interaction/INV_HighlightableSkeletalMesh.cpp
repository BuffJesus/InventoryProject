// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/INV_HighlightableSkeletalMesh.h"

void UINV_HighlightableSkeletalMesh::Highlight_Implementation()
{
	if (HighlightMaterial)
	{
		SetOverlayMaterial(HighlightMaterial);
	}
}

void UINV_HighlightableSkeletalMesh::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
}

