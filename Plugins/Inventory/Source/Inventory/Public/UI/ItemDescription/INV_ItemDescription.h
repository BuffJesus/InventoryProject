// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SizeBox.h"
#include "INV_ItemDescription.generated.h"

class USizeBox;
/**
 * 
 */
UCLASS()
class INVENTORY_API UINV_ItemDescription : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE FVector2D GetBoxSize() const { return SizeBox->GetDesiredSize(); }
	
private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USizeBox> SizeBox;
};
