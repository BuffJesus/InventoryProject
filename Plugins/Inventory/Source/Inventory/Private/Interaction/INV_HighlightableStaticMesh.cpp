// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/INV_HighlightableStaticMesh.h"

void UINV_HighlightableStaticMesh::Highlight_Implementation()
{
	if (HighlightMaterial)
	{
		bDisallowNanite = true;
		SetOverlayMaterial(HighlightMaterial);
	}
}

void UINV_HighlightableStaticMesh::UnHighlight_Implementation()
{
	SetOverlayMaterial(nullptr);
	bDisallowNanite = false;
}