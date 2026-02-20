// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_Composite_Base.h"
#include "INV_Composite.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_Composite : public UINV_Composite_Base
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual void ApplyFunction(FuncType Function) override;
	virtual void Collapse() override;
	
private:
	UPROPERTY() TArray<TObjectPtr<UINV_Composite_Base>> Children;
};
