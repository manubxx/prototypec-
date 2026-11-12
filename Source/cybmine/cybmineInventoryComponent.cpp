#include "cybmineInventoryComponent.h"

UcybmineInventoryComponent::UcybmineInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	StorageSlots.SetNum(CybmineConstants::TotalStorageSlotCount);
	ArmorSlots.SetNum(CybmineConstants::ArmorSlotCount);
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
}

void UcybmineInventoryComponent::SelectHotbarSlot(int32 SlotIndex)
{
	if (bIsInventoryOpen) return;
	SelectedHotbarSlot = FMath::Clamp(SlotIndex, 0, CybmineConstants::HotbarSlotCount - 1);
}

void UcybmineInventoryComponent::NextHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectedHotbarSlot = (SelectedHotbarSlot + 1) % CybmineConstants::HotbarSlotCount;
}

void UcybmineInventoryComponent::PrevHotbarSlot()
{
	if (bIsInventoryOpen) return;
	SelectedHotbarSlot = (SelectedHotbarSlot - 1 + CybmineConstants::HotbarSlotCount) % CybmineConstants::HotbarSlotCount;
}

FInventoryItem UcybmineInventoryComponent::GetSelectedHotbarItem() const
{
	if (StorageSlots.IsValidIndex(SelectedHotbarSlot))
	{
		return StorageSlots[SelectedHotbarSlot];
	}
	return FInventoryItem();
}

bool UcybmineInventoryComponent::AddItem(const FInventoryItem& NewItem)
{
	if (NewItem.IsEmpty()) return false;

	int32 Remaining = NewItem.Quantity;

	// 1. Prima prova ad impilare sugli slot esistenti con lo stesso ID
	for (FInventoryItem& Slot : StorageSlots)
	{
		if (!Slot.IsEmpty() && Slot.ItemID == NewItem.ItemID && Slot.Quantity < Slot.MaxStackSize)
		{
			const int32 Space = Slot.MaxStackSize - Slot.Quantity;
			const int32 ToAdd = FMath::Min(Space, Remaining);
			Slot.Quantity += ToAdd;
			Remaining -= ToAdd;

			if (Remaining <= 0) return true;
		}
	}

	// 2. Poi riempi il primo slot vuoto disponibile (partendo dalla hotbar 0..5)
	for (FInventoryItem& Slot : StorageSlots)
	{
		if (Slot.IsEmpty())
		{
			Slot = NewItem;
			Slot.Quantity = FMath::Min(NewItem.MaxStackSize, Remaining);
			Remaining -= Slot.Quantity;

			if (Remaining <= 0) return true;
		}
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
		const bool bValidMatch =
			(ArmorIndex == 0 && Item.Category == EItemCategory::Helmet) ||
			(ArmorIndex == 1 && Item.Category == EItemCategory::Chestplate) ||
			(ArmorIndex == 2 && Item.Category == EItemCategory::Leggings) ||
			(ArmorIndex == 3 && Item.Category == EItemCategory::Boots);

		if (!bValidMatch)
		{
			return false;
		}
	}

	FInventoryItem Temp = ArmorSlots[ArmorIndex];
	ArmorSlots[ArmorIndex] = StorageSlots[StorageIndex];
	StorageSlots[StorageIndex] = Temp;
	return true;
}
