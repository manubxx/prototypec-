#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "cybmineTypes.h"
#include "cybmineInventoryComponent.generated.h"

class UcybmineItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged, const FInventoryItem&, NewEquippedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventorySlotUpdated, int32, SlotIndex, const FInventoryItem&, UpdatedItem, bool, bIsArmor);
DECLARE_DELEGATE_OneParam(FOnItemDataLoaded, UcybmineItemData*);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CYBMINE_API UcybmineInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UcybmineInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnEquippedItemChanged OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotUpdated OnInventorySlotUpdated;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsInventoryOpen = false;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedHotbarSlot, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 SelectedHotbarSlot = 0;

	// Delta-Replicati tramite FFastArraySerializer (invia solo lo slot modificato)
// Delta-Replicati tramite FFastArraySerializer (invia solo lo slot modificato)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FCybmineInventoryList StorageList;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FCybmineInventoryList ArmorList;

	// --- CALLBACK INTERNE FAST ARRAY ---
	void NotifySlotReplicated(int32 SlotIndex, const FInventoryItem& UpdatedItem, bool bIsArmor);

	// --- ACCESSO SICURO AGLI SLOT ---
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidStorageIndex(int32 Index) const { return StorageList.Entries.IsValidIndex(Index); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidArmorIndex(int32 Index) const { return ArmorList.Entries.IsValidIndex(Index); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetStorageItem(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetArmorItem(int32 Index) const;

	// --- ASSET MANAGER & CARICAMENTO ASINCRONO ---
	UFUNCTION(BlueprintPure, Category = "Inventory|Data")
	static UcybmineItemData* GetItemData(const FPrimaryAssetId& ItemID);

	static void GetItemDataAsync(const FPrimaryAssetId& ItemID, FOnItemDataLoaded OnLoaded);

	// --- CONTROLLO HOTBAR ---
	void SelectHotbarSlot(int32 SlotIndex);
	void NextHotbarSlot();
	void PrevHotbarSlot();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetSelectedHotbarItem() const;

	// --- OPERAZIONI SUGLI OGGETTI (Server Authoritative) ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemByID(FPrimaryAssetId ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& NewItem);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapStorageSlots(int32 IndexA, int32 IndexB);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex);

protected:
	virtual void BeginPlay() override;

	void InitializeSlotArrays();

	UFUNCTION(Server, Reliable)
	void Server_SelectHotbarSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_SwapStorageSlots(int32 IndexA, int32 IndexB);

	UFUNCTION(Server, Reliable)
	void Server_EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex);

	UFUNCTION()
	void OnRep_SelectedHotbarSlot();

	void BroadcastEquippedItemChanged();
};