// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SizeBox.h"
#include "UI/Composite/INV_Composite.h"
#include "INV_ItemDescription.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_ItemDescription : public UINV_Composite
{
	GENERATED_BODY()
	
public:
	FORCEINLINE FVector2D GetBoxSize() const
	{
		return IsValid(SizeBox) ? SizeBox->GetDesiredSize() : FVector2D::ZeroVector;
	}
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USizeBox> SizeBox;
};
