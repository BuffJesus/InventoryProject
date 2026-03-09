// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagement/ProxyMesh/INV_ProxyMesh.h"
#include "InventoryUI/Public/Components/INV_EquipmentComponent.h"

AINV_ProxyMesh::AINV_ProxyMesh()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	EquipmentComponent = CreateDefaultSubobject<UINV_EquipmentComponent>(TEXT("Equipment"));
}

void AINV_ProxyMesh::BeginPlay()
{
	Super::BeginPlay();
	
}
