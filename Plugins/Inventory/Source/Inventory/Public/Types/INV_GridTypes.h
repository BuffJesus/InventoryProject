#pragma once

#include "INV_GridTypes.generated.h"

UENUM(BlueprintType)
enum class EINV_ItemCategory : uint8
{
	Equippable,
	Consumable,
	Craftable,
	None
};