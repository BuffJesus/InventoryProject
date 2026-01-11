// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/INV_ItemTags.h"

namespace GameItems
{
	namespace Equipment
	{
		namespace Weapons
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Axe, "GameItems.Equipment.Weapons.Axe", "Axe weapon type")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sword, "GameItems.Equipment.Weapons.Sword", "Sword weapon type")
		}
		namespace Cloaks
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(RedCloak, "GameItems.Equipment.Cloaks.RedCloak", "A majestic red cloak")
		}
		namespace Masks
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SteelMask, "GameItems.Equipment.Masks.SteelMask", "A sturdy steel mask")
		}
	}

	namespace Consumables
	{
		namespace Potions
		{
			namespace Red
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Small, "GameItems.Consumables.Potions.Red.Small", "Small Red Potion")
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Large, "GameItems.Consumables.Potions.Red.Large", "Large Red Potion")
			}
			namespace Blue
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Small, "GameItems.Consumables.Potions.Blue.Small", "Small Blue Potion")
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Large, "GameItems.Consumables.Potions.Blue.Large", "Large Blue Potion")
			}
		}
	}

	namespace Craftables
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(FireFernFruit, "GameItems.Craftables.FireFernFruit", "Crafting ingredient: Fire Fern Fruit")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(LuminDaisy, "GameItems.Craftables.LuminDaisy", "Crafting ingredient: Lumin Daisy")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ScorchPedalBlossom, "GameItems.Craftables.ScorchPedalBlossom", "Crafting ingredient: Scorch Pedal Blossom")
	}
}