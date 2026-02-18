#pragma once

struct FINV_ItemManifest;
class UINV_Composite_Base;

class INVENTORY_API FINV_ItemPresentationUtils
{
public:
	static void AssimilateInventoryFragments(const FINV_ItemManifest& Manifest, UINV_Composite_Base* Composite);
};
