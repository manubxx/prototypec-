#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "cybmineCombatComponent.generated.h"

class UCameraComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CYBMINE_API UcybmineCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UcybmineCombatComponent();

	// --- STATISTICHE SALUTE ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth = 20.0f;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthRatio() const { return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f; }

	// --- PARAMETRI COMBATTIMENTO ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float InvulnerabilityDuration = 0.5f;

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

	// --- AZIONI & STATO ---
	void PerformAttack(UCameraComponent* AimCamera);
	float HandleIncomingDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackCooldownProgress() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInvulnerable() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTimeSinceLastHitConfirm() const;

protected:
	virtual void BeginPlay() override;

private:
	float LastAttackTime = -10.0f;
	float LastDamageTakenTime = -10.0f;
	float LastHitConfirmTime = -10.0f;
};
