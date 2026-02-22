#include "UI/Inventory/Services/INV_SpatialCategoryService.h"

#include "InputCoreTypes.h"

bool FINV_SpatialCategoryService::IsGridNavigationInput(const FKey& PressedKey)
{
	return
		PressedKey == EKeys::Gamepad_DPad_Up ||
		PressedKey == EKeys::Gamepad_DPad_Down ||
		PressedKey == EKeys::Gamepad_DPad_Left ||
		PressedKey == EKeys::Gamepad_DPad_Right ||
		PressedKey == EKeys::Gamepad_LeftStick_Up ||
		PressedKey == EKeys::Gamepad_LeftStick_Down ||
		PressedKey == EKeys::Gamepad_LeftStick_Left ||
		PressedKey == EKeys::Gamepad_LeftStick_Right ||
		PressedKey == EKeys::Gamepad_FaceButton_Bottom ||
		PressedKey == EKeys::Gamepad_FaceButton_Left ||
		PressedKey == EKeys::Gamepad_FaceButton_Right;
}

int32 FINV_SpatialCategoryService::ResolveNextCategoryIndex(
	const int32 CurrentIndex,
	const int32 Direction,
	const int32 CategoryCount)
{
	if (CategoryCount <= 0 || Direction == 0)
	{
		return INDEX_NONE;
	}

	const int32 SafeCurrent = CurrentIndex == INDEX_NONE ? 0 : FMath::Clamp(CurrentIndex, 0, CategoryCount - 1);
	const int32 Offset = Direction > 0 ? 1 : -1;
	return (SafeCurrent + Offset + CategoryCount) % CategoryCount;
}
