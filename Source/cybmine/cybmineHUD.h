#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "cybmineHUD.generated.h"

class AcybmineCharacter;

UCLASS(BlueprintType, Blueprintable)
class CYBMINE_API AcybmineHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawSolidRect(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void DrawBoxWithBorder(float X, float Y, float Width, float Height, const FLinearColor& FillColor, const FLinearColor& BorderColor, float BorderThickness = 2.0f);

	void DrawCrosshair(float CenterX, float CenterY);
	void DrawAttackCooldown(float CenterX, float CenterY, float CooldownProgress);
	void DrawHealthBar(float ScreenWidth, float ScreenHeight, float HealthRatio);
	void DrawHotbar(float CenterX, float ScreenHeight);
	void DrawInventory(float ScreenWidth, float ScreenHeight, AcybmineCharacter* MyChar);

private:
	// Variabili per ruotare il personaggio trascinando il mouse nel riquadro
	bool bIsDraggingPreview = false;
	float LastMouseX = 0.0f;
};