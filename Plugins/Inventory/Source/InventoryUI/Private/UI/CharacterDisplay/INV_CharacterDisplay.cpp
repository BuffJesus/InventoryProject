// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CharacterDisplay/INV_CharacterDisplay.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/INV_InventoryComponent.h"
#include "EquipmentManagement/ProxyMesh/INV_ProxyMesh.h"
#include "InputCoreTypes.h"
#include "UI/Utils/INV_InventoryStatics.h"
#include "TimerManager.h"

FReply UINV_CharacterDisplay::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(MyGeometry, MouseEvent);
	}

	CurrentPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	LastPosition = CurrentPosition;

	BeginDrag();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UINV_CharacterDisplay::NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(MyGeometry, MouseEvent);
	}

	EndDrag();
	return FReply::Handled().ReleaseMouseCapture();
}

void UINV_CharacterDisplay::NativeOnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::NativeOnMouseLeave(MouseEvent);
}

void UINV_CharacterDisplay::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResolvePreviewMesh();
}

void UINV_CharacterDisplay::NativeDestruct()
{
	EndDrag();
	Super::NativeDestruct();
}

void UINV_CharacterDisplay::NativeOnMouseCaptureLost(const FCaptureLostEvent& InCaptureLostEvent)
{
	EndDrag();
	Super::NativeOnMouseCaptureLost(InCaptureLostEvent);
}

void UINV_CharacterDisplay::BeginDrag()
{
	if (bIsDragging) return;

	bIsDragging = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DragUpdateTimerHandle,
			this,
			&ThisClass::UpdateDragRotation,
			DragUpdateInterval,
			true);
	}
}

void UINV_CharacterDisplay::EndDrag()
{
	bIsDragging = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DragUpdateTimerHandle);
	}
}

void UINV_CharacterDisplay::UpdateDragRotation()
{
	if (!bIsDragging) return;
	if (!IsValid(GetOwningPlayer())) return;

	if (!Mesh.IsValid())
	{
		ResolvePreviewMesh();
	}
	if (!Mesh.IsValid()) return;

	LastPosition = CurrentPosition;
	CurrentPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	const float HorizontalDelta = LastPosition.X - CurrentPosition.X;
	Mesh->AddLocalRotation(FRotator(0.f, HorizontalDelta, 0.f));
}

void UINV_CharacterDisplay::ResolvePreviewMesh()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!IsValid(OwningPlayer)) return;

	UINV_InventoryComponent* InventoryComponent = UINV_InventoryStatics::GetInventoryComponent(OwningPlayer);
	if (!IsValid(InventoryComponent)) return;

	AINV_ProxyMesh* ProxyMesh = InventoryComponent->GetCachedProxyMesh();
	if (!IsValid(ProxyMesh)) return;

	Mesh = ProxyMesh->GetMesh();
}
