#include "cybmineHUD.h"
#include "cybmineCharacter.h"
#include "cybmineInventoryComponent.h"
#include "cybmineCombatComponent.h"
#include "cybmineItemData.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/AssetManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"

void AcybmineHUD::DrawSolidRect(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	if (!Canvas || Width <= 0.0f || Height <= 0.0f) return;

	FCanvasTileItem TileItem(FVector2D(X, Y), FVector2D(Width, Height), Color);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}

void AcybmineHUD::DrawBoxWithBorder(float X, float Y, float Width, float Height, const FLinearColor& FillColor, const FLinearColor& BorderColor, float BorderThickness)
{
	DrawSolidRect(X, Y, Width, Height, FillColor);
	DrawSolidRect(X, Y, Width, BorderThickness, BorderColor);
	DrawSolidRect(X, Y + Height - BorderThickness, Width, BorderThickness, BorderColor);
	DrawSolidRect(X, Y, BorderThickness, Height, BorderColor);
	DrawSolidRect(X + Width - BorderThickness, Y, BorderThickness, Height, BorderColor);
}

void AcybmineHUD::DrawItemInSlot(float SlotX, float SlotY, float SlotSize, const FInventoryItem& Item)
{
	if (Item.IsEmpty()) return;

	UcybmineItemData* StaticData = UcybmineInventoryComponent::GetItemData(Item.ItemID);
	if (!StaticData)
	{
		// Se il Data Asset è su disco e non ancora caricato, avvia caricamento asincrono senza bloccare
		UcybmineInventoryComponent::GetItemDataAsync(Item.ItemID, FOnItemDataLoaded());
		return;
	}

	const FLinearColor TintColor = StaticData->IconTint;
	UTexture2D* IconTex = nullptr;

	if (!StaticData->Icon.IsNull())
	{
		const FSoftObjectPath IconPath = StaticData->Icon.ToSoftObjectPath();
		if (TObjectPtr<UTexture2D>* Cached = LoadedIconCache.Find(IconPath))
		{
			IconTex = Cached->Get();
		}
		else if (UTexture2D* AlreadyInMemory = StaticData->Icon.Get())
		{
			LoadedIconCache.Add(IconPath, AlreadyInMemory);
			IconTex = AlreadyInMemory;
		}
		else if (!PendingIconLoads.Contains(IconPath))
		{
			PendingIconLoads.Add(IconPath);
			FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
			Streamable.RequestAsyncLoad(IconPath, FStreamableDelegate::CreateWeakLambda(this, [this, IconPath]()
				{
					PendingIconLoads.Remove(IconPath);
					if (UTexture2D* LoadedTex = Cast<UTexture2D>(IconPath.ResolveObject()))
					{
						LoadedIconCache.Add(IconPath, LoadedTex);
					}
				}));
		}
	}

	const float Pad = 8.0f;
	const float IconSize = SlotSize - (Pad * 2.0f);

	if (IconTex && IconTex->GetResource())
	{
		FCanvasTileItem IconTile(
			FVector2D(SlotX + Pad, SlotY + Pad),
			IconTex->GetResource(),
			FVector2D(IconSize, IconSize),
			TintColor
		);
		IconTile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(IconTile);
	}
	else
	{
		// Segnaposto colorato dal Data Asset mentre la texture carica asincronamente o se non assegnata
		DrawSolidRect(SlotX + Pad, SlotY + Pad, IconSize, IconSize, TintColor);
	}

	// Barra della durabilità
	if (StaticData->MaxDurability > 0.0f && Item.CurrentDurability < StaticData->MaxDurability)
	{
		const float DurRatio = FMath::Clamp(Item.CurrentDurability / StaticData->MaxDurability, 0.0f, 1.0f);
		const float BarWidth = SlotSize - 10.0f;
		const float BarHeight = 3.0f;
		const float BarX = SlotX + 5.0f;
		const float BarY = SlotY + SlotSize - 6.0f;

		const FLinearColor DurColor = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Green, DurRatio);
		DrawSolidRect(BarX, BarY, BarWidth, BarHeight, FLinearColor(0.05f, 0.05f, 0.05f, 0.9f));
		DrawSolidRect(BarX, BarY, BarWidth * DurRatio, BarHeight, DurColor);
	}

	if (Item.Quantity > 1)
	{
		const FString QtyText = FString::FromInt(Item.Quantity);
		DrawText(QtyText, FLinearColor::White, SlotX + SlotSize - 18.0f, SlotY + SlotSize - 16.0f, nullptr, 0.85f);
	}
}

void AcybmineHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float ScreenWidth = Canvas->ClipX;
	const float ScreenHeight = Canvas->ClipY;
	const float CenterX = ScreenWidth * 0.5f;
	const float CenterY = ScreenHeight * 0.5f;

	AcybmineCharacter* MyChar = Cast<AcybmineCharacter>(GetOwningPawn());
	UcybmineInventoryComponent* InvComp = MyChar ? MyChar->InventoryComponent : nullptr;
	UcybmineCombatComponent* CombatComp = MyChar ? MyChar->CombatComponent : nullptr;

	if (InvComp && InvComp->bIsInventoryOpen)
	{
		DrawInventory(ScreenWidth, ScreenHeight, MyChar);
		return;
	}

	DrawCrosshair(CenterX, CenterY);

	if (CombatComp)
	{
		const float HitAge = CombatComp->GetTimeSinceLastHitConfirm();
		const float HitDuration = 0.18f;
		if (HitAge >= 0.0f && HitAge < HitDuration)
		{
			DrawHitmarker(CenterX, CenterY, 1.0f - (HitAge / HitDuration));
		}

		const float Cooldown = CombatComp->GetAttackCooldownProgress();
		if (Cooldown < 1.0f)
		{
			DrawAttackCooldown(CenterX, CenterY + 16.0f, Cooldown);
		}

		DrawHealthBar(ScreenWidth, ScreenHeight, CombatComp->GetHealthRatio());
	}

	DrawHotbar(CenterX, ScreenHeight, InvComp);
}

void AcybmineHUD::DrawCrosshair(float CenterX, float CenterY)
{
	const FLinearColor CrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);
	const float Length = 8.0f;
	const float Thickness = 2.0f;

	DrawSolidRect(CenterX - Length, CenterY - (Thickness * 0.5f), Length * 2.0f, Thickness, CrosshairColor);
	DrawSolidRect(CenterX - (Thickness * 0.5f), CenterY - Length, Thickness, Length * 2.0f, CrosshairColor);
}

void AcybmineHUD::DrawHitmarker(float CenterX, float CenterY, float Alpha)
{
	const FLinearColor HitColor = FLinearColor(1.0f, 1.0f, 1.0f, FMath::Clamp(Alpha, 0.0f, 1.0f));
	const float InnerOffset = 5.0f;
	const float OuterOffset = 13.0f;
	const float LineThickness = 2.0f;

	FCanvasLineItem TopLeft(FVector2D(CenterX - OuterOffset, CenterY - OuterOffset), FVector2D(CenterX - InnerOffset, CenterY - InnerOffset));
	TopLeft.SetColor(HitColor);
	TopLeft.LineThickness = LineThickness;
	Canvas->DrawItem(TopLeft);

	FCanvasLineItem TopRight(FVector2D(CenterX + OuterOffset, CenterY - OuterOffset), FVector2D(CenterX + InnerOffset, CenterY - InnerOffset));
	TopRight.SetColor(HitColor);
	TopRight.LineThickness = LineThickness;
	Canvas->DrawItem(TopRight);

	FCanvasLineItem BottomLeft(FVector2D(CenterX - OuterOffset, CenterY + OuterOffset), FVector2D(CenterX - InnerOffset, CenterY + InnerOffset));
	BottomLeft.SetColor(HitColor);
	BottomLeft.LineThickness = LineThickness;
	Canvas->DrawItem(BottomLeft);

	FCanvasLineItem BottomRight(FVector2D(CenterX + OuterOffset, CenterY + OuterOffset), FVector2D(CenterX + InnerOffset, CenterY + InnerOffset));
	BottomRight.SetColor(HitColor);
	BottomRight.LineThickness = LineThickness;
	Canvas->DrawItem(BottomRight);
}

void AcybmineHUD::DrawAttackCooldown(float CenterX, float CenterY, float CooldownProgress)
{
	const float BarWidth = 24.0f;
	const float BarHeight = 4.0f;
	const float StartX = CenterX - (BarWidth * 0.5f);

	DrawSolidRect(StartX, CenterY, BarWidth, BarHeight, FLinearColor(0.1f, 0.1f, 0.1f, 0.7f));
	DrawSolidRect(StartX, CenterY, BarWidth * FMath::Clamp(CooldownProgress, 0.0f, 1.0f), BarHeight, FLinearColor(1.0f, 1.0f, 1.0f, 0.9f));
}

void AcybmineHUD::DrawHealthBar(float ScreenWidth, float ScreenHeight, float HealthRatio)
{
	const float BarWidth = 180.0f;
	const float BarHeight = 14.0f;
	const float StartX = (ScreenWidth * 0.5f) - 150.0f;
	const float StartY = ScreenHeight - 75.0f;

	DrawBoxWithBorder(StartX, StartY, BarWidth, BarHeight, FLinearColor(0.05f, 0.05f, 0.05f, 0.8f), FLinearColor(0.2f, 0.2f, 0.2f, 1.0f), 2.0f);

	const float ClampedRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);
	if (ClampedRatio > 0.0f)
	{
		DrawSolidRect(StartX + 2.0f, StartY + 2.0f, (BarWidth - 4.0f) * ClampedRatio, BarHeight - 4.0f, FLinearColor(0.85f, 0.1f, 0.1f, 0.95f));
	}
}

void AcybmineHUD::DrawHotbar(float CenterX, float ScreenHeight, UcybmineInventoryComponent* InvComp)
{
	const int32 SlotCount = CybmineConstants::HotbarSlotCount;
	const int32 SelectedSlot = InvComp ? InvComp->SelectedHotbarSlot : 0;
	const float SlotSize = 44.0f;
	const float Spacing = 4.0f;
	const float TotalWidth = (SlotCount * SlotSize) + ((SlotCount - 1) * Spacing);
	const float StartX = CenterX - (TotalWidth * 0.5f);
	const float StartY = ScreenHeight - SlotSize - 15.0f;

	const FLinearColor SlotBg = FLinearColor(0.15f, 0.15f, 0.15f, 0.65f);
	const FLinearColor SelectedBg = FLinearColor(0.28f, 0.28f, 0.28f, 0.80f);
	const FLinearColor NormalBorder = FLinearColor(0.35f, 0.35f, 0.35f, 0.9f);
	const FLinearColor SelectedBorder = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const bool bIsSelected = (i == SelectedSlot);
		const float CurrentX = StartX + i * (SlotSize + Spacing);
		const float CurrentY = bIsSelected ? (StartY - 3.0f) : StartY;

		DrawBoxWithBorder(CurrentX, CurrentY, SlotSize, SlotSize, bIsSelected ? SelectedBg : SlotBg, bIsSelected ? SelectedBorder : NormalBorder, bIsSelected ? 3.0f : 2.0f);

		if (InvComp && InvComp->StorageSlots.IsValidIndex(i))
		{
			DrawItemInSlot(CurrentX, CurrentY, SlotSize, InvComp->StorageSlots[i]);
		}

		DrawText(FString::FromInt(i + 1), bIsSelected ? FLinearColor::White : FLinearColor(0.65f, 0.65f, 0.65f, 0.8f), CurrentX + 5.0f, CurrentY + 3.0f, nullptr, 0.85f);
	}
}

void AcybmineHUD::DrawInventory(float ScreenWidth, float ScreenHeight, AcybmineCharacter* MyChar)
{
	DrawSolidRect(0.0f, 0.0f, ScreenWidth, ScreenHeight, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));

	UcybmineInventoryComponent* InvComp = MyChar ? MyChar->InventoryComponent : nullptr;

	const int32 Cols = CybmineConstants::InventoryCols;
	const int32 InvRows = CybmineConstants::InventoryRows;
	const float SlotSize = 48.0f;
	const float Spacing = 5.0f;
	const float Padding = 16.0f;

	const float GridWidth = (Cols * SlotSize) + ((Cols - 1) * Spacing);
	const float ArmorSectionHeight = (CybmineConstants::ArmorSlotCount * SlotSize) + ((CybmineConstants::ArmorSlotCount - 1) * Spacing);
	const float InvGridHeight = (InvRows * SlotSize) + ((InvRows - 1) * Spacing);
	const float HotbarGap = 14.0f;

	const float PanelWidth = GridWidth + (Padding * 2.0f);
	const float PanelHeight = Padding + 26.0f + ArmorSectionHeight + 16.0f + InvGridHeight + HotbarGap + SlotSize + Padding;

	const float PanelX = (ScreenWidth - PanelWidth) * 0.5f;
	const float PanelY = (ScreenHeight - PanelHeight) * 0.5f;

	const FLinearColor PanelBg = FLinearColor(0.18f, 0.18f, 0.18f, 0.95f);
	const FLinearColor PanelBorder = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);
	const FLinearColor SlotBg = FLinearColor(0.09f, 0.09f, 0.09f, 0.90f);
	const FLinearColor SlotBorder = FLinearColor(0.32f, 0.32f, 0.32f, 1.0f);
	const FLinearColor HoverBorder = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

	DrawBoxWithBorder(PanelX, PanelY, PanelWidth, PanelHeight, PanelBg, PanelBorder, 3.0f);
	DrawText(TEXT("INVENTARIO"), FLinearColor(0.92f, 0.92f, 0.92f, 1.0f), PanelX + Padding, PanelY + 10.0f, nullptr, 1.1f);

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->GetMousePosition(MouseX, MouseY);
	}

	const float ContentStartX = PanelX + Padding;
	const float TopSectionY = PanelY + Padding + 26.0f;

	// 1. Slot Armatura (0..3)
	const TCHAR* ArmorLabels[4] = { TEXT("H"), TEXT("C"), TEXT("L"), TEXT("B") };
	for (int32 r = 0; r < CybmineConstants::ArmorSlotCount; ++r)
	{
		const float SlotY = TopSectionY + r * (SlotSize + Spacing);
		const bool bHover = (MouseX >= ContentStartX && MouseX <= ContentStartX + SlotSize && MouseY >= SlotY && MouseY <= SlotY + SlotSize);

		DrawBoxWithBorder(ContentStartX, SlotY, SlotSize, SlotSize, SlotBg, bHover ? HoverBorder : SlotBorder, 2.0f);
		if (InvComp && InvComp->ArmorSlots.IsValidIndex(r) && !InvComp->ArmorSlots[r].IsEmpty())
		{
			DrawItemInSlot(ContentStartX, SlotY, SlotSize, InvComp->ArmorSlots[r]);
		}
		else
		{
			DrawText(ArmorLabels[r], FLinearColor(0.35f, 0.35f, 0.35f, 0.8f), ContentStartX + 18.0f, SlotY + 15.0f, nullptr, 1.0f);
		}
	}

	// 2. Riquadro Anteprima 3D del Personaggio
	const float PreviewX = ContentStartX + SlotSize + 12.0f;
	const float PreviewY = TopSectionY;
	const float PreviewWidth = 140.0f;
	const float PreviewHeight = ArmorSectionHeight;

	DrawBoxWithBorder(PreviewX, PreviewY, PreviewWidth, PreviewHeight, FLinearColor(0.04f, 0.04f, 0.04f, 1.0f), SlotBorder, 2.0f);

	if (MyChar && MyChar->PreviewRenderTarget)
	{
		FCanvasTileItem PreviewTile(
			FVector2D(PreviewX + 2.0f, PreviewY + 2.0f),
			MyChar->PreviewRenderTarget->GetResource(),
			FVector2D(PreviewWidth - 4.0f, PreviewHeight - 4.0f),
			FLinearColor::White
		);
		PreviewTile.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(PreviewTile);
	}

	// 3. Zaino Principale (indici 6..23)
	const float InvStartY = TopSectionY + ArmorSectionHeight + 16.0f;
	for (int32 r = 0; r < InvRows; ++r)
	{
		for (int32 c = 0; c < Cols; ++c)
		{
			const int32 StorageIdx = CybmineConstants::HotbarSlotCount + (r * Cols) + c;
			const float SlotX = ContentStartX + c * (SlotSize + Spacing);
			const float SlotY = InvStartY + r * (SlotSize + Spacing);
			const bool bHover = (MouseX >= SlotX && MouseX <= SlotX + SlotSize && MouseY >= SlotY && MouseY <= SlotY + SlotSize);

			DrawBoxWithBorder(SlotX, SlotY, SlotSize, SlotSize, SlotBg, bHover ? HoverBorder : SlotBorder, 2.0f);
			if (InvComp && InvComp->StorageSlots.IsValidIndex(StorageIdx))
			{
				DrawItemInSlot(SlotX, SlotY, SlotSize, InvComp->StorageSlots[StorageIdx]);
			}
		}
	}

	// 4. Hotbar sincronizzata (indici 0..5)
	const float HotbarStartY = InvStartY + InvGridHeight + HotbarGap;
	const int32 ActiveSlot = InvComp ? InvComp->SelectedHotbarSlot : 0;
	for (int32 c = 0; c < Cols; ++c)
	{
		const float SlotX = ContentStartX + c * (SlotSize + Spacing);
		const bool bHover = (MouseX >= SlotX && MouseX <= SlotX + SlotSize && MouseY >= HotbarStartY && MouseY <= HotbarStartY + SlotSize);
		const bool bSelected = (c == ActiveSlot);

		DrawBoxWithBorder(SlotX, HotbarStartY, SlotSize, SlotSize, SlotBg, (bHover || bSelected) ? HoverBorder : SlotBorder, bSelected ? 3.0f : 2.0f);
		if (InvComp && InvComp->StorageSlots.IsValidIndex(c))
		{
			DrawItemInSlot(SlotX, HotbarStartY, SlotSize, InvComp->StorageSlots[c]);
		}
	}
}