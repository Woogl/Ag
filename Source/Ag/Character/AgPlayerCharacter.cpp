// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Camera/CameraComponent.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "MotionWarpingComponent.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"

AAgPlayerCharacter::AAgPlayerCharacter()
{
	// The character faces where it moves; the camera turns with the mouse (플레이어 사양 '이동').
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyCameraData();
}

void AAgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITORONLY_DATA
	if (DevPressInput.IsValid())
	{
		const FGameplayTag InputTag = DevPressInput;
		DevPressInput = FGameplayTag();
		HandleInputPressed(InputTag);
	}
	if (DevMashInput.IsValid())
	{
		HandleInputPressed(DevMashInput);
	}
	if (!DevMoveInput.IsZero())
	{
		Move(FInputActionValue(DevMoveInput));
	}
#endif
}

void AAgPlayerCharacter::ApplyCameraData()
{
	const UAgCameraData* CameraData = UAgSettings::Get()->CameraData.LoadSynchronous();
	if (!CameraData)
	{
		UE_LOG(LogAg, Error, TEXT("Camera data is not set (Project Settings > Game > Ag)."));
		return;
	}

	CameraBoom->TargetArmLength = CameraData->Distance;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, CameraData->PivotHeight));

	// The arm lags exponentially; the follow lag time is its time constant (about 63% caught up after that time).
	CameraBoom->bEnableCameraLag = CameraData->FollowLagTime > 0.f;
	CameraBoom->CameraLagSpeed = CameraBoom->bEnableCameraLag ? 1.f / CameraData->FollowLagTime : 0.f;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera->SetFieldOfView(CameraData->FieldOfView);
}

void AAgPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InputMapping, 0);
		}
	}
}

void AAgPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::AttackPressed);
}

bool AAgPlayerCharacter::CanMove() const
{
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Acting);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	Blockers.AddTag(AgGameplayTags::State_Groggy);
	Blockers.AddTag(AgGameplayTags::State_Execution_Executing);
	Blockers.AddTag(AgGameplayTags::State_Dead);
	return !GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	// Movement input never cancels an action motion; it is ignored until the motion ends (플레이어 사양 '모션 캔슬').
	if (!Controller || !CanMove())
	{
		return;
	}

	// X is right, Y is forward, relative to where the camera looks.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
	// Applied directly with no smoothing (카메라 '자유 시점' 회전 반응).
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AAgPlayerCharacter::AttackPressed()
{
	HandleInputPressed(AgGameplayTags::Input_Attack);
}

void AAgPlayerCharacter::HandleInputPressed(const FGameplayTag& InputTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (InputTag == AgGameplayTags::Input_Attack)
	{
		// 평타 키의 처리 순서: a running basic attack takes the press itself (next hit during recovery); otherwise hit 1.
		const FGameplayTagContainer BasicAttackTag(AgGameplayTags::Ability_Action_BasicAttack);
		TArray<FGameplayAbilitySpec*> Specs;
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(BasicAttackTag, Specs, false);
		const bool bRunning = Specs.ContainsByPredicate([](const FGameplayAbilitySpec* Spec) { return Spec->IsActive(); });
		if (bRunning)
		{
			FGameplayEventData Payload;
			Payload.EventTag = InputTag;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, InputTag, Payload);
		}
		else
		{
			ASC->TryActivateAbilitiesByTag(BasicAttackTag);
		}
	}
}

AAgBossCharacter* AAgPlayerCharacter::FindBoss(float Range) const
{
	const UTargetingPreset* Preset = UAgSettings::Get()->BossTargeting.LoadSynchronous();
	UTargetingSubsystem* Targeting = UTargetingSubsystem::Get(GetWorld());
	if (!Preset || !Targeting)
	{
		UE_LOG(LogAg, Error, TEXT("Boss targeting preset is not set (Project Settings > Game > Ag)."));
		return nullptr;
	}

	FTargetingSourceContext Source;
	Source.SourceActor = const_cast<AAgPlayerCharacter*>(this);
	Source.InstigatorActor = Source.SourceActor;
	FTargetingRequestHandle Request = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Source);
	Targeting->ExecuteTargetingRequestWithHandle(Request);
	TArray<AActor*> Candidates;
	Targeting->GetTargetingResultsActors(Request, Candidates);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Request);

	for (AActor* Candidate : Candidates)
	{
		AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Candidate);
		if (Boss && !Boss->IsDead() && UAgCombatLibrary::GetHorizontalDistance(this, Boss) <= Range)
		{
			return Boss;
		}
	}
	return nullptr;
}

void AAgPlayerCharacter::PrepareAttackMovement(float TargetingRange)
{
	const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData());
	if (!Data)
	{
		return;
	}

	// The stop distance applies whether or not the boss is within the targeting range.
	AAgBossCharacter* Boss = FindBoss(TNumericLimits<float>::Max());
	SetStopTarget(Boss, Data->AttackStopDistance);

	if (Boss && UAgCombatLibrary::GetHorizontalDistance(this, Boss) <= TargetingRange)
	{
		StartApproach(Boss, Data->AttackStopDistance, TargetingRange, 0.f, /*bFaceTarget*/ true);
	}
	else
	{
		// No target: the motion plays as made, toward the character's front.
		StopApproach();
		GetMotionWarping()->RemoveWarpTarget(AttackWarpTarget);
	}
}

void AAgPlayerCharacter::ClearAttackMovement()
{
	ClearStopTarget();
	StopApproach();
	GetMotionWarping()->RemoveWarpTarget(AttackWarpTarget);
}
