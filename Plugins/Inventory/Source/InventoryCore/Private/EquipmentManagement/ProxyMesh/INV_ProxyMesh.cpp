// Fill out your copyright notice in the Description page of Project Settings.

#include "EquipmentManagement/ProxyMesh/INV_ProxyMesh.h"
#include "GameFramework/Character.h"
#include "InventoryUI/Public/Components/INV_EquipmentComponent.h"
#include "TimerManager.h"

AINV_ProxyMesh::AINV_ProxyMesh()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	
	EquipmentComponent = CreateDefaultSubobject<UINV_EquipmentComponent>(TEXT("Equipment"));
	EquipmentComponent->InitializeEquipmentContext(nullptr, Mesh, true);
}

void AINV_ProxyMesh::InitializeProxy(APlayerController* PlayerController)
{
	ObservedPlayerController = PlayerController;
	InitializationAttempts = 0;
	ScheduleInitializationRetry();
}

void AINV_ProxyMesh::BeginPlay()
{
	Super::BeginPlay();
	ScheduleInitializationRetry();
}

void AINV_ProxyMesh::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerForNextTick);
	}

	Super::EndPlay(EndPlayReason);
}

void AINV_ProxyMesh::AttemptInitialization()
{
	if (TryInitializeFromPlayerController(ResolvePlayerController()))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TimerForNextTick);
		}
		return;
	}

	++InitializationAttempts;
	if (InitializationAttempts < MaxInitializationAttempts)
	{
		ScheduleInitializationRetry();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Proxy mesh failed to initialize after %d attempts."), MaxInitializationAttempts);
}

void AINV_ProxyMesh::ScheduleInitializationRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::AttemptInitialization);
}

APlayerController* AINV_ProxyMesh::ResolvePlayerController() const
{
	if (ObservedPlayerController.IsValid())
	{
		return ObservedPlayerController.Get();
	}

	UWorld* World = GetWorld();
	return IsValid(World) ? World->GetFirstPlayerController() : nullptr;
}

bool AINV_ProxyMesh::TryInitializeFromPlayerController(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return false;

	ACharacter* Character { Cast<ACharacter>(PlayerController->GetPawn()) };
	if (!IsValid(Character)) return false;

	USkeletalMeshComponent* CharacterMesh { Character->GetMesh() };
	if (!IsValid(CharacterMesh)) return false;

	ObservedPlayerController = PlayerController;
	SourceMesh = CharacterMesh;
	Mesh->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());

	if (UAnimInstance* SourceAnimInstance = SourceMesh->GetAnimInstance())
	{
		Mesh->SetAnimInstanceClass(SourceAnimInstance->GetClass());
	}

	EquipmentComponent->InitializeEquipmentContext(PlayerController, Mesh, true);
	return true;
}
