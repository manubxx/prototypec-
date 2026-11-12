#include "cybmineInventoryComponent.h"
#include "cybmineItemData.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

UcybmineInventoryComponent::UcybmineInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	StorageSlots.SetNum(CybmineConstants::TotalStorageSlotCount);
	ArmorSlots.SetNum(CybmineConstants::ArmorSlotCount);
}

void UcybmineInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UcybmineInventoryComponent, SelectedHotbarSlot);
	DOREPLIFETIME(UcybmineInventoryComponent, StorageSlots);
	DOREPLIFETIME(UcybmineInventoryComponent, ArmorSlots);
}

void UcybmineInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (StorageSlots.Num() != CybmineConstants::TotalStorageSlotCount)
	{
		StorageSlots.SetNum(CybmineConstants::TotalStorageSlotCount);
	}
	if (ArmorSlots.Num() != CybmineConstants::ArmorSlotCount)
	{
		ArmorSlots.SetNum(CybmineConstants::ArmorSlotCount);
	}

	// Pre-carica asincronamente il catalogo degli ItemData registrati nell'Asset Manager
	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		Manager->LoadPrimaryAssetsWithType(FPrimaryAssetType("ItemData"));
	}

	BroadcastEquippedItemChanged();
}

UcybmineItemData* UcybmineInventoryComponent::GetItemData(const FPrimaryAssetId& ItemID)
{
	if (!ItemID.IsValid())
	{
		return nullptr;
	}

	// 1. Registro C++ in memoria
	if (UcybmineItemData* CodeItem = UcybmineItemData::FindCodeItem(ItemID))
	{
		return CodeItem;
	}

	// 2. Asset Manager (solo se già caricato in memoria, zero blocchi sincroni)
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

void UcybmineInventoryComponent::OnRep_StorageSlots()
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
	}
}

void UcybmineInventoryComponent::NextHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectedHotbarSlot = (SelectedHotbarSlot + 1) % CybmineConstants::HotbarSlotCount;
	BroadcastEquippedItemChanged();
}

void UcybmineInventoryComponent::PrevHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectedHotbarSlot = (SelectedHotbarSlot - 1 + CybmineConstants::HotbarSlotCount) % CybmineConstants::HotbarSlotCount;
	BroadcastEquippedItemChanged();
}

FInventoryItem UcybmineInventoryComponent::GetSelectedHotbarItem() const
{
	if (StorageSlots.IsValidIndex(SelectedHotbarSlot))
	{
		return StorageSlots[SelectedHotbarSlot];
	}
	return FInventoryItem();
}

bool UcybmineInventoryComponent::AddItemByID(FPrimaryAssetId ItemID, int32 Quantity)
{
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
	if (NewItem.IsEmpty()) return false;

	UcybmineItemData* StaticData = GetItemData(NewItem.ItemID);
	if (!StaticData) return false;

	const int32 MaxStack = FMath::Max(1, StaticData->MaxStackSize);
	int32 Remaining = NewItem.Quantity;
	bool bModifiedActiveSlot = false;

	// 1. Impila solo se MaxStack > 1
	if (MaxStack > 1)
	{
		for (int32 i = 0; i < StorageSlots.Num(); ++i)
		{
			FInventoryItem& Slot = StorageSlots[i];
			if (!Slot.IsEmpty() && Slot.ItemID == NewItem.ItemID && Slot.Quantity < MaxStack)
			{
				const int32 Space = MaxStack - Slot.Quantity;
				const int32 ToAdd = FMath::Min(Space, Remaining);
				Slot.Quantity += ToAdd;
				Remaining -= ToAdd;

				if (i == SelectedHotbarSlot) bModifiedActiveSlot = true;
				if (Remaining <= 0) break;
			}
		}
	}

	// 2. Riempi il primo slot vuoto disponibile (partendo dalla hotbar 0..5)
	if (Remaining > 0)
	{
		for (int32 i = 0; i < StorageSlots.Num(); ++i)
		{
			FInventoryItem& Slot = StorageSlots[i];
			if (Slot.IsEmpty())
			{
				Slot = NewItem;
				Slot.Quantity = FMath::Min(MaxStack, Remaining);

				if (MaxStack <= 1 && !Slot.UniqueInstanceID.IsValid())
				{
					Slot.UniqueInstanceID = FGuid::NewGuid();
				}

				Remaining -= Slot.Quantity;
				if (i == SelectedHotbarSlot) bModifiedActiveSlot = true;

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
	if (!StorageSlots.IsValidIndex(IndexA) || !StorageSlots.IsValidIndex(IndexB) || IndexA == IndexB)
	{
		return;
	}

	FInventoryItem Temp = StorageSlots[IndexA];
	StorageSlots[IndexA] = StorageSlots[IndexB];
	StorageSlots[IndexB] = Temp;

	if (IndexA == SelectedHotbarSlot || IndexB == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}
}

bool UcybmineInventoryComponent::EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex)
{
	if (!StorageSlots.IsValidIndex(StorageIndex) || !ArmorSlots.IsValidIndex(ArmorIndex))
	{
		return false;
	}

	const FInventoryItem& Item = StorageSlots[StorageIndex];
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

	FInventoryItem Temp = ArmorSlots[ArmorIndex];
	ArmorSlots[ArmorIndex] = StorageSlots[StorageIndex];
	StorageSlots[StorageIndex] = Temp;

	if (StorageIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}

	return true;
}