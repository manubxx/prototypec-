#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "cybmineTypes.h"
#include "cybmineInventoryComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CYBMINE_API UcybmineInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UcybmineInventoryComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsInventoryOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 SelectedHotbarSlot = 0;

	// Slot 0..5 = Hotbar, Slot 6..23 = Zaino principale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventoryItem> StorageSlots;

	// Slot 0 = Elmo, 1 = Corazza, 2 = Gambali, 3 = Stivali
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventoryItem> ArmorSlots;

	// --- CONTROLLO HOTBAR ---
	void SelectHotbarSlot(int32 SlotIndex);
	void NextHotbarSlot();
	void PrevHotbarSlot();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetSelectedHotbarItem() const;

	// --- OPERAZIONI SUGLI OGGETTI ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& NewItem);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapStorageSlots(int32 IndexA, int32 IndexB);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex);

protected:
	virtual void BeginPlay() override;
};
