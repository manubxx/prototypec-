#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "cybmineCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UPointLightComponent;
class UcybmineInventoryComponent;
class UcybmineCombatComponent;

UCLASS()
class CYBMINE_API AcybmineCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AcybmineCharacter();

	// --- COMPONENTI PRINCIPALI ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UcybmineInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UcybmineCombatComponent* CombatComponent;

	// --- ASPETTO AVATAR ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor AvatarTintColor = FLinearColor(0.05f, 0.25f, 0.95f, 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void ApplyAvatarColor(const FLinearColor& NewColor);

	// --- TELECAMERA & ANTEPRIMA 3D INVENTARIO ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	USpringArmComponent* PreviewSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	USceneCaptureComponent2D* PreviewCaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	UPointLightComponent* PreviewLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryPreview")
	UTextureRenderTarget2D* PreviewRenderTarget;

	// --- CONTROLLO INVENTARIO ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

	bool IsInventoryOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void AutoAssignAnimBlueprint();

	// --- ANIMAZIONI DIRETTE 100% C++ ---
	UPROPERTY()
	class UAnimSequence* AnimIdle = nullptr;

	UPROPERTY()
	class UAnimSequence* AnimWalk = nullptr;

	UPROPERTY()
	class UAnimSequence* AnimRun = nullptr;

	UPROPERTY()
	class UAnimSequence* AnimJump = nullptr;

	UPROPERTY()
	class UAnimSequence* CurrentPlayingAnim = nullptr;

	void UpdateCharacterAnimation();



	// --- MOVIMENTO E SPRINT ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 560.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	bool bWDown = false;
	bool bSDown = false;
	bool bDDown = false;
	bool bADown = false;

	float LastWReleaseTime = -10.0f;
	const float DoubleTapWindow = 0.25f;
	float PunchAnimAlpha = 0.0f;

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

	void SelectSlot1();
	void SelectSlot2();
	void SelectSlot3();
	void SelectSlot4();
	void SelectSlot5();
	void SelectSlot6();
	void NextHotbarSlot();
	void PrevHotbarSlot();

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
