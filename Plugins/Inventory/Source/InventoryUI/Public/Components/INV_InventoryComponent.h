// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagement/FastArray/INV_FastArray.h"
#include "INV_InventoryComponent.generated.h"

struct FINV_SlotAvailabilityResult;
struct FINV_InventoryFastArray;
class UINV_ItemComponent;
class UINV_InventoryBase;
class UINV_EquipmentComponent;
class APawn;
class AINV_ProxyMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, UINV_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNoRoomInInventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStackChange, const FINV_SlotAvailabilityResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemEquipStatusChanged, UINV_InventoryItem*, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORYUI_API UINV_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UINV_InventoryComponent();
	// Shows or hides the inventory UI widget.
	void ToggleInventoryMenu();
	bool IsInventoryMenuOpen() const { return bInventoryMenuOpen; }
	// Refresh cursor/UI behavior when active input source changes.
	void OnInputMethodChanged(bool bIsGamepadInput);
	// Registers a UObject for replication when using the subobject list.
	void AddRepSubObj(UObject* SubObj);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Server RPC: add a new runtime item entry from a pickup component.
	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(UINV_ItemComponent* ItemComponent, int32 StackCount);
	
	// Server RPC: merge stacks into an existing item and handle pickup remainder.
	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(UINV_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder);
	
	// Server RPC: remove/adjust inventory item stacks and spawn dropped pickup actor.
	UFUNCTION(Server, Reliable)
	void Server_DropItem(UINV_InventoryItem* Item, int32 StackCount);
	
	// Server RPC: consume one stack and execute consume behavior.
	UFUNCTION(Server, Reliable)
	void Server_ConsumeItem(UINV_InventoryItem* Item);
	
	UFUNCTION(BlueprintCallable, Category="INV|Inventory", BlueprintAuthorityOnly)
	// Entry point for adding an item to the inventory.
	void TryAddItem(UINV_ItemComponent* ItemComponent);
	// Server-side helper that computes safe drop location and spawns pickup actor.
	void SpawnDroppedItem(UINV_InventoryItem* Item, int32 StackCount);
	FORCEINLINE UINV_InventoryBase* GetInventoryMenu() const { return Inventory; }
	FORCEINLINE AINV_ProxyMesh* GetCachedProxyMesh() const { return CachedProxyMesh; }
	bool HasAuthorityOnOwner() const;
	
	UFUNCTION(Server, Reliable) void Server_EquipSlotClicked(UINV_InventoryItem* ItemToEquip, UINV_InventoryItem* ItemToUnequip);
	
	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;
	FInventoryItemChange OnItemChanged;
	FNoRoomInInventory OnNoRoomInInventory;
	FStackChange OnStackChange;
	FItemEquipStatusChanged OnItemEquipped;
	FItemEquipStatusChanged OnItemUnequipped;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

private:
	void ConfigureFastArrayCallbacks();
	FVector ResolveVisualDropSeparation(const FVector& ProposedLocation);
	void RememberSuccessfulDropLocation(const FVector& DropLocation);
	void TrimRecentDropLocations();
	void ApplyPointerInputMode(bool bIsOpen) const;
	void EnsureProxyMeshSpawned();
	void EnsureLiveEquipmentComponent();
	void UpdateProxyMeshVisibility(bool bVisible);
	FTransform BuildProxyMeshSpawnTransform() const;

	// Owning controller used for UI input mode and widget creation.
	TWeakObjectPtr<APlayerController> OwningController { nullptr };
	TWeakObjectPtr<APawn> InputSuppressedPawn { nullptr };
	
	// Creates the inventory UI widget for local controllers.
	void ConstructInventory();
	// Sets UI visibility and input mode.
	void HandleInventoryMenu(ESlateVisibility Visibility, bool bIsOpen);
	
	// Replicated list of inventory items.
	UPROPERTY(Replicated)
	FINV_InventoryFastArray InventoryFastArray;

	// Widget class used to build the inventory UI.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	TSubclassOf<UINV_InventoryBase> InventoryClass { nullptr };
	
	// Actual widget instance for this component.
	UPROPERTY()
	TObjectPtr<UINV_InventoryBase> Inventory { nullptr };

	UPROPERTY(EditAnywhere, Category = "INV|Inventory|Proxy")
	TSubclassOf<AINV_ProxyMesh> ProxyMeshClass { nullptr };

	UPROPERTY()
	TObjectPtr<AINV_ProxyMesh> CachedProxyMesh { nullptr };

	UPROPERTY()
	TObjectPtr<UINV_EquipmentComponent> LiveEquipmentComponent { nullptr };
	
	// Tracks menu state for toggle logic.
	bool bInventoryMenuOpen = false;

	UPROPERTY(EditAnywhere, Category = "INV|Inventory|Proxy")
	float ProxyPreviewForwardOffset { 0.f };

	UPROPERTY(EditAnywhere, Category = "INV|Inventory|Proxy")
	float ProxyPreviewLeftOffset { 0.f };

	UPROPERTY(EditAnywhere, Category = "INV|Inventory|Proxy")
	float ProxyPreviewVerticalOffset { 10000.f };
	
	// Min/max angular offset (degrees) used for randomized drop direction from pawn forward.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float DropSpawnAngleMin { -85.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float DropSpawnAngleMax { 85.f };
	
	// Min/max horizontal distance (cm) from pawn for initial drop candidate.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float DropSpawnDistanceMin { 10.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float DropSpawnDistanceMax { 50.f };
	
	UPROPERTY(EditAnywhere, Category = "INV|Inventory")
	float RelativeSpawnElevation { 70.f };

	// Vertical offset used for downward ground trace start point.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation")
	float DropGroundTraceStartHeight { 150.f };

	// Vertical trace distance used to find a valid surface.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation")
	float DropGroundTraceDepth { 600.f };

	// Radius used to validate drop spawn overlap.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation", meta = (ClampMin = "1.0"))
	float DropValidationRadius { 20.f };

	// Half-height used to validate drop spawn overlap.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation", meta = (ClampMin = "1.0"))
	float DropValidationHalfHeight { 20.f };

	// Small offset from surface normal to avoid starting interpenetrating.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation")
	float DropSurfaceOffset { 2.f };

	// Extra horizontal clearance to keep dropped items out of the player's capsule.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation")
	float DropPlayerClearance { 10.f };

	// Collision-independent separation so dropped pickups do not visually overlap when actors have no collision.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DropVisualSeparationDistance { 55.f };

	// Maximum number of recent drop locations remembered for separation checks.
	UPROPERTY(EditAnywhere, Category = "INV|Inventory|DropValidation", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxRememberedDropLocations { 32 };

	// Recent successful drop locations used for collision-independent visual separation.
	UPROPERTY()
	TArray<FVector> RecentDropLocations;
};
