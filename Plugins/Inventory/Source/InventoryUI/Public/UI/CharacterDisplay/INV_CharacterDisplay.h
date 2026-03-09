// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "INV_CharacterDisplay.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYUI_API UINV_CharacterDisplay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& InCaptureLostEvent) override;
	
private:
	void BeginDrag();
	void EndDrag();
	void UpdateDragRotation();
	void ResolvePreviewMesh();

	bool bIsDragging { false };
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	FTimerHandle DragUpdateTimerHandle;
	
	FVector2D CurrentPosition;
	FVector2D LastPosition;
	float DragUpdateInterval { 0.01f };
};
