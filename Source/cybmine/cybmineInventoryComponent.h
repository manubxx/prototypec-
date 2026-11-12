#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "cybmineTypes.h"
#include "cybmineInventoryComponent.generated.h"

class UcybmineItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged, const FInventoryItem&, NewEquippedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventorySlotUpdated, int32, SlotIndex, const FInventoryItem&, UpdatedItem, bool, bIsArmor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemDroppedWorld, const FInventoryItem&, DroppedItem, FVector, DropLocation, FRotator, DropRotation);
DECLARE_DELEGATE_OneParam(FOnItemDataLoaded, UcybmineItemData*);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CYBMINE_API UcybmineInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UcybmineInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- CONFIGURAZIONE CONTENITORE UNIVERSALE (Agnostico dal proprietario) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	ECybmineContainerType ContainerType = ECybmineContainerType::PlayerInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", meta = (ClampMin = "1", ClampMax = "200"))
	int32 MaxSlotCount = CybmineConstants::TotalStorageSlotCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bHasArmorSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bHasHotbar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	float MaxInteractionDistance = CybmineConstants::MaxContainerInteractDistance;

	// --- EVENTI ---
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnEquippedItemChanged OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotUpdated OnInventorySlotUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemDroppedWorld OnItemDroppedWorld;

	// --- STATO LOCALE & REPLICATO ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsInventoryOpen = false;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedHotbarSlot, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 SelectedHotbarSlot = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FCybmineInventoryList StorageList;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FCybmineInventoryList ArmorList;

	// --- CALLBACK INTERNE FAST ARRAY ---
	void NotifySlotReplicated(int32 SlotIndex, const FInventoryItem& UpdatedItem, bool bIsArmor);

	// --- ACCESSO SICURO AGLI SLOT ---
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetStorageCapacity() const { return MaxSlotCount; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidStorageIndex(int32 Index) const { return Index >= 0 && Index < MaxSlotCount && StorageList.Entries.IsValidIndex(Index); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidArmorIndex(int32 Index) const { return bHasArmorSlots && Index >= 0 && Index < CybmineConstants::ArmorSlotCount && ArmorList.Entries.IsValidIndex(Index); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetStorageItem(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetArmorItem(int32 Index) const;

	// --- ASSET MANAGER & CARICAMENTO ASINCRONO ---
	UFUNCTION(BlueprintPure, Category = "Inventory|Data")
	static UcybmineItemData* GetItemData(const FPrimaryAssetId& ItemID);

	static void GetItemDataAsync(const FPrimaryAssetId& ItemID, FOnItemDataLoaded OnLoaded);

	// --- CONTROLLO HOTBAR (Client chiama RPC, Server valida) ---
	void SelectHotbarSlot(int32 SlotIndex);
	void NextHotbarSlot();
	void PrevHotbarSlot();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetSelectedHotbarItem() const;

	// --- RICHIESTE CLIENT -> SERVER (Zero modifiche locali sul Client) ---
	// Sposta o scambia oggetti nello stesso contenitore (TargetContainer == nullptr o == this) oppure tra due contenitori diversi (es. Zaino <-> Cassa Fazione)
	UFUNCTION(BlueprintCallable, Category = "Inventory|Network")
	void RequestMoveItem(UcybmineInventoryComponent* TargetContainer, int32 SourceSlot, int32 TargetSlot, int32 QuantityToMove = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Network")
	void RequestDropItem(int32 SlotIndex, int32 QuantityToDrop = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Network")
	void SwapStorageSlots(int32 IndexA, int32 IndexB);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Network")
	bool EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex);

	// --- FUNZIONI SERVER-ONLY (Autorità assoluta & Transazioni Atomiche) ---
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Inventory|Server")
	bool AddItemByID(FPrimaryAssetId ItemID, int32 Quantity = 1);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Inventory|Server")
	bool AddItem(const FInventoryItem& NewItem);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Inventory|Server")
	bool ConsumeItemFromSlot(int32 SlotIndex, int32 QuantityToConsume = 1);

	// Transazione atomica validata tra due contenitori qualsiasi (Zaino, Cassa Fazione, Deposito Aste, LootBag)
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Inventory|Server")
	static bool ExecuteAtomicTransfer(
		AActor* RequestingActor,
		UcybmineInventoryComponent* SourceComp,
		int32 SourceSlot,
		UcybmineInventoryComponent* DestComp,
		int32 DestSlot,
		int32 QuantityToMove = -1
	);

protected:
	virtual void BeginPlay() override;

	void InitializeSlotArrays();

	// Verifica lato Server che il richiedente abbia il diritto fisico e logico di operare su questo contenitore
	bool ValidateServerAccess(AActor* RequestingActor) const;

	// Lock anti-race condition durante transazioni atomiche
	bool bTransactionInProgress = false;

	// --- RPC SERVER ---
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SelectHotbarSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestMoveItem(UcybmineInventoryComponent* TargetContainer, int32 SourceSlot, int32 TargetSlot, int32 QuantityToMove);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestDropItem(int32 SlotIndex, int32 QuantityToDrop);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestEquipArmor(int32 StorageIndex, int32 ArmorIndex);

	UFUNCTION()
	void OnRep_SelectedHotbarSlot();

	void BroadcastEquippedItemChanged();
};