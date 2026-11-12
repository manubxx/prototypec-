#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "cybmineTypes.h"
#include "cybmineHUD.generated.h"

class AcybmineCharacter;
class UcybmineInventoryComponent;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class CYBMINE_API AcybmineHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawSolidRect(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void DrawBoxWithBorder(float X, float Y, float Width, float Height, const FLinearColor& FillColor, const FLinearColor& BorderColor, float BorderThickness = 2.0f);
	void DrawItemInSlot(float SlotX, float SlotY, float SlotSize, const FInventoryItem& Item);

	void DrawCrosshair(float CenterX, float CenterY);
	void DrawHitmarker(float CenterX, float CenterY, float Alpha);
	void DrawAttackCooldown(float CenterX, float CenterY, float CooldownProgress);
	void DrawHealthBar(float ScreenWidth, float ScreenHeight, float HealthRatio);
	void DrawHotbar(float CenterX, float ScreenHeight, UcybmineInventoryComponent* InvComp);
	void DrawInventory(float ScreenWidth, float ScreenHeight, AcybmineCharacter* MyChar);

private:
	// Cache per icone caricate asincronamente tramite FStreamableManager (zero stutter in UI)
	UPROPERTY(Transient)
	TMap<FSoftObjectPath, TObjectPtr<UTexture2D>> LoadedIconCache;

	TSet<FSoftObjectPath> PendingIconLoads;
};