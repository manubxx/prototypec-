#include "cybmineCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"

AcybmineCharacter::AcybmineCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Rotazione del Controller agganciata alla visuale FPS
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingFrictionFactor = 0.5f;
	GetCharacterMovement()->FallingLateralFriction = 0.3f;
	GetCharacterMovement()->AirControl = 0.4f;

	// Telecamera pura in prima persona posizionata ad altezza occhi
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Caricamento mesh ed animazioni in C++
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));

	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
	}

	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}

	// Posizionamento della mesh per avere braccia e corpo allineati
	GetMesh()->SetupAttachment(GetCapsuleComponent());
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	// Visibile al proprietario in prima persona
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->bCastHiddenShadow = true;

	// --- SETUP ANTEPRIMA 3D PER INVENTARIO ---
	PreviewSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PreviewSpringArm"));
	PreviewSpringArm->SetupAttachment(GetCapsuleComponent());
	PreviewSpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
	PreviewSpringArm->TargetArmLength = 210.0f;
	PreviewSpringArm->bDoCollisionTest = false;
	PreviewSpringArm->bUsePawnControlRotation = false;
	PreviewSpringArm->bInheritPitch = false;
	PreviewSpringArm->bInheritRoll = false;
	PreviewSpringArm->bInheritYaw = true;
	// 180 gradi guarda frontalmente il personaggio
	PreviewSpringArm->SetRelativeRotation(FRotator(-5.0f, 180.0f, 0.0f));

	PreviewCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCaptureComponent"));
	PreviewCaptureComponent->SetupAttachment(PreviewSpringArm, USpringArmComponent::SocketName);
	PreviewCaptureComponent->FOVAngle = 45.0f;
	// Isola SOLO questo personaggio (niente sfondo del mondo)
	PreviewCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	PreviewCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	// Attivo solo quando l'inventario è aperto per risparmiare prestazioni
	PreviewCaptureComponent->bCaptureEveryFrame = false;
	PreviewCaptureComponent->bCaptureOnMovement = false;

	bIsSprinting = false;
	bIsInventoryOpen = false;

	bWDown = false;
	bSDown = false;
	bDDown = false;
	bADown = false;

	LastAttackTime = -10.0f;
	LastDamageTakenTime = -10.0f;
	LastWReleaseTime = -10.0f;

	CurrentHealth = MaxHealth;
}

void AcybmineCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Creazione dinamica della RenderTarget per l'anteprima nell'inventario
	PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	PreviewRenderTarget->InitAutoFormat(256, 384);
	PreviewRenderTarget->ClearColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
	PreviewRenderTarget->UpdateResourceImmediate(true);

	if (PreviewCaptureComponent)
	{
		PreviewCaptureComponent->TextureTarget = PreviewRenderTarget;
		PreviewCaptureComponent->ShowOnlyActors.Add(this);
	}
}

void AcybmineCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSprinting && !bWDown)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}

	if (Controller != nullptr && !bIsInventoryOpen)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		float ForwardValue = 0.0f;
		if (bWDown) ForwardValue += 1.0f;
		if (bSDown) ForwardValue -= 1.0f;

		if (ForwardValue != 0.0f)
		{
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(ForwardDirection, ForwardValue);
		}

		float RightValue = 0.0f;
		if (bDDown) RightValue += 1.0f;
		if (bADown) RightValue -= 1.0f;

		if (RightValue != 0.0f)
		{
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			AddMovementInput(RightDirection, RightValue);
		}
	}
}

void AcybmineCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent) return;

	// Apertura / Chiusura Inventario (I apre/chiude, Esc e Tab chiudono)
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AcybmineCharacter::ToggleInventory);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AcybmineCharacter::CloseInventory);
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AcybmineCharacter::CloseInventory);

	// Attacco
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AcybmineCharacter::PerformAttack);

	// Salto
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AcybmineCharacter::DoJumpStart);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AcybmineCharacter::DoJumpEnd);

	// Sprint
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AcybmineCharacter::ToggleSprint);
	PlayerInputComponent->BindKey(EKeys::RightControl, IE_Pressed, this, &AcybmineCharacter::ToggleSprint);

	// Movimento WASD
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AcybmineCharacter::OnWPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &AcybmineCharacter::OnWReleased);

	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &AcybmineCharacter::OnSPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &AcybmineCharacter::OnSReleased);

	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AcybmineCharacter::OnDPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &AcybmineCharacter::OnDReleased);

	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &AcybmineCharacter::OnAPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &AcybmineCharacter::OnAReleased);

	// Visuale Mouse
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AcybmineCharacter::LookRightDirect);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AcybmineCharacter::LookUpDirect);
}

void AcybmineCharacter::ToggleInventory()
{
	bIsInventoryOpen = !bIsInventoryOpen;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		if (bIsInventoryOpen)
		{
			// Ferma eventuali movimenti in corso
			bWDown = bSDown = bADown = bDDown = false;
			bIsSprinting = false;
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

			// Mostra il cursore e sblocca il mouse sullo schermo
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);

			// Resetta la rotazione frontale dell'anteprima e avvia la cattura
			if (PreviewSpringArm)
			{
				PreviewSpringArm->SetRelativeRotation(FRotator(-5.0f, 180.0f, 0.0f));
			}
			if (PreviewCaptureComponent)
			{
				PreviewCaptureComponent->bCaptureEveryFrame = true;
			}
		}
		else
		{
			// Nasconde il cursore e torna al controllo FPS puro
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			if (PreviewCaptureComponent)
			{
				PreviewCaptureComponent->bCaptureEveryFrame = false;
			}
		}
	}
}

void AcybmineCharacter::CloseInventory()
{
	if (bIsInventoryOpen)
	{
		ToggleInventory();
	}
}

void AcybmineCharacter::RotateCharacterPreview(float DeltaYaw)
{
	if (PreviewSpringArm)
	{
		FRotator CurrentRot = PreviewSpringArm->GetRelativeRotation();
		CurrentRot.Yaw += DeltaYaw;
		PreviewSpringArm->SetRelativeRotation(CurrentRot);
	}
}

void AcybmineCharacter::OnWPressed()
{
	if (bIsInventoryOpen) return;
	bWDown = true;
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if ((CurrentTime - LastWReleaseTime) <= DoubleTapWindow)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AcybmineCharacter::OnWReleased()
{
	bWDown = false;
	LastWReleaseTime = GetWorld()->GetTimeSeconds();
}

void AcybmineCharacter::OnSPressed()
{
	if (bIsInventoryOpen) return;
	bSDown = true;
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void AcybmineCharacter::OnSReleased()
{
	bSDown = false;
}

void AcybmineCharacter::OnDPressed()
{
	if (bIsInventoryOpen) return;
	bDDown = true;
}

void AcybmineCharacter::OnDReleased()
{
	bDDown = false;
}

void AcybmineCharacter::OnAPressed()
{
	if (bIsInventoryOpen) return;
	bADown = true;
}

void AcybmineCharacter::OnAReleased()
{
	bADown = false;
}

void AcybmineCharacter::ToggleSprint()
{
	if (bIsInventoryOpen) return;
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else if (bWDown)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AcybmineCharacter::LookRightDirect(float Value)
{
	if (!bIsInventoryOpen)
	{
		AddControllerYawInput(Value);
	}
}

void AcybmineCharacter::LookUpDirect(float Value)
{
	if (!bIsInventoryOpen)
	{
		AddControllerPitchInput(-Value);
	}
}

void AcybmineCharacter::DoMove(float Forward, float Right)
{
	if (bIsInventoryOpen) return;
	bWDown = (Forward > 0.1f);
	bSDown = (Forward < -0.1f);
	bDDown = (Right > 0.1f);
	bADown = (Right < -0.1f);
}

void AcybmineCharacter::DoLook(float Yaw, float Pitch)
{
	if (bIsInventoryOpen) return;
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AcybmineCharacter::DoJumpStart()
{
	if (!bIsInventoryOpen)
	{
		Jump();
	}
}

void AcybmineCharacter::DoJumpEnd()
{
	StopJumping();
}

float AcybmineCharacter::GetAttackCooldownProgress() const
{
	if (AttackSpeed <= 0.0f) return 1.0f;

	const float TimeSinceLastAttack = GetWorld()->GetTimeSeconds() - LastAttackTime;
	const float TimeRequired = 1.0f / AttackSpeed;
	return FMath::Clamp(TimeSinceLastAttack / TimeRequired, 0.0f, 1.0f);
}

bool AcybmineCharacter::IsInvulnerable() const
{
	return (GetWorld()->GetTimeSeconds() - LastDamageTakenTime) < InvulnerabilityDuration;
}

float AcybmineCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsInvulnerable() || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();

	if (CurrentHealth <= 0.0f)
	{
		Destroy();
	}

	return ActualDamage;
}

void AcybmineCharacter::PerformAttack()
{
	// Non attaccare mentre clicchi nell'inventario!
	if (bIsInventoryOpen)
	{
		return;
	}

	const float CooldownProgress = GetAttackCooldownProgress();
	LastAttackTime = GetWorld()->GetTimeSeconds();

	const FVector TraceStart = FirstPersonCamera->GetComponentLocation();
	const FVector ForwardVector = FirstPersonCamera->GetForwardVector();
	const FVector TraceEnd = TraceStart + (ForwardVector * AttackRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Pawn,
		QueryParams
	);

	if (bHit && HitResult.GetActor())
	{
		AcybmineCharacter* Target = Cast<AcybmineCharacter>(HitResult.GetActor());

		if (Target && !Target->IsInvulnerable() && Target->CurrentHealth > 0.0f)
		{
			const float DamageMultiplier = 0.2f + 0.8f * (CooldownProgress * CooldownProgress);
			const float FinalDamage = BaseDamage * DamageMultiplier;

			FDamageEvent DamageEvent;
			Target->TakeDamage(FinalDamage, DamageEvent, GetController(), this);

			if (Target->IsValidLowLevel() && !Target->IsPendingKillPending())
			{
				FVector KnockbackDir = ForwardVector;
				KnockbackDir.Z = 0.0f;
				KnockbackDir.Normalize();

				const float EffectiveKnockback = HorizontalKnockback * FMath::Clamp(CooldownProgress, 0.6f, 1.0f);
				const float EffectiveVertical = VerticalKnockback * FMath::Clamp(CooldownProgress, 0.6f, 1.0f);

				FVector TargetVelocity = Target->GetCharacterMovement()->Velocity;
				TargetVelocity.Z = 0.0f;

				FVector FinalVelocity = TargetVelocity + (KnockbackDir * EffectiveKnockback) + FVector(0.0f, 0.0f, EffectiveVertical);

				Target->LaunchCharacter(FinalVelocity, true, true);
			}

			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 15.0f, 12, FColor::Green, false, 0.5f);
		}
	}
	else
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.2f);
	}
}