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
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemCategory Category = EItemCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStackSize = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float StatValue = 0.0f; // Danno per armi, Difesa per armature, Cura per cibo

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FLinearColor IconColor = FLinearColor::White;

	bool IsEmpty() const
	{
		return ItemID == NAME_None || Quantity <= 0;
	}

	void Clear()
	{
		ItemID = NAME_None;
		DisplayName = FText::GetEmpty();
		Category = EItemCategory::None;
		Quantity = 0;
		MaxStackSize = 64;
		StatValue = 0.0f;
		IconColor = FLinearColor::White;
	}
};
