#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "cybmineTypes.h"
#include "cybmineInventoryComponent.generated.h"

class UcybmineItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged, const FInventoryItem&, NewEquippedItem);
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsInventoryOpen = false;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedHotbarSlot, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 SelectedHotbarSlot = 0;

	// Slot 0..5 = Hotbar, Slot 6..23 = Zaino principale
	UPROPERTY(ReplicatedUsing = OnRep_StorageSlots, EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventoryItem> StorageSlots;

	// Slot 0 = Elmo, 1 = Corazza, 2 = Gambali, 3 = Stivali
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventoryItem> ArmorSlots;

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

	// --- OPERAZIONI SUGLI OGGETTI ---
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

	UFUNCTION()
	void OnRep_SelectedHotbarSlot();

	UFUNCTION()
	void OnRep_StorageSlots();

	void BroadcastEquippedItemChanged();
};