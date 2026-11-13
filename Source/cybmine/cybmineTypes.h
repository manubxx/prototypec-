#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "cybmineTypes.generated.h"


UENUM(BlueprintType)
enum class EPlayerJob : uint8
{
	None        UMETA(DisplayName = "Disoccupato"),
	Miner       UMETA(DisplayName = "Minatore"),
	Blacksmith  UMETA(DisplayName = "Fabbro"),
	Warrior     UMETA(DisplayName = "Guerriero"),
	Builder     UMETA(DisplayName = "Costruttore")
};


class UcybmineInventoryComponent;

namespace CybmineConstants
{
	static constexpr int32 HotbarSlotCount = 6;
	static constexpr int32 InventoryRows = 3;
	static constexpr int32 InventoryCols = 6;
	static constexpr int32 MainInventorySlotCount = InventoryRows * InventoryCols; // 18
	static constexpr int32 TotalStorageSlotCount = MainInventorySlotCount + HotbarSlotCount; // 24 (0..5 Hotbar, 6..23 Zaino)
	static constexpr int32 ArmorSlotCount = 4; // 0: Elmo, 1: Corazza, 2: Gambali, 3: Stivali
	static constexpr float MaxContainerInteractDistance = 450.0f; // 4.5 metri anti-exploit
}

UENUM(BlueprintType)
enum class ECybmineContainerType : uint8
{
	PlayerInventory UMETA(DisplayName = "Inventario Giocatore"),
	FactionChest    UMETA(DisplayName = "Cassa Fazione / Baule"),
	AuctionStorage  UMETA(DisplayName = "Deposito Aste"),
	LootBag         UMETA(DisplayName = "Sacco Loot PvP")
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None        UMETA(DisplayName = "Nessuno"),
	Weapon      UMETA(DisplayName = "Arma"),
	Shield      UMETA(DisplayName = "Scudo"),
	Tool        UMETA(DisplayName = "Strumento"),
	Block       UMETA(DisplayName = "Blocco"),
	Food        UMETA(DisplayName = "Cibo"),
	Helmet      UMETA(DisplayName = "Elmo"),
	Chestplate  UMETA(DisplayName = "Corazza"),
	Leggings    UMETA(DisplayName = "Gambali"),
	Boots       UMETA(DisplayName = "Stivali")
};

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FPrimaryAssetId ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float CurrentDurability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGuid UniqueInstanceID;

	bool IsEmpty() const
	{
		return !ItemID.IsValid() || Quantity <= 0;
	}

	void Clear()
	{
		ItemID = FPrimaryAssetId();
		Quantity = 0;
		CurrentDurability = 0.0f;
		UniqueInstanceID.Invalidate();
	}
};

// --- SINGOLO SLOT DELTA-REPLICATO ---
USTRUCT(BlueprintType)
struct FCybmineInventorySlotEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FInventoryItem Item;

	void PreReplicatedRemove(const struct FCybmineInventoryList& InArraySerializer);
	void PostReplicatedAdd(const struct FCybmineInventoryList& InArraySerializer);
	void PostReplicatedChange(const struct FCybmineInventoryList& InArraySerializer);
};

// --- CONTENITORE FAST ARRAY SERIALIZER ---
USTRUCT(BlueprintType)
struct FCybmineInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FCybmineInventorySlotEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UcybmineInventoryComponent> OwnerComponent = nullptr;

	UPROPERTY(NotReplicated)
	bool bIsArmorList = false;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FCybmineInventorySlotEntry, FCybmineInventoryList>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FCybmineInventoryList> : public TStructOpsTypeTraitsBase2<FCybmineInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};