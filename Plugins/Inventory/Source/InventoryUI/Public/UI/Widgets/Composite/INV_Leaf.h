// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_Composite_Base.h"
#include "INV_Leaf.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_Leaf : public UINV_Composite_Base
{
	GENERATED_BODY()
	
public:
	virtual void ApplyFunction(FuncType Function) override;
};
