// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UINV_ItemPopUp;
class UINV_GridSlot;
class UINV_InventoryItem;
class UCanvasPanel;
class UUserWidget;
class APlayerController;

/**
 * Manages item popup menu creation, positioning, and lifecycle.
 * Handles popup action callbacks and click-outside-to-close detection.
 */
class INVENTORY_API FINV_GridPopupManager
{
public:
	/**
	 * Create and display an item popup menu at the clicked grid slot.
	 * Note: Caller must bind callbacks after creation using returned popup.
	 *
	 * @param GridSlots All grid slots
	 * @param GridIndex Index of the clicked slot
	 * @param ItemPopUpClass Widget class for the popup
	 * @param OwningCanvasPanel Canvas to add popup to
	 * @param OwningWidget Widget creating the popup
	 * @param PlayerController Player controller for input
	 * @return Created popup widget (or null if failed)
	 */
	static UINV_ItemPopUp* CreateItemPopup(
		TObjectPtr<UINV_ItemPopUp>& ItemPopUp,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridIndex,
		TSubclassOf<UINV_ItemPopUp> ItemPopUpClass,
		UCanvasPanel* OwningCanvasPanel,
		UUserWidget* OwningWidget,
		APlayerController* PlayerController);

	/**
	 * Close the active popup if user clicked outside of it.
	 *
	 * @param ItemPopUp Current active popup (or null)
	 * @param PlayerController Player controller for input
	 * @return True if popup was closed
	 */
	static bool ClosePopupIfClickedOutside(
		TObjectPtr<UINV_ItemPopUp>& ItemPopUp,
		APlayerController* PlayerController);

	/**
	 * Force close the active popup menu.
	 *
	 * @param ItemPopUp Current active popup (or null)
	 */
	static void CloseActiveItemPopup(TObjectPtr<UINV_ItemPopUp>& ItemPopUp);
};
