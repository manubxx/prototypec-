#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "cybmineCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class CYBMINE_API AcybmineCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AcybmineCharacter();

	// Telecamera pura in prima persona
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	// --- COMPONENTI ANTEPRIMA PERSONAGGIO (INVENTARIO) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	USpringArmComponent* PreviewSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	USceneCaptureComponent2D* PreviewCaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	UTextureRenderTarget2D* PreviewRenderTarget;

	// --- STATO INVENTARIO ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bIsInventoryOpen = false;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

	// Ruota l'anteprima del personaggio nell'inventario
	void RotateCharacterPreview(float DeltaYaw);

	// --- STATISTICHE PUBBLICHE ACCESSIBILI DALL'HUD ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthRatio() const { return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackCooldownProgress() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInvulnerable() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// --- MINECRAFT I-FRAMES (10 Tick = 0.5s) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float InvulnerabilityDuration = 0.5f;

	float LastDamageTakenTime;

	// --- MINECRAFT COMBAT ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackSpeed = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float HorizontalKnockback = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float VerticalKnockback = 320.0f;

	float LastAttackTime;

	// --- MOVIMENTO E SPRINT ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 560.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	bool bWDown;
	bool bSDown;
	bool bDDown;
	bool bADown;

	float LastWReleaseTime;
	const float DoubleTapWindow = 0.25f;

	void OnWPressed();
	void OnWReleased();
	void OnSPressed();
	void OnSReleased();
	void OnDPressed();
	void OnDReleased();
	void OnAPressed();
	void OnAReleased();

	void ToggleSprint();
	void LookUpDirect(float Value);
	void LookRightDirect(float Value);

public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoMove(float Forward, float Right);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoJumpEnd();

	void PerformAttack();
};