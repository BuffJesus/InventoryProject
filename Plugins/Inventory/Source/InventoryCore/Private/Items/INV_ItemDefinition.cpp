// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/INV_ItemDefinition.h"

#include "HAL/CriticalSection.h"
#include "Misc/Crc.h"
#include "Misc/ScopeLock.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace
{
struct FINV_ItemDefinitionRegistry
{
	FCriticalSection Mutex;
	TMap<FString, TWeakObjectPtr<UINV_ItemDefinition>> DefinitionsByKey;

	static FINV_ItemDefinitionRegistry& Get()
	{
		static FINV_ItemDefinitionRegistry Registry;
		return Registry;
	}
};

FString BuildManifestSignature(const FINV_ItemManifest& Manifest)
{
	FString ExportedManifest;
	FINV_ItemManifest::StaticStruct()->ExportText(ExportedManifest, &Manifest, nullptr, nullptr, PPF_None, nullptr);
	const uint32 Hash = FCrc::StrCrc32(*ExportedManifest);
	return FString::Printf(TEXT("%08X:%d"), Hash, ExportedManifest.Len());
}
}

void UINV_ItemDefinition::InitializeFromManifest(const FINV_ItemManifest& Manifest, const FINV_ItemDefinitionHandle& InHandle)
{
	ItemManifest = FInstancedStruct::Make<FINV_ItemManifest>(Manifest);
	DefinitionHandle = InHandle;
}

FINV_ItemDefinitionHandle UINV_ItemDefinition::BuildHandleFromManifest(const FINV_ItemManifest& Manifest)
{
	FINV_ItemDefinitionHandle Handle;
	Handle.DefinitionKey = BuildManifestSignature(Manifest);
	return Handle;
}

UINV_ItemDefinition* UINV_ItemDefinition::FindOrCreateSharedDefinition(const FINV_ItemManifest& Manifest, UObject* OuterContext)
{
	const FINV_ItemDefinitionHandle Handle = BuildHandleFromManifest(Manifest);
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	FINV_ItemDefinitionRegistry& Registry = FINV_ItemDefinitionRegistry::Get();
	FScopeLock RegistryLock(&Registry.Mutex);

	if (const TWeakObjectPtr<UINV_ItemDefinition>* ExistingDefinition = Registry.DefinitionsByKey.Find(Handle.DefinitionKey))
	{
		if (ExistingDefinition->IsValid())
		{
			return ExistingDefinition->Get();
		}
	}

	UObject* Outer = IsValid(OuterContext) ? OuterContext : GetTransientPackage();
	UINV_ItemDefinition* NewDefinition = NewObject<UINV_ItemDefinition>(Outer, UINV_ItemDefinition::StaticClass());
	if (!IsValid(NewDefinition))
	{
		return nullptr;
	}

	NewDefinition->InitializeFromManifest(Manifest, Handle);
	Registry.DefinitionsByKey.Add(Handle.DefinitionKey, NewDefinition);
	return NewDefinition;
}
