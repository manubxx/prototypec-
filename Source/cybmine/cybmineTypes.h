#pragma once

#include "CoreMinimal.h"
#include "cybmineTypes.generated.h"

namespace CybmineConstants
{
	static constexpr int32 HotbarSlotCount = 6;
	static constexpr int32 InventoryRows = 3;
	static constexpr int32 InventoryCols = 6;
	static constexpr int32 MainInventorySlotCount = InventoryRows * InventoryCols; // 18
	static constexpr int32 TotalStorageSlotCount = MainInventorySlotCount + HotbarSlotCount; // 24 (0..5 Hotbar, 6..23 Zaino)
	static constexpr int32 ArmorSlotCount = 4; // 0: Elmo, 1: Corazza, 2: Gambali, 3: Stivali
}

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

	// Puntatore leggerissimo al UcybmineItemData
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FPrimaryAssetId ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float CurrentDurability = 0.0f;

	// ID univoco per tracciare oggetti non impilabili nelle aste ed evitare duplicazioni
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