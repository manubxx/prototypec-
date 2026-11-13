#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "cybmineTypes.generated.h"

// ==========================================
// 1. ENUMERAZIONI BASE (Indipendenti)
// ==========================================

// --- SISTEMA CHAT ---
UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	Global		UMETA(DisplayName = "Globale"),
	Local		UMETA(DisplayName = "Locale"),
	Faction		UMETA(DisplayName = "Fazione"),
	System		UMETA(DisplayName = "Sistema")
};

// --- GRADI FAZIONE ---
UENUM(BlueprintType)
enum class EFactionRank : uint8
{
	Recruit		UMETA(DisplayName = "Recluta"),
	Member		UMETA(DisplayName = "Membro"),
	Officer		UMETA(DisplayName = "Ufficiale"),
	Leader		UMETA(DisplayName = "Leader")
};

// --- LAVORI GIOCATORE ---
UENUM(BlueprintType)
enum class EPlayerJob : uint8
{
	None        UMETA(DisplayName = "Disoccupato"),
	Miner       UMETA(DisplayName = "Minatore"),
	Blacksmith  UMETA(DisplayName = "Fabbro"),
	Warrior     UMETA(DisplayName = "Guerriero"),
	Builder     UMETA(DisplayName = "Costruttore")
};

// --- CONTENITORI ---
UENUM(BlueprintType)
enum class ECybmineContainerType : uint8
{
	PlayerInventory UMETA(DisplayName = "Inventario Giocatore"),
	FactionChest    UMETA(DisplayName = "Cassa Fazione / Baule"),
	AuctionStorage  UMETA(DisplayName = "Deposito Aste"),
	LootBag         UMETA(DisplayName = "Sacco Loot PvP")
};

// --- CATEGORIE OGGETTI ---
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


// ==========================================
// 2. STRUTTURE FAZIONE E COSTANTI
// ==========================================

USTRUCT(BlueprintType)
struct FFactionMember
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	EFactionRank Rank;

	FFactionMember() : PlayerName(TEXT("Unknown")), Rank(EFactionRank::Recruit) {}
	FFactionMember(FString InName, EFactionRank InRank) : PlayerName(InName), Rank(InRank) {}
};

USTRUCT(BlueprintType)
struct FFactionClaim
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FIntPoint ChunkCoords;

	UPROPERTY(BlueprintReadWrite)
	FGuid FactionID;

	FFactionClaim() : ChunkCoords(FIntPoint(0, 0)), FactionID(FGuid()) {}
	FFactionClaim(FIntPoint InCoords, FGuid InFactionID) : ChunkCoords(InCoords), FactionID(InFactionID) {}
};

USTRUCT(BlueprintType)
struct FFactionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid FactionID;

	UPROPERTY(BlueprintReadWrite)
	FString FactionName;

	UPROPERTY(BlueprintReadWrite)
	TMap<FString, FFactionMember> Members;

	UPROPERTY(BlueprintReadWrite)
	int32 BankBalance;

	FFactionData() : FactionID(FGuid()), FactionName(TEXT("")), BankBalance(0) {}
};

class UcybmineInventoryComponent;

namespace CybmineConstants
{
	static constexpr int32 HotbarSlotCount = 6;
	static constexpr int32 InventoryRows = 3;
	static constexpr int32 InventoryCols = 6;
	static constexpr int32 MainInventorySlotCount = InventoryRows * InventoryCols; // 18
	static constexpr int32 TotalStorageSlotCount = MainInventorySlotCount + HotbarSlotCount; // 24
	static constexpr int32 ArmorSlotCount = 4;
	static constexpr float MaxContainerInteractDistance = 450.0f;
}


// ==========================================
// 3. CORE ITEM (Deve precedere Aste e Slots)
// ==========================================

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


// ==========================================
// 4. STRUTTURE DIPENDENTI DA FInventoryItem
// ==========================================

// --- SISTEMA ASTE (AUCTION HOUSE) ---
USTRUCT(BlueprintType)
struct FAuctionListing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid AuctionID;

	UPROPERTY(BlueprintReadWrite)
	FString SellerPlayerID;

	UPROPERTY(BlueprintReadWrite)
	FString SellerName;

	UPROPERTY(BlueprintReadWrite)
	FInventoryItem ItemToSell; // Ora compila correttamente perché FInventoryItem è dichiarato sopra

	UPROPERTY(BlueprintReadWrite)
	int32 BuyoutPrice;

	UPROPERTY(BlueprintReadWrite)
	FDateTime ExpirationTime;

	FAuctionListing() : AuctionID(FGuid()), SellerPlayerID(""), SellerName(""), BuyoutPrice(0) {}
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


// ==========================================
// 5. LISTE E SERIALIZZAZIONE
// ==========================================

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