#include "cybmineInventoryComponent.h"
#include "cybmineItemData.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

// ============================================================================
// CALLBACKS DI FFastArraySerializerItem (Eseguite automaticamente sui Client)
// ============================================================================

void FCybmineInventorySlotEntry::PreReplicatedRemove(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, FInventoryItem(), InArraySerializer.bIsArmorList);
	}
}

void FCybmineInventorySlotEntry::PostReplicatedAdd(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, Item, InArraySerializer.bIsArmorList);
	}
}

void FCybmineInventorySlotEntry::PostReplicatedChange(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, Item, InArraySerializer.bIsArmorList);
	}
}

// ============================================================================
// UcybmineInventoryComponent
// ============================================================================

UcybmineInventoryComponent::UcybmineInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	StorageList.OwnerComponent = this;
	StorageList.bIsArmorList = false;

	ArmorList.OwnerComponent = this;
	ArmorList.bIsArmorList = true;
}

void UcybmineInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UcybmineInventoryComponent, SelectedHotbarSlot);
	DOREPLIFETIME(UcybmineInventoryComponent, StorageList);
	DOREPLIFETIME(UcybmineInventoryComponent, ArmorList);
}

void UcybmineInventoryComponent::InitializeSlotArrays()
{
	StorageList.OwnerComponent = this;
	StorageList.bIsArmorList = false;

	ArmorList.OwnerComponent = this;
	ArmorList.bIsArmorList = true;

	// Solo il Server crea e possiede la struttura autoritativa iniziale degli slot
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (StorageList.Entries.Num() != CybmineConstants::TotalStorageSlotCount)
		{
			StorageList.Entries.SetNum(CybmineConstants::TotalStorageSlotCount);
			for (int32 i = 0; i < CybmineConstants::TotalStorageSlotCount; ++i)
			{
				StorageList.Entries[i].SlotIndex = i;
				StorageList.Entries[i].Item.Clear();
			}
			StorageList.MarkArrayDirty();
		}

		if (ArmorList.Entries.Num() != CybmineConstants::ArmorSlotCount)
		{
			ArmorList.Entries.SetNum(CybmineConstants::ArmorSlotCount);
			for (int32 i = 0; i < CybmineConstants::ArmorSlotCount; ++i)
			{
				ArmorList.Entries[i].SlotIndex = i;
				ArmorList.Entries[i].Item.Clear();
			}
			ArmorList.MarkArrayDirty();
		}
	}
}

void UcybmineInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeSlotArrays();

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		Manager->LoadPrimaryAssetsWithType(FPrimaryAssetType("ItemData"));
	}

	BroadcastEquippedItemChanged();
}

void UcybmineInventoryComponent::NotifySlotReplicated(int32 SlotIndex, const FInventoryItem& UpdatedItem, bool bIsArmor)
{
	OnInventorySlotUpdated.Broadcast(SlotIndex, UpdatedItem, bIsArmor);

	// Se è cambiato lo slot attualmente impugnato nella hotbar, aggiorna la mesh 3D in mano
	if (!bIsArmor && SlotIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}
}

FInventoryItem UcybmineInventoryComponent::GetStorageItem(int32 Index) const
{
	if (StorageList.Entries.IsValidIndex(Index))
	{
		return StorageList.Entries[Index].Item;
	}

	// Fallback se su un client gli elementi replicati arrivano fuori ordine
	for (const FCybmineInventorySlotEntry& Entry : StorageList.Entries)
	{
		if (Entry.SlotIndex == Index)
		{
			return Entry.Item;
		}
	}
	return FInventoryItem();
}

FInventoryItem UcybmineInventoryComponent::GetArmorItem(int32 Index) const
{
	if (ArmorList.Entries.IsValidIndex(Index))
	{
		return ArmorList.Entries[Index].Item;
	}

	for (const FCybmineInventorySlotEntry& Entry : ArmorList.Entries)
	{
		if (Entry.SlotIndex == Index)
		{
			return Entry.Item;
		}
	}
	return FInventoryItem();
}

UcybmineItemData* UcybmineInventoryComponent::GetItemData(const FPrimaryAssetId& ItemID)
{
	if (!ItemID.IsValid())
	{
		return nullptr;
	}

	if (UcybmineItemData* CodeItem = UcybmineItemData::FindCodeItem(ItemID))
	{
		return CodeItem;
	}

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		return Cast<UcybmineItemData>(Manager->GetPrimaryAssetObject(ItemID));
	}

	return nullptr;
}

void UcybmineInventoryComponent::GetItemDataAsync(const FPrimaryAssetId& ItemID, FOnItemDataLoaded OnLoaded)
{
	if (!ItemID.IsValid())
	{
		OnLoaded.ExecuteIfBound(nullptr);
		return;
	}

	if (UcybmineItemData* AlreadyLoaded = GetItemData(ItemID))
	{
		OnLoaded.ExecuteIfBound(AlreadyLoaded);
		return;
	}

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		Manager->LoadPrimaryAsset(ItemID, TArray<FName>(), FStreamableDelegate::CreateLambda([ItemID, OnLoaded]()
			{
				OnLoaded.ExecuteIfBound(GetItemData(ItemID));
			}));
	}
	else
	{
		OnLoaded.ExecuteIfBound(nullptr);
	}
}

void UcybmineInventoryComponent::OnRep_SelectedHotbarSlot()
{
	BroadcastEquippedItemChanged();
}

void UcybmineInventoryComponent::BroadcastEquippedItemChanged()
{
	OnEquippedItemChanged.Broadcast(GetSelectedHotbarItem());
}

void UcybmineInventoryComponent::SelectHotbarSlot(int32 SlotIndex)
{
	if (bIsInventoryOpen) return;

	const int32 Clamped = FMath::Clamp(SlotIndex, 0, CybmineConstants::HotbarSlotCount - 1);
	if (SelectedHotbarSlot != Clamped)
	{
		SelectedHotbarSlot = Clamped;
		BroadcastEquippedItemChanged();

		if (GetOwner() && !GetOwner()->HasAuthority())
		{
			Server_SelectHotbarSlot(Clamped);
		}
	}
}

void UcybmineInventoryComponent::Server_SelectHotbarSlot_Implementation(int32 SlotIndex)
{
	const int32 Clamped = FMath::Clamp(SlotIndex, 0, CybmineConstants::HotbarSlotCount - 1);
	if (SelectedHotbarSlot != Clamped)
	{
		SelectedHotbarSlot = Clamped;
		BroadcastEquippedItemChanged();
	}
}

void UcybmineInventoryComponent::NextHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectHotbarSlot((SelectedHotbarSlot + 1) % CybmineConstants::HotbarSlotCount);
}

void UcybmineInventoryComponent::PrevHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectHotbarSlot((SelectedHotbarSlot - 1 + CybmineConstants::HotbarSlotCount) % CybmineConstants::HotbarSlotCount);
}

FInventoryItem UcybmineInventoryComponent::GetSelectedHotbarItem() const
{
	return GetStorageItem(SelectedHotbarSlot);
}

bool UcybmineInventoryComponent::AddItemByID(FPrimaryAssetId ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	UcybmineItemData* StaticData = GetItemData(ItemID);
	if (!StaticData || Quantity <= 0)
	{
		return false;
	}

	FInventoryItem NewInstance;
	NewInstance.ItemID = ItemID;
	NewInstance.Quantity = Quantity;
	NewInstance.CurrentDurability = StaticData->MaxDurability;

	if (StaticData->MaxStackSize <= 1)
	{
		NewInstance.UniqueInstanceID = FGuid::NewGuid();
	}

	return AddItem(NewInstance);
}

bool UcybmineInventoryComponent::AddItem(const FInventoryItem& NewItem)
{
	// Solo il server può generare o aggiungere oggetti (anti-cheat)
	if (!GetOwner() || !GetOwner()->HasAuthority() || NewItem.IsEmpty())
	{
		return false;
	}

	InitializeSlotArrays();

	UcybmineItemData* StaticData = GetItemData(NewItem.ItemID);
	if (!StaticData) return false;

	const int32 MaxStack = FMath::Max(1, StaticData->MaxStackSize);
	int32 Remaining = NewItem.Quantity;
	bool bModifiedActiveSlot = false;

	// 1. Impila solo se MaxStack > 1 e marca SOLO gli slot modificati come Dirty
	if (MaxStack > 1)
	{
		for (int32 i = 0; i < StorageList.Entries.Num(); ++i)
		{
			FCybmineInventorySlotEntry& Entry = StorageList.Entries[i];
			if (!Entry.Item.IsEmpty() && Entry.Item.ItemID == NewItem.ItemID && Entry.Item.Quantity < MaxStack)
			{
				const int32 Space = MaxStack - Entry.Item.Quantity;
				const int32 ToAdd = FMath::Min(Space, Remaining);
				Entry.Item.Quantity += ToAdd;
				Remaining -= ToAdd;

				// DELTA SERIALIZATION: invia in rete solo questo singolo slot
				StorageList.MarkItemDirty(Entry);
				OnInventorySlotUpdated.Broadcast(Entry.SlotIndex, Entry.Item, false);

				if (Entry.SlotIndex == SelectedHotbarSlot) bModifiedActiveSlot = true;
				if (Remaining <= 0) break;
			}
		}
	}

	// 2. Riempi il primo slot vuoto disponibile e marcalo come Dirty
	if (Remaining > 0)
	{
		for (int32 i = 0; i < StorageList.Entries.Num(); ++i)
		{
			FCybmineInventorySlotEntry& Entry = StorageList.Entries[i];
			if (Entry.Item.IsEmpty())
			{
				Entry.Item = NewItem;
				Entry.Item.Quantity = FMath::Min(MaxStack, Remaining);

				if (MaxStack <= 1 && !Entry.Item.UniqueInstanceID.IsValid())
				{
					Entry.Item.UniqueInstanceID = FGuid::NewGuid();
				}

				Remaining -= Entry.Item.Quantity;

				// DELTA SERIALIZATION: invia in rete solo questo singolo slot
				StorageList.MarkItemDirty(Entry);
				OnInventorySlotUpdated.Broadcast(Entry.SlotIndex, Entry.Item, false);

				if (Entry.SlotIndex == SelectedHotbarSlot) bModifiedActiveSlot = true;
				if (Remaining <= 0) break;
			}
		}
	}

	if (bModifiedActiveSlot)
	{
		BroadcastEquippedItemChanged();
	}

	return (Remaining < NewItem.Quantity);
}

void UcybmineInventoryComponent::SwapStorageSlots(int32 IndexA, int32 IndexB)
{
	if (IndexA == IndexB || IndexA < 0 || IndexB < 0 ||
		IndexA >= CybmineConstants::TotalStorageSlotCount || IndexB >= CybmineConstants::TotalStorageSlotCount)
	{
		return;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_SwapStorageSlots(IndexA, IndexB);
		return;
	}

	if (!StorageList.Entries.IsValidIndex(IndexA) || !StorageList.Entries.IsValidIndex(IndexB))
	{
		return;
	}

	FInventoryItem Temp = StorageList.Entries[IndexA].Item;
	StorageList.Entries[IndexA].Item = StorageList.Entries[IndexB].Item;
	StorageList.Entries[IndexB].Item = Temp;

	// Marca SOLO i due slot coinvolti nello scambio
	StorageList.MarkItemDirty(StorageList.Entries[IndexA]);
	StorageList.MarkItemDirty(StorageList.Entries[IndexB]);

	OnInventorySlotUpdated.Broadcast(IndexA, StorageList.Entries[IndexA].Item, false);
	OnInventorySlotUpdated.Broadcast(IndexB, StorageList.Entries[IndexB].Item, false);

	if (IndexA == SelectedHotbarSlot || IndexB == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}
}

void UcybmineInventoryComponent::Server_SwapStorageSlots_Implementation(int32 IndexA, int32 IndexB)
{
	SwapStorageSlots(IndexA, IndexB);
}

bool UcybmineInventoryComponent::EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex)
{
	if (StorageIndex < 0 || StorageIndex >= CybmineConstants::TotalStorageSlotCount ||
		ArmorIndex < 0 || ArmorIndex >= CybmineConstants::ArmorSlotCount)
	{
		return false;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_EquipArmorFromStorage(StorageIndex, ArmorIndex);
		return true;
	}

	if (!StorageList.Entries.IsValidIndex(StorageIndex) || !ArmorList.Entries.IsValidIndex(ArmorIndex))
	{
		return false;
	}

	const FInventoryItem& Item = StorageList.Entries[StorageIndex].Item;
	if (!Item.IsEmpty())
	{
		UcybmineItemData* StaticData = GetItemData(Item.ItemID);
		if (!StaticData) return false;

		const bool bValidMatch =
			(ArmorIndex == 0 && StaticData->Category == EItemCategory::Helmet) ||
			(ArmorIndex == 1 && StaticData->Category == EItemCategory::Chestplate) ||
			(ArmorIndex == 2 && StaticData->Category == EItemCategory::Leggings) ||
			(ArmorIndex == 3 && StaticData->Category == EItemCategory::Boots);

		if (!bValidMatch)
		{
			return false;
		}
	}

	FInventoryItem Temp = ArmorList.Entries[ArmorIndex].Item;
	ArmorList.Entries[ArmorIndex].Item = StorageList.Entries[StorageIndex].Item;
	StorageList.Entries[StorageIndex].Item = Temp;

	// Marca SOLO lo slot dello zaino e lo slot armatura modificati
	StorageList.MarkItemDirty(StorageList.Entries[StorageIndex]);
	ArmorList.MarkItemDirty(ArmorList.Entries[ArmorIndex]);

	OnInventorySlotUpdated.Broadcast(StorageIndex, StorageList.Entries[StorageIndex].Item, false);
	OnInventorySlotUpdated.Broadcast(ArmorIndex, ArmorList.Entries[ArmorIndex].Item, true);

	if (StorageIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}

	return true;
}

void UcybmineInventoryComponent::Server_EquipArmorFromStorage_Implementation(int32 StorageIndex, int32 ArmorIndex)
{
	EquipArmorFromStorage(StorageIndex, ArmorIndex);
}