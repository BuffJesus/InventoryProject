#pragma once

struct FINV_ItemPresentationSnapshot;
struct FINV_ItemManifest;
class UINV_InventoryItem;
class UINV_Composite_Base;

class INVENTORYUI_API FINV_ItemPresentationUtils
{
public:
	static void AssimilateInventoryFragments(const FINV_ItemManifest& Manifest, UINV_Composite_Base* Composite);
	static void AssimilateInventoryItem(const UINV_InventoryItem& Item, UINV_Composite_Base* Composite);
	static void AssimilatePresentationSnapshot(const FINV_ItemPresentationSnapshot& Snapshot, UINV_Composite_Base* Composite);
};
