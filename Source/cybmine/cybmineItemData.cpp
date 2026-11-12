#include "cybmineItemData.h"

TMap<FPrimaryAssetId, TObjectPtr<UcybmineItemData>> UcybmineItemData::CodeItemRegistry;

FPrimaryAssetId UcybmineItemData::GetPrimaryAssetId() const
{
	const FName AssetName = !CustomAssetName.IsNone() ? CustomAssetName : GetFName();
	return FPrimaryAssetId(FPrimaryAssetType("ItemData"), AssetName);
}

void UcybmineItemData::RegisterCodeItems()
{
	if (CodeItemRegistry.Num() > 0) return;

	auto CreateItem = [](FName IDName, const FString& Name, EItemCategory Cat, int32 MaxStack, int32 Gold, FLinearColor Tint, float Stat, float BlockRed, float Durability, const TCHAR* MeshPath = nullptr)
		{
			UcybmineItemData* Item = NewObject<UcybmineItemData>(GetTransientPackage(), IDName);
			Item->AddToRoot(); // Evita che il Garbage Collector lo elimini
			Item->CustomAssetName = IDName;
			Item->DisplayName = FText::FromString(Name);
			Item->Category = Cat;
			Item->MaxStackSize = MaxStack;
			Item->BaseGoldValue = Gold;
			Item->IconTint = Tint;
			Item->PrimaryStatValue = Stat;
			Item->BlockDamageReduction = BlockRed;
			Item->MaxDurability = Durability;

			if (MeshPath)
			{
				Item->WorldStaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
			}

			CodeItemRegistry.Add(Item->GetPrimaryAssetId(), Item);
		};

	// --- DEFINIZIONE OGGETTI DEL GIOCO (100% C++) ---
	CreateItem(
		FName("DA_SpadaFerro"),
		TEXT("Spada di Ferro"),
		EItemCategory::Weapon,
		1,      // MaxStackSize
		50,     // Valore Oro
		FLinearColor(0.75f, 0.78f, 0.82f, 1.0f), // Colore Icona Grigio Ferro
		25.0f,  // Danno
		0.0f,   // Riduzione Parata
		250.0f, // Durabilità
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);

	CreateItem(
		FName("DA_ScudoLegno"),
		TEXT("Scudo Rinforzato"),
		EItemCategory::Shield,
		1,      // MaxStackSize
		35,     // Valore Oro
		FLinearColor(0.55f, 0.35f, 0.15f, 1.0f), // Colore Icona Marrone Legno
		0.0f,   // Danno
		0.75f,  // Parata riduce il 75% del danno
		180.0f, // Durabilità
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
	);

	CreateItem(
		FName("DA_BloccoPietra"),
		TEXT("Pietra"),
		EItemCategory::Block,
		64,     // MaxStackSize
		2,      // Valore Oro
		FLinearColor(0.45f, 0.45f, 0.45f, 1.0f),
		0.0f,
		0.0f,
		0.0f,   // Indistruttibile
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
}

UcybmineItemData* UcybmineItemData::FindCodeItem(const FPrimaryAssetId& ItemID)
{
	if (CodeItemRegistry.Num() == 0)
	{
		RegisterCodeItems();
	}

	if (TObjectPtr<UcybmineItemData>* Found = CodeItemRegistry.Find(ItemID))
	{
		return Found->Get();
	}
	return nullptr;
}