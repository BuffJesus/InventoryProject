// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "INV_Leaf.h"
#include "INV_Leaf_Image.generated.h"

class UImage;
class USizeBox;
class UTexture2D;

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_Leaf_Image : public UINV_Leaf
{
	GENERATED_BODY()
	
public:
	void SetImage(UTexture2D* Texture) const;
	void SetBoxSize(const FVector2D& Size) const;
	void SetImageSize(const FVector2D& Size) const;
	FVector2D GetBoxSize() const;

private:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USizeBox> SizeBox_Icon;
};
