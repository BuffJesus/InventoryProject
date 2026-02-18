// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

#define INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(TagName) extern INVENTORYCORE_API FNativeGameplayTag TagName;

namespace GameItems
{
	namespace Equipment
	{
		namespace Weapons
		{
			INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Axe)
			INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Sword)
		}
		namespace Cloaks
		{
			INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(RedCloak)
		}
		namespace Masks
		{
			INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelMask)
		}
	}
	
	namespace Consumables
	{
		namespace Potions
		{
			namespace Red
			{
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Small)
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Large)
			}
			namespace Blue
			{
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Small)
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Large)
			}
			namespace Frog
			{
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Small)
				INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(Large)
			}
		}
	}
	
	namespace Craftables
	{
		INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(FireFernFruit)
		INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(LuminDaisy)
		INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN(ScorchPedalBlossom)
	}
}

#undef INVCORE_DECLARE_GAMEPLAY_TAG_EXTERN
