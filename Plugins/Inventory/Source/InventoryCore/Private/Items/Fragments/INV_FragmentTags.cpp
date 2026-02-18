// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Fragments/INV_FragmentTags.h"

namespace FragmentTags
{
	UE_DEFINE_GAMEPLAY_TAG(GridFragment, "FragmentTags.GridFragment")
	UE_DEFINE_GAMEPLAY_TAG(IconFragment, "FragmentTags.IconFragment")
	UE_DEFINE_GAMEPLAY_TAG(StackableFragment, "FragmentTags.StackableFragment")
	UE_DEFINE_GAMEPLAY_TAG(ConsumableFragment, "FragmentTags.ConsumableFragment")
	
	UE_DEFINE_GAMEPLAY_TAG(ItemNameFragment, "FragmentTags.ItemNameFragment")
	UE_DEFINE_GAMEPLAY_TAG(PrimaryStatFragment, "FragmentTags.PrimaryStatFragment")
	
	UE_DEFINE_GAMEPLAY_TAG(ItemTypeFragment, "FragmentTags.ItemTypeFragment")
	UE_DEFINE_GAMEPLAY_TAG(FlavorTextFragment, "FragmentTags.FlavorTextFragment")
	UE_DEFINE_GAMEPLAY_TAG(SellValueFragment, "FragmentTags.SellValueFragment")
	UE_DEFINE_GAMEPLAY_TAG(RequiredLevelFragment, "FragmentTags.RequiredLevelFragment")
	
	namespace StatMod
	{
		UE_DEFINE_GAMEPLAY_TAG(StatMod_1, "FragmentTags.StatMod.1")
		UE_DEFINE_GAMEPLAY_TAG(StatMod_2, "FragmentTags.StatMod.2")
		UE_DEFINE_GAMEPLAY_TAG(StatMod_3, "FragmentTags.StatMod.3")
	}
	
	namespace Rarity
	{
		UE_DEFINE_GAMEPLAY_TAG(Rarity_Common, "FragmentTags.Rarity.Common")
		UE_DEFINE_GAMEPLAY_TAG(Rarity_Uncommon, "FragmentTags.Rarity.Uncommon")
		UE_DEFINE_GAMEPLAY_TAG(Rarity_Rare, "FragmentTags.Rarity.Rare")
		UE_DEFINE_GAMEPLAY_TAG(Rarity_Epic, "FragmentTags.Rarity.Epic")
		UE_DEFINE_GAMEPLAY_TAG(Rarity_Legendary, "FragmentTags.Rarity.Legendary")
	}
}