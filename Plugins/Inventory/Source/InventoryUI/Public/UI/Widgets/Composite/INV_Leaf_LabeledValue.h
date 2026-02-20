// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_Leaf.h"
#include "INV_Leaf_LabeledValue.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_Leaf_LabeledValue : public UINV_Leaf
{
	GENERATED_BODY()
	
public:
	void SetText_Label(const FText& Text, bool bCollapse) const;
	void SetText_Value(const FText& Text, bool bCollapse) const;
	virtual void NativePreConstruct() override;
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr <UTextBlock> Text_Label;
	UPROPERTY(meta = (BindWidget)) TObjectPtr <UTextBlock> Text_Value;
	
	UPROPERTY(EditAnywhere, Category="INV|Text", meta = (ClampMin = 8, ClampMax = 100))
	int32 FontSize_Label { 12 };
	
	UPROPERTY(EditAnywhere, Category="INV|Text", meta = (ClampMin = 8, ClampMax = 100))
	int32 FontSize_Value { 18 };
};
