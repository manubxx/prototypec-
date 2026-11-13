#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "cybmineCombatComponent.generated.h"

class UCameraComponent;

// Enumerazione degli stati di combattimento
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Attacking	UMETA(DisplayName = "Attacking"),
	Blocking	UMETA(DisplayName = "Blocking"),
	Stunned		UMETA(DisplayName = "Stunned")
};

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

	// --- PARAMETRI PARATA ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Blocking")
	float BlockAngleThreshold = 0.5f; // ~60 gradi frontali

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Blocking")
	float StaminaBlockCost = 15.0f;

	// --- AZIONI & STATO ---
	void PerformAttack(UCameraComponent* AimCamera);
	float HandleIncomingDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackCooldownProgress() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInvulnerable() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTimeSinceLastHitConfirm() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	ECombatState GetCombatState() const { return CombatState; }

	// RPC per comunicare al server l'intenzione di parare
	UFUNCTION(Server, Reliable)
	void Server_SetBlocking(bool bIsBlocking);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState, BlueprintReadOnly, Category = "Combat|State")
	ECombatState CombatState;

	UFUNCTION()
	void OnRep_CombatState();

private:
	float LastAttackTime = -10.0f;
	float LastDamageTakenTime = -10.0f;
	float LastHitConfirmTime = -10.0f;
};