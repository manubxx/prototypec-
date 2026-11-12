#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "cybmineTypes.h"
#include "cybmineItemData.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;

UCLASS(BlueprintType)
class CYBMINE_API UcybmineItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- DATI GENERALI ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|General")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|General", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|General")
	EItemCategory Category = EItemCategory::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|General", meta = (ClampMin = "1"))
	int32 MaxStackSize = 64;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 BaseGoldValue = 1;

	// --- GRAFICA (Lazy Loaded tramite TSoftObjectPtr) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	FLinearColor IconTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<UStaticMesh> WorldStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<USkeletalMesh> EquippedSkeletalMesh;

	// --- STATISTICHE COMBATTIMENTO / UTILIZZO ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stats")
	float PrimaryStatValue = 0.0f; // Danno per armi, Difesa per armature, Cura per cibo

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockDamageReduction = 0.0f; // % riduzione danno quando si para con lo scudo (0.0 - 1.0)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stats", meta = (ClampMin = "0.0"))
	float MaxDurability = 0.0f; // 0 = Indistruttibile

	// Identificativo univoco
	FName CustomAssetName = NAME_None;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// --- REGISTRO OGGETTI 100% C++ ---
	static void RegisterCodeItems();
	static UcybmineItemData* FindCodeItem(const FPrimaryAssetId& ItemID);

private:
	static TMap<FPrimaryAssetId, TObjectPtr<UcybmineItemData>> CodeItemRegistry;
};