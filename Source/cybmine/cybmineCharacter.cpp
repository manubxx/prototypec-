#include "cybmineCharacter.h"
#include "cybmineInventoryComponent.h"
#include "cybmineCombatComponent.h"
#include "cybmineItemData.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/AssetManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimSequence.h"

AcybmineCharacter::AcybmineCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingFrictionFactor = 0.5f;
	GetCharacterMovement()->FallingLateralFriction = 0.3f;
	GetCharacterMovement()->AirControl = 0.4f;

	// Componenti modulari
	InventoryComponent = CreateDefaultSubobject<UcybmineInventoryComponent>(TEXT("InventoryComponent"));
	CombatComponent = CreateDefaultSubobject<UcybmineCombatComponent>(TEXT("CombatComponent"));

	// Telecamera FPS
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(16.0f, 0.0f, 66.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Mesh e Animazioni
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannySimpleMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EngineFallbackMesh(TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));

	if (MannyMesh.Succeeded()) GetMesh()->SetSkeletalMesh(MannyMesh.Object);
	else if (MannySimpleMesh.Succeeded()) GetMesh()->SetSkeletalMesh(MannySimpleMesh.Object);
	else if (QuinnMesh.Succeeded()) GetMesh()->SetSkeletalMesh(QuinnMesh.Object);
	else if (EngineFallbackMesh.Succeeded()) GetMesh()->SetSkeletalMesh(EngineFallbackMesh.Object);

	static ConstructorHelpers::FClassFinder<UAnimInstance> MannySubAnim(TEXT("/Game/Characters/Mannequins/Animations/Manny/ABP_Manny.ABP_Manny_C"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(TEXT("/Game/Characters/Mannequins/Animations/ABP_Unarmed.ABP_Unarmed_C"));

	if (MannySubAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(MannySubAnim.Class);
	else if (MannyAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	else if (UnarmedAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(UnarmedAnim.Class);

	GetMesh()->SetupAttachment(GetCapsuleComponent());
	GetMesh()->SetRelativeLocation(FVector(-6.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->bCastHiddenShadow = true;

	// Mesh 3D oggetto impugnato (attaccata alla mano destra del manichino)
	EquippedItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedItemMesh"));
	EquippedItemMesh->SetupAttachment(GetMesh(), FName("hand_r"));
	EquippedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedItemMesh->SetRelativeScale3D(FVector(0.25f));
	EquippedItemMesh->SetVisibility(false);

	// Anteprima 3D per inventario
	PreviewSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PreviewSpringArm"));
	PreviewSpringArm->SetupAttachment(GetCapsuleComponent());
	PreviewSpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
	PreviewSpringArm->TargetArmLength = 220.0f;
	PreviewSpringArm->bDoCollisionTest = false;
	PreviewSpringArm->bUsePawnControlRotation = false;
	PreviewSpringArm->bInheritPitch = false;
	PreviewSpringArm->bInheritRoll = false;
	PreviewSpringArm->bInheritYaw = true;
	PreviewSpringArm->SetRelativeRotation(FRotator(-5.0f, 180.0f, 0.0f));

	PreviewCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCaptureComponent"));
	PreviewCaptureComponent->SetupAttachment(PreviewSpringArm, USpringArmComponent::SocketName);
	PreviewCaptureComponent->FOVAngle = 45.0f;
	PreviewCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	PreviewCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	PreviewCaptureComponent->bCaptureEveryFrame = false;
	PreviewCaptureComponent->bCaptureOnMovement = false;

	PreviewLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PreviewLight"));
	PreviewLight->SetupAttachment(PreviewSpringArm, USpringArmComponent::SocketName);
	PreviewLight->SetRelativeLocation(FVector(40.0f, 40.0f, 50.0f));
	PreviewLight->SetIntensity(5000.0f);
	PreviewLight->SetAttenuationRadius(500.0f);
	PreviewLight->SetCastShadows(false);
	PreviewLight->SetVisibility(false);
}

void AcybmineCharacter::AutoAssignAnimBlueprint()
{
	if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName("/Game"));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	USkeleton* TargetSkeleton = GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();

	for (const FAssetData& Asset : AssetList)
	{
		if (UAnimSequence* Seq = Cast<UAnimSequence>(Asset.GetAsset()))
		{
			if (TargetSkeleton && Seq->GetSkeleton() != TargetSkeleton)
			{
				continue;
			}

			const FString Name = Asset.AssetName.ToString();
			if (!AnimIdle && Name.Contains(TEXT("Idle")) && !Name.Contains(TEXT("Combat")))
			{
				AnimIdle = Seq;
			}
			else if (!AnimWalk && Name.Contains(TEXT("Walk")) && (Name.Contains(TEXT("Fwd")) || Name.Contains(TEXT("Forward")) || !Name.Contains(TEXT("Bwd"))))
			{
				AnimWalk = Seq;
			}
			else if (!AnimRun && Name.Contains(TEXT("Run")) && (Name.Contains(TEXT("Fwd")) || Name.Contains(TEXT("Forward")) || !Name.Contains(TEXT("Bwd"))))
			{
				AnimRun = Seq;
			}
			else if (!AnimJump && (Name.Contains(TEXT("Jump")) || Name.Contains(TEXT("Fall"))))
			{
				AnimJump = Seq;
			}
		}
	}

	if (!AnimWalk) AnimWalk = AnimRun;
	if (!AnimRun) AnimRun = AnimWalk;

	if (AnimIdle)
	{
		GetMesh()->PlayAnimation(AnimIdle, true);
		CurrentPlayingAnim = AnimIdle;
	}
}

void AcybmineCharacter::UpdateCharacterAnimation()
{
	if (!GetMesh() || GetMesh()->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return;
	}

	UAnimSequence* DesiredAnim = AnimIdle;

	if (GetCharacterMovement()->IsFalling())
	{
		if (AnimJump) DesiredAnim = AnimJump;
	}
	else
	{
		const float Speed2D = GetVelocity().Size2D();
		if (Speed2D > 20.0f)
		{
			DesiredAnim = (bIsSprinting && AnimRun) ? AnimRun : AnimWalk;
		}
	}

	if (DesiredAnim && DesiredAnim != CurrentPlayingAnim)
	{
		const bool bLoop = (DesiredAnim != AnimJump);
		GetMesh()->PlayAnimation(DesiredAnim, bLoop);
		CurrentPlayingAnim = DesiredAnim;
	}
}

void AcybmineCharacter::ApplyAvatarColor(const FLinearColor& NewColor)
{
	if (!GetMesh()) return;

	UMaterialInterface* BaseShapeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	const int32 NumMaterials = GetMesh()->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (BaseShapeMat)
		{
			if (UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseShapeMat, this))
			{
				DynMat->SetVectorParameterValue(FName("Color"), NewColor);
				GetMesh()->SetMaterial(i, DynMat);
			}
		}
		else if (UMaterialInstanceDynamic* DynMat = GetMesh()->CreateAndSetMaterialInstanceDynamic(i))
		{
			DynMat->SetVectorParameterValue(FName("Tint"), NewColor);
			DynMat->SetVectorParameterValue(FName("BaseColor"), NewColor);
			DynMat->SetVectorParameterValue(FName("Color"), NewColor);
		}
	}
}

void AcybmineCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (InventoryComponent)
	{
		InventoryComponent->OnEquippedItemChanged.AddDynamic(this, &AcybmineCharacter::HandleEquippedItemChanged);
	}

	// Il Dedicated Server non carica animazioni, mesh 3D o RenderTarget
	if (GetNetMode() != NM_DedicatedServer)
	{
		AutoAssignAnimBlueprint();
		ApplyAvatarColor(AvatarTintColor);

		PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		PreviewRenderTarget->InitAutoFormat(256, 384);
		PreviewRenderTarget->ClearColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
		PreviewRenderTarget->UpdateResourceImmediate(true);

		if (PreviewCaptureComponent)
		{
			PreviewCaptureComponent->TextureTarget = PreviewRenderTarget;
			PreviewCaptureComponent->ShowOnlyActors.Add(this);
			PreviewCaptureComponent->ShowOnlyComponents.Add(GetMesh());
			if (EquippedItemMesh)
			{
				PreviewCaptureComponent->ShowOnlyComponents.Add(EquippedItemMesh);
			}
		}

		if (InventoryComponent)
		{
			HandleEquippedItemChanged(InventoryComponent->GetSelectedHotbarItem());
		}
	}
}

void AcybmineCharacter::HandleEquippedItemChanged(const FInventoryItem& NewItem)
{
	// Il Dedicated Server conosce solo gli ID, non carica mai grafica
	if (GetNetMode() == NM_DedicatedServer || !EquippedItemMesh)
	{
		return;
	}

	// Annulla eventuale caricamento asincrono precedente in corso
	if (EquippedMeshLoadHandle.IsValid())
	{
		EquippedMeshLoadHandle->CancelHandle();
		EquippedMeshLoadHandle.Reset();
	}

	if (NewItem.IsEmpty())
	{
		CurrentVisualItemID = FPrimaryAssetId();
		EquippedItemMesh->SetStaticMesh(nullptr);
		EquippedItemMesh->SetVisibility(false);
		return;
	}

	CurrentVisualItemID = NewItem.ItemID;

	// Se il Data Asset è già in RAM lo usiamo subito, altrimenti lo carichiamo asincronamente
	if (UcybmineItemData* CachedData = UcybmineInventoryComponent::GetItemData(NewItem.ItemID))
	{
		StartAsyncLoadEquippedMesh(CachedData);
	}
	else
	{
		const FPrimaryAssetId ExpectedID = NewItem.ItemID;
		UcybmineInventoryComponent::GetItemDataAsync(ExpectedID, FOnItemDataLoaded::CreateWeakLambda(this, [this, ExpectedID](UcybmineItemData* LoadedData)
			{
				if (CurrentVisualItemID == ExpectedID && LoadedData)
				{
					StartAsyncLoadEquippedMesh(LoadedData);
				}
			}));
	}
}

void AcybmineCharacter::StartAsyncLoadEquippedMesh(UcybmineItemData* ItemData)
{
	if (!ItemData || ItemData->WorldStaticMesh.IsNull())
	{
		EquippedItemMesh->SetStaticMesh(nullptr);
		EquippedItemMesh->SetVisibility(false);
		return;
	}

	// Se la mesh 3D è già caricata in RAM, la applichiamo istantaneamente
	if (UStaticMesh* AlreadyLoadedMesh = ItemData->WorldStaticMesh.Get())
	{
		EquippedItemMesh->SetStaticMesh(AlreadyLoadedMesh);
		EquippedItemMesh->SetVisibility(true);
		return;
	}

	// Altrimenti avvia caricamento asincrono tramite FStreamableManager
	const FPrimaryAssetId ExpectedID = ItemData->GetPrimaryAssetId();
	const FSoftObjectPath MeshPath = ItemData->WorldStaticMesh.ToSoftObjectPath();

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	EquippedMeshLoadHandle = Streamable.RequestAsyncLoad(
		MeshPath,
		FStreamableDelegate::CreateUObject(this, &AcybmineCharacter::OnEquippedMeshAsyncLoaded, ExpectedID, MeshPath)
	);
}

void AcybmineCharacter::OnEquippedMeshAsyncLoaded(FPrimaryAssetId ExpectedItemID, FSoftObjectPath LoadedMeshPath)
{
	EquippedMeshLoadHandle.Reset();

	// Verifica che il giocatore non abbia già cambiato slot mentre caricava
	if (CurrentVisualItemID != ExpectedItemID || !EquippedItemMesh)
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(LoadedMeshPath.ResolveObject()))
	{
		EquippedItemMesh->SetStaticMesh(LoadedMesh);
		EquippedItemMesh->SetVisibility(true);
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

	if (Controller != nullptr && !IsInventoryOpen())
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const float ForwardValue = (bWDown ? 1.0f : 0.0f) - (bSDown ? 1.0f : 0.0f);
		if (ForwardValue != 0.0f)
		{
			AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), ForwardValue);
		}

		const float RightValue = (bDDown ? 1.0f : 0.0f) - (bADown ? 1.0f : 0.0f);
		if (RightValue != 0.0f)
		{
			AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), RightValue);
		}
	}

	if (PunchAnimAlpha > 0.0f)
	{
		PunchAnimAlpha = FMath::Max(0.0f, PunchAnimAlpha - (DeltaTime * 5.0f));
	}

	if (FirstPersonCamera)
	{
		const float Swing = FMath::Sin(PunchAnimAlpha * PI);
		FirstPersonCamera->SetRelativeLocation(FVector(16.0f + (Swing * 4.0f), Swing * -2.0f, 66.0f - (Swing * 2.5f)));
	}
	UpdateCharacterAnimation();
}

void AcybmineCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent) return;

	// Inventario
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AcybmineCharacter::ToggleInventory);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AcybmineCharacter::CloseInventory);
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AcybmineCharacter::CloseInventory);

	// Hotbar
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AcybmineCharacter::SelectSlot1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AcybmineCharacter::SelectSlot2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AcybmineCharacter::SelectSlot3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AcybmineCharacter::SelectSlot4);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AcybmineCharacter::SelectSlot5);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AcybmineCharacter::SelectSlot6);
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AcybmineCharacter::PrevHotbarSlot);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AcybmineCharacter::NextHotbarSlot);

	// Combattimento & Movimento
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AcybmineCharacter::PerformAttack);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AcybmineCharacter::PerformAttack);
	// Binding per lo Scudo / Parata
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AcybmineCharacter::StartBlock);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AcybmineCharacter::StopBlock);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AcybmineCharacter::DoJumpStart);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AcybmineCharacter::DoJumpEnd);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AcybmineCharacter::ToggleSprint);
	PlayerInputComponent->BindKey(EKeys::RightControl, IE_Pressed, this, &AcybmineCharacter::ToggleSprint);

	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AcybmineCharacter::OnWPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &AcybmineCharacter::OnWReleased);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &AcybmineCharacter::OnSPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &AcybmineCharacter::OnSReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AcybmineCharacter::OnDPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &AcybmineCharacter::OnDReleased);
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &AcybmineCharacter::OnAPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &AcybmineCharacter::OnAReleased);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AcybmineCharacter::LookRightDirect);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AcybmineCharacter::LookUpDirect);
}

bool AcybmineCharacter::IsInventoryOpen() const
{
	return InventoryComponent && InventoryComponent->bIsInventoryOpen;
}

void AcybmineCharacter::ToggleInventory()
{
	if (!InventoryComponent) return;

	InventoryComponent->bIsInventoryOpen = !InventoryComponent->bIsInventoryOpen;
	const bool bOpen = InventoryComponent->bIsInventoryOpen;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (bOpen)
		{
			bWDown = bSDown = bADown = bDDown = false;
			bIsSprinting = false;
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);

			if (PreviewSpringArm) PreviewSpringArm->SetRelativeRotation(FRotator(-5.0f, 180.0f, 0.0f));
			if (PreviewCaptureComponent) PreviewCaptureComponent->bCaptureEveryFrame = true;
			if (PreviewLight) PreviewLight->SetVisibility(true);
		}
		else
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			if (PreviewCaptureComponent) PreviewCaptureComponent->bCaptureEveryFrame = false;
			if (PreviewLight) PreviewLight->SetVisibility(false);
		}
	}
}

void AcybmineCharacter::CloseInventory()
{
	if (IsInventoryOpen())
	{
		ToggleInventory();
	}
}

void AcybmineCharacter::SelectSlot1() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(0); }
void AcybmineCharacter::SelectSlot2() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(1); }
void AcybmineCharacter::SelectSlot3() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(2); }
void AcybmineCharacter::SelectSlot4() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(3); }
void AcybmineCharacter::SelectSlot5() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(4); }
void AcybmineCharacter::SelectSlot6() { if (InventoryComponent) InventoryComponent->SelectHotbarSlot(5); }
void AcybmineCharacter::NextHotbarSlot() { if (InventoryComponent) InventoryComponent->NextHotbarSlot(); }
void AcybmineCharacter::PrevHotbarSlot() { if (InventoryComponent) InventoryComponent->PrevHotbarSlot(); }

void AcybmineCharacter::OnWPressed()
{
	if (IsInventoryOpen()) return;
	bWDown = true;
	if ((GetWorld()->GetTimeSeconds() - LastWReleaseTime) <= DoubleTapWindow)
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
	if (IsInventoryOpen()) return;
	bSDown = true;
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void AcybmineCharacter::OnSReleased() { bSDown = false; }
void AcybmineCharacter::OnDPressed() { if (!IsInventoryOpen()) bDDown = true; }
void AcybmineCharacter::OnDReleased() { bDDown = false; }
void AcybmineCharacter::OnAPressed() { if (!IsInventoryOpen()) bADown = true; }
void AcybmineCharacter::OnAReleased() { bADown = false; }

void AcybmineCharacter::ToggleSprint()
{
	if (IsInventoryOpen()) return;
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
	if (!IsInventoryOpen()) AddControllerYawInput(Value);
}

void AcybmineCharacter::LookUpDirect(float Value)
{
	if (!IsInventoryOpen()) AddControllerPitchInput(-Value);
}

void AcybmineCharacter::DoMove(float Forward, float Right)
{
	if (IsInventoryOpen()) return;
	bWDown = (Forward > 0.1f);
	bSDown = (Forward < -0.1f);
	bDDown = (Right > 0.1f);
	bADown = (Right < -0.1f);
}

void AcybmineCharacter::DoLook(float Yaw, float Pitch)
{
	if (IsInventoryOpen()) return;
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AcybmineCharacter::DoJumpStart()
{
	if (!IsInventoryOpen()) Jump();
}

void AcybmineCharacter::DoJumpEnd()
{
	StopJumping();
}

float AcybmineCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return CombatComponent ? CombatComponent->HandleIncomingDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser) : 0.0f;
}

void AcybmineCharacter::PerformAttack()
{
	if (IsInventoryOpen() || !CombatComponent) return;

	PunchAnimAlpha = 1.0f;
	CombatComponent->PerformAttack(FirstPersonCamera);
}
void AcybmineCharacter::StartBlock()
{
	if (IsInventoryOpen() || !CombatComponent) return;
	CombatComponent->Server_SetBlocking(true);
}

void AcybmineCharacter::StopBlock()
{
	if (!CombatComponent) return;
	CombatComponent->Server_SetBlocking(false);
}
