#include "cybmineCombatComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

UcybmineCombatComponent::UcybmineCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UcybmineCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

float UcybmineCombatComponent::GetAttackCooldownProgress() const
{
	if (AttackSpeed <= 0.0f || !GetWorld()) return 1.0f;

	const float TimeSinceLastAttack = GetWorld()->GetTimeSeconds() - LastAttackTime;
	const float TimeRequired = 1.0f / AttackSpeed;
	return FMath::Clamp(TimeSinceLastAttack / TimeRequired, 0.0f, 1.0f);
}

bool UcybmineCombatComponent::IsInvulnerable() const
{
	if (!GetWorld()) return false;
	return (GetWorld()->GetTimeSeconds() - LastDamageTakenTime) < InvulnerabilityDuration;
}

float UcybmineCombatComponent::GetTimeSinceLastHitConfirm() const
{
	if (!GetWorld()) return 999.0f;
	return GetWorld()->GetTimeSeconds() - LastHitConfirmTime;
}

float UcybmineCombatComponent::HandleIncomingDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsInvulnerable() || CurrentHealth <= 0.0f || !GetWorld())
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();

	if (CurrentHealth <= 0.0f && GetOwner())
	{
		GetOwner()->Destroy();
	}

	return DamageAmount;
}

void UcybmineCombatComponent::PerformAttack(UCameraComponent* AimCamera)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !AimCamera || !GetWorld())
	{
		return;
	}

	const float CooldownProgress = GetAttackCooldownProgress();
	LastAttackTime = GetWorld()->GetTimeSeconds();

	const FVector TraceStart = AimCamera->GetComponentLocation();
	const FVector ForwardVector = AimCamera->GetForwardVector();
	const FVector TraceEnd = TraceStart + (ForwardVector * AttackRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Pawn,
		QueryParams
	);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		UcybmineCombatComponent* TargetCombat = HitActor->FindComponentByClass<UcybmineCombatComponent>();

		if (TargetCombat && !TargetCombat->IsInvulnerable() && TargetCombat->CurrentHealth > 0.0f)
		{
			LastHitConfirmTime = GetWorld()->GetTimeSeconds();

			const float DamageMultiplier = 0.2f + 0.8f * (CooldownProgress * CooldownProgress);
			const float FinalDamage = BaseDamage * DamageMultiplier;

			APawn* OwnerPawn = Cast<APawn>(OwnerActor);
			AController* InstigatorCtrl = OwnerPawn ? OwnerPawn->GetController() : nullptr;

			FDamageEvent DamageEvent;
			HitActor->TakeDamage(FinalDamage, DamageEvent, InstigatorCtrl, OwnerActor);

			ACharacter* TargetChar = Cast<ACharacter>(HitActor);
			if (TargetChar && TargetChar->IsValidLowLevel() && !TargetChar->IsPendingKillPending())
			{
				FVector KnockbackDir = ForwardVector;
				KnockbackDir.Z = 0.0f;
				KnockbackDir.Normalize();

				const float EffectiveKnockback = HorizontalKnockback * FMath::Clamp(CooldownProgress, 0.6f, 1.0f);
				const float EffectiveVertical = VerticalKnockback * FMath::Clamp(CooldownProgress, 0.6f, 1.0f);

				FVector TargetVelocity = TargetChar->GetCharacterMovement()->Velocity;
				TargetVelocity.Z = 0.0f;

				FVector FinalVelocity = TargetVelocity + (KnockbackDir * EffectiveKnockback) + FVector(0.0f, 0.0f, EffectiveVertical);
				TargetChar->LaunchCharacter(FinalVelocity, true, true);
			}

			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 15.0f, 12, FColor::Green, false, 0.5f);
		}
	}
	else
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.2f);
	}
}
