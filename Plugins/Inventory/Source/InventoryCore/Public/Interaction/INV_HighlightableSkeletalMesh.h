// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_Highlightable.h"
#include "Components/SkeletalMeshComponent.h"
#include "INV_HighlightableSkeletalMesh.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYCORE_API UINV_HighlightableSkeletalMesh : public USkeletalMeshComponent, public IINV_Highlightable
{
	GENERATED_BODY()
	
public:
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category="INV|Inventory")
	TObjectPtr<UMaterialInterface> HighlightMaterial { nullptr };
};

