#include "cybmineHUD.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "cybmineCharacter.h"

void AcybmineHUD::DrawSolidRect(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	if (!Canvas || Width <= 0.0f || Height <= 0.0f)
	{
		return;
	}

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

void AcybmineHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float ScreenWidth = Canvas->ClipX;
	const float ScreenHeight = Canvas->ClipY;
	const float CenterX = ScreenWidth * 0.5f;
	const float CenterY = ScreenHeight * 0.5f;

	AcybmineCharacter* MyChar = Cast<AcybmineCharacter>(GetOwningPawn());

	if (MyChar && MyChar->bIsInventoryOpen)
	{
		// Quando l'inventario è aperto disegniamo la finestra dell'inventario
		DrawInventory(ScreenWidth, ScreenHeight, MyChar);
		return;
	}

	// Reset dello stato di trascinamento se l'inventario viene chiuso
	bIsDraggingPreview = false;

	// 1. Mirino centrale
	DrawCrosshair(CenterX, CenterY);

	if (MyChar)
	{
		// 2. Indicatore di ricarica colpo Minecraft
		const float Cooldown = MyChar->GetAttackCooldownProgress();
		if (Cooldown < 1.0f)
		{
			DrawAttackCooldown(CenterX, CenterY + 16.0f, Cooldown);
		}

		// 3. Barra della salute reale collegata al personaggio
		DrawHealthBar(ScreenWidth, ScreenHeight, MyChar->GetHealthRatio());
	}

	// 4. Hotbar
	DrawHotbar(CenterX, ScreenHeight);
}

void AcybmineHUD::DrawCrosshair(float CenterX, float CenterY)
{
	const FLinearColor CrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);
	const float Length = 8.0f;
	const float Thickness = 2.0f;

	DrawSolidRect(CenterX - Length, CenterY - (Thickness * 0.5f), Length * 2.0f, Thickness, CrosshairColor);
	DrawSolidRect(CenterX - (Thickness * 0.5f), CenterY - Length, Thickness, Length * 2.0f, CrosshairColor);
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

void AcybmineHUD::DrawHotbar(float CenterX, float ScreenHeight)
{
	const int32 SlotCount = 6;
	const float SlotSize = 44.0f;
	const float Spacing = 4.0f;
	const float TotalWidth = (SlotCount * SlotSize) + ((SlotCount - 1) * Spacing);
	const float StartX = CenterX - (TotalWidth * 0.5f);
	const float StartY = ScreenHeight - SlotSize - 15.0f;

	const FLinearColor SlotBg = FLinearColor(0.15f, 0.15f, 0.15f, 0.65f);
	const FLinearColor NormalBorder = FLinearColor(0.35f, 0.35f, 0.35f, 0.9f);
	const FLinearColor SelectedBorder = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const float CurrentX = StartX + i * (SlotSize + Spacing);
		const FLinearColor BorderCol = (i == 0) ? SelectedBorder : NormalBorder;
		const float BorderThick = (i == 0) ? 3.0f : 2.0f;

		DrawBoxWithBorder(CurrentX, StartY, SlotSize, SlotSize, SlotBg, BorderCol, BorderThick);
	}
}

void AcybmineHUD::DrawInventory(float ScreenWidth, float ScreenHeight, AcybmineCharacter* MyChar)
{
	// 1. Oscuramento sfondo schermo intero
	DrawSolidRect(0.0f, 0.0f, ScreenWidth, ScreenHeight, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));

	// Parametri griglia stile Minecraft (6 colonne allineate alla hotbar)
	const int32 Cols = 6;
	const int32 InvRows = 3;
	const float SlotSize = 48.0f;
	const float Spacing = 5.0f;
	const float Padding = 16.0f;

	const float GridWidth = (Cols * SlotSize) + ((Cols - 1) * Spacing);
	const float ArmorSectionHeight = (4 * SlotSize) + (3 * Spacing);
	const float InvGridHeight = (InvRows * SlotSize) + ((InvRows - 1) * Spacing);
	const float HotbarGap = 14.0f;

	const float PanelWidth = GridWidth + (Padding * 2.0f);
	const float PanelHeight = Padding + 26.0f + ArmorSectionHeight + 16.0f + InvGridHeight + HotbarGap + SlotSize + Padding;

	const float PanelX = (ScreenWidth - PanelWidth) * 0.5f;
	const float PanelY = (ScreenHeight - PanelHeight) * 0.5f;

	// Colori pannello
	const FLinearColor PanelBg = FLinearColor(0.18f, 0.18f, 0.18f, 0.95f);
	const FLinearColor PanelBorder = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);
	const FLinearColor SlotBg = FLinearColor(0.09f, 0.09f, 0.09f, 0.90f);
	const FLinearColor SlotBorder = FLinearColor(0.32f, 0.32f, 0.32f, 1.0f);
	const FLinearColor HoverBorder = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

	// Finestra principale
	DrawBoxWithBorder(PanelX, PanelY, PanelWidth, PanelHeight, PanelBg, PanelBorder, 3.0f);

	// Titolo finestra
	DrawText(TEXT("INVENTARIO"), FLinearColor(0.92f, 0.92f, 0.92f, 1.0f), PanelX + Padding, PanelY + 10.0f, nullptr, 1.1f);

	// Posizione cursore per evidenziare gli slot e gestire la rotazione 3D
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	bool bLeftMouseDown = false;
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->GetMousePosition(MouseX, MouseY);
		bLeftMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	}

	const float ContentStartX = PanelX + Padding;
	const float TopSectionY = PanelY + Padding + 26.0f;

	// 2. Colonna sinistra: 4 Slot Armatura (Elmo, Corazza, Gambali, Stivali)
	const TCHAR* ArmorLabels[4] = { TEXT("H"), TEXT("C"), TEXT("L"), TEXT("B") };
	for (int32 r = 0; r < 4; ++r)
	{
		const float SlotY = TopSectionY + r * (SlotSize + Spacing);
		const bool bHover = (MouseX >= ContentStartX && MouseX <= ContentStartX + SlotSize && MouseY >= SlotY && MouseY <= SlotY + SlotSize);

		DrawBoxWithBorder(ContentStartX, SlotY, SlotSize, SlotSize, SlotBg, bHover ? HoverBorder : SlotBorder, 2.0f);
		DrawText(ArmorLabels[r], FLinearColor(0.35f, 0.35f, 0.35f, 0.8f), ContentStartX + 18.0f, SlotY + 15.0f, nullptr, 1.0f);
	}

	// 3. Riquadro Anteprima 3D del Personaggio (accanto agli slot armatura)
	const float PreviewX = ContentStartX + SlotSize + 12.0f;
	const float PreviewY = TopSectionY;
	const float PreviewWidth = 140.0f;
	const float PreviewHeight = ArmorSectionHeight;

	DrawBoxWithBorder(PreviewX, PreviewY, PreviewWidth, PreviewHeight, FLinearColor(0.04f, 0.04f, 0.04f, 1.0f), SlotBorder, 2.0f);

	// Disegna la RenderTarget del personaggio dentro il riquadro
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

	// Gestione rotazione col trascinamento del mouse nel riquadro
	const bool bMouseInPreview = (MouseX >= PreviewX && MouseX <= PreviewX + PreviewWidth &&
		MouseY >= PreviewY && MouseY <= PreviewY + PreviewHeight);

	if (bLeftMouseDown)
	{
		if (!bIsDraggingPreview && bMouseInPreview)
		{
			bIsDraggingPreview = true;
			LastMouseX = MouseX;
		}
		else if (bIsDraggingPreview && MyChar)
		{
			const float DeltaX = MouseX - LastMouseX;
			MyChar->RotateCharacterPreview(DeltaX * 0.65f);
			LastMouseX = MouseX;
		}
	}
	else
	{
		bIsDraggingPreview = false;
	}

	// 4. Griglia Inventario Principale (3 righe x 6 colonne)
	const float InvStartY = TopSectionY + ArmorSectionHeight + 16.0f;
	for (int32 r = 0; r < InvRows; ++r)
	{
		for (int32 c = 0; c < Cols; ++c)
		{
			const float SlotX = ContentStartX + c * (SlotSize + Spacing);
			const float SlotY = InvStartY + r * (SlotSize + Spacing);
			const bool bHover = (MouseX >= SlotX && MouseX <= SlotX + SlotSize && MouseY >= SlotY && MouseY <= SlotY + SlotSize);

			DrawBoxWithBorder(SlotX, SlotY, SlotSize, SlotSize, SlotBg, bHover ? HoverBorder : SlotBorder, 2.0f);
		}
	}

	// 5. Riga Hotbar (1 riga x 6 colonne) separata leggermente in basso
	const float HotbarStartY = InvStartY + InvGridHeight + HotbarGap;
	for (int32 c = 0; c < Cols; ++c)
	{
		const float SlotX = ContentStartX + c * (SlotSize + Spacing);
		const bool bHover = (MouseX >= SlotX && MouseX <= SlotX + SlotSize && MouseY >= HotbarStartY && MouseY <= HotbarStartY + SlotSize);

		DrawBoxWithBorder(SlotX, HotbarStartY, SlotSize, SlotSize, SlotBg, bHover ? HoverBorder : SlotBorder, 2.0f);
	}
}