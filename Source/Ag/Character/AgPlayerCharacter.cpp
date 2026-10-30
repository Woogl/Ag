// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/AgExecutionCameraModifier.h"
#include "Camera/AgFieldOfViewCameraModifier.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgAssetManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"

namespace
{
	const FName JumpMontageId(TEXT("Jump"));
	const FName JumpLandSection(TEXT("Land"));

	/**
	 * CriticallyDampedSmoothing은 한 스텝이 SmoothingTime의 절반보다 짧을 때만 정확하므로 긴 프레임은 나눠 계산합니다.
	 */
	float SpringToward(float Angle, float& Rate, float Target, float DeltaTime, float SmoothingTime, float MaxSpeed)
	{
		const int32 Steps = SmoothingTime > 0.f ? FMath::Clamp(FMath::CeilToInt(DeltaTime / (0.5f * SmoothingTime)), 1, 8) : 1;
		const float StepTime = DeltaTime / Steps;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float Start = Angle;
			FMath::CriticallyDampedSmoothing(Angle, Rate, Target, 0.f, StepTime, SmoothingTime);
			Angle = Start + FMath::Clamp(Angle - Start, -MaxSpeed * StepTime, MaxSpeed * StepTime);
			Rate = FMath::Clamp(Rate, -MaxSpeed, MaxSpeed);
		}
		return Angle;
	}
}

AAgPlayerCharacter::AAgPlayerCharacter()
{
	CharacterDataClass = UAgPlayerData::StaticClass();
	Team = EAgTeam::Ally;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;
	// 스프링 암은 캐릭터의 회전을 물려받지 않습니다. 행동이 캐릭터를 한 번에 돌리는 일이 스프링 암이 그 프레임의 카메라
	// 자리를 정한 뒤에도 일어나므로(선입력한 회피는 타이머로 실행), 물려받으면 그 프레임 동안 카메라도 함께 돌아갑니다.
	CameraBoom->SetUsingAbsoluteRotation(true);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PerfectDodgeCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("PerfectDodgeCapsule"));
	PerfectDodgeCapsule->SetupAttachment(GetCapsuleComponent());
	PerfectDodgeCapsule->SetUsingAbsoluteLocation(true);
	PerfectDodgeCapsule->SetUsingAbsoluteRotation(true);
	PerfectDodgeCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 투사체와 대상 찾기는 Pawn 오브젝트 종류로 찾고 응답은 보지 않으므로, 응답은 무기 판정 채널만 둡니다.
	PerfectDodgeCapsule->SetCollisionObjectType(ECC_Pawn);
	PerfectDodgeCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	PerfectDodgeCapsule->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);
	PerfectDodgeCapsule->SetGenerateOverlapEvents(false);
	PerfectDodgeCapsule->SetCanEverAffectNavigation(false);
}

void AAgPlayerCharacter::UpdateRotationMode()
{
	// 락온 카메라는 대상을 보므로, 카메라 정면을 바라보면 대상을 바라봅니다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bSprinting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting);
	const bool bFaceCamera = (GetLockOnTarget() && !bSprinting) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
	GetCharacterMovement()->bOrientRotationToMovement = !bFaceCamera;
	GetCharacterMovement()->bUseControllerDesiredRotation = bFaceCamera;
}

void AAgPlayerCharacter::PlayCameraEffect(EAgCameraSituation Situation, const AActor* Opponent)
{
	const FAgCameraEffect* Effect = CameraEffects.Find(Situation);
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!Effect || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}
	for (const TSubclassOf<UCameraShakeBase>& Shake : Effect->CameraShakes)
	{
		PlayerController->PlayerCameraManager->StartCameraShake(Shake);
	}
	if (FieldOfViewCamera)
	{
		FieldOfViewCamera->Play(Effect->FieldOfView);
	}
	if (Effect->bTurnToOpponent && Opponent)
	{
		StartCameraTurn(*Opponent, Effect->TurnAngle);
	}
}

void AAgPlayerCharacter::PlayDamageCameraEffects(const FGameplayTagContainer& SpecTags, const FGameplayTag& Reaction, const AAgCharacterBase& Attacker, const AAgCharacterBase& Target)
{
	const bool bHeavy = Reaction == AgGameplayTags::Event_Hit_KnockUp || Reaction == AgGameplayTags::Event_Hit_KnockDown ||
	                    Reaction == AgGameplayTags::Event_Guard_Pushback;
	if (&Attacker == this)
	{
		if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			const bool bFinisher = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Finisher);
			PlayCameraEffect(bFinisher ? EAgCameraSituation::ExecutionBlow : EAgCameraSituation::ExecutionStab, &Target);
		}
		else if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit))
		{
			PlayCameraEffect(bHeavy ? EAgCameraSituation::PlayerHitHeavy : EAgCameraSituation::PlayerHit, &Target);
			if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_DodgeCounter))
			{
				PlayCameraEffect(EAgCameraSituation::DodgeCounterHit, &Target);
			}
			else if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_GuardCounter))
			{
				PlayCameraEffect(EAgCameraSituation::GuardCounterHit, &Target);
			}
		}
	}
	else if (&Target == this)
	{
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit))
		{
			PlayCameraEffect(bHeavy ? EAgCameraSituation::PlayerHurtHeavy : EAgCameraSituation::PlayerHurt, &Attacker);
		}
		else if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Parry))
		{
			PlayCameraEffect(EAgCameraSituation::Parry, &Attacker);
		}
		else if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Guard))
		{
			PlayCameraEffect(bHeavy ? EAgCameraSituation::GuardPushback : EAgCameraSituation::Guard, &Attacker);
		}
		else if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_GuardBreak))
		{
			PlayCameraEffect(EAgCameraSituation::GuardBreak, &Attacker);
		}
	}
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleAbilityCommitted);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleSprintFieldOfViewChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::Input_Move, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleSprintFieldOfViewChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Busy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_PerfectDodgeWindow, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandlePerfectDodgeWindowChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_TimeStop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleTimeStopChanged);
	PerfectDodgeCapsule->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	UpdateRotationMode();
}

void AAgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSprint();
	UpdateLockOn();
	// 락온 카메라처럼 히트스톱(주인의 시간 배율)에 멈추지 않도록 월드 시간으로 돕니다.
	UpdateCameraTurn(GetWorld()->GetDeltaSeconds());

#if WITH_EDITORONLY_DATA
	// 누른 키는 다음 프레임에 뗍니다. 같은 프레임에 떼면 점프 키는 엔진 점프가 시작되기 전에 StopJumping이
	// 누름을 지웁니다.
	if (DevReleaseInput.IsValid())
	{
		const FGameplayTag InputTag = DevReleaseInput;
		DevReleaseInput = FGameplayTag();
		HandleInputReleased(InputTag);
	}
	if (DevPressInput.IsValid())
	{
		DevReleaseInput = DevPressInput;
		DevPressInput = FGameplayTag();
		HandleInputPressed(DevReleaseInput);
	}
	if (DevMashInput.IsValid())
	{
		HandleInputPressed(DevMashInput);
	}
	if (DevHoldInput != DevHeldInput)
	{
		if (DevHeldInput.IsValid())
		{
			HandleInputReleased(DevHeldInput);
		}
		DevHeldInput = DevHoldInput;
		if (DevHeldInput.IsValid())
		{
			HandleInputPressed(DevHeldInput);
		}
	}
	if (!DevMoveInput.IsZero())
	{
		bDevMoving = true;
		Move(FInputActionValue(DevMoveInput));
	}
	else if (bDevMoving)
	{
		bDevMoving = false;
		MoveReleased();
	}
#endif
}

void AAgPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InputMapping, 0);
		}
		if (!ExecutionCamera && ExecutionCameraClass && PlayerController->PlayerCameraManager)
		{
			ExecutionCamera = Cast<UAgExecutionCameraModifier>(PlayerController->PlayerCameraManager->AddNewCameraModifier(ExecutionCameraClass));
		}
		if (!FieldOfViewCamera && PlayerController->PlayerCameraManager)
		{
			FieldOfViewCamera = Cast<UAgFieldOfViewCameraModifier>(PlayerController->PlayerCameraManager->AddNewCameraModifier(UAgFieldOfViewCameraModifier::StaticClass()));
		}
	}
}

void AAgPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::MoveReleased);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);

	const TPair<const UInputAction*, FGameplayTag> ActionKeys[] = {
		{AttackAction, AgGameplayTags::Input_Attack},
		{GuardAction, AgGameplayTags::Input_Guard},
		{DodgeAction, AgGameplayTags::Input_Dodge},
		{JumpAction, AgGameplayTags::Input_Jump},
		{SkillAction, AgGameplayTags::Input_Skill},
		{UltimateAction, AgGameplayTags::Input_Ultimate},
		{ExecuteAction, AgGameplayTags::Input_Execute},
		{LockOnAction, AgGameplayTags::Input_LockOn},
	};
	for (const TPair<const UInputAction*, FGameplayTag>& ActionKey : ActionKeys)
	{
		EnhancedInput->BindAction(ActionKey.Key, ETriggerEvent::Started, this, &ThisClass::HandleInputPressed, ActionKey.Value);
	}
	EnhancedInput->BindAction(GuardAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased,
		FGameplayTag(AgGameplayTags::Input_Guard));
	EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased,
		FGameplayTag(AgGameplayTags::Input_Dodge));
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased,
		FGameplayTag(AgGameplayTags::Input_Jump));
}

void AAgPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 점프 공격의 시작 조건이라, 점프 행동이 다른 까닭으로 끝나도 착지할 때까지 남깁니다.
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->SetLooseGameplayTagCount(AgGameplayTags::State_Jumping, 1);

	// 키를 누른 뒤 실제 점프 전에 시작한 행동·피격 반응의 모션을 끊지 않도록, 점프 행동이 실행 중일 때만 재생합니다.
	const FGameplayAbilitySpec* Jump = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	UAnimMontage* JumpMontage = GetCharacterData()->FindMontage(JumpMontageId);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (Jump && Jump->IsActive() && JumpMontage && AnimInstance && !IsDead())
	{
		AnimInstance->Montage_Play(JumpMontage);
	}
}

void AAgPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// 점프 공격이나 피격 반응이 점프 행동을 끝냈으면 그 행동의 모션이 착지를 맡습니다.
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Jump = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	const bool bJumpAbility = Jump && Jump->IsActive();
	ASC->SetLooseGameplayTagCount(AgGameplayTags::State_Jumping, 0);
	UAnimMontage* JumpMontage = GetCharacterData()->FindMontage(JumpMontageId);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!bJumpAbility || !JumpMontage || !AnimInstance || IsDead())
	{
		return;
	}
	if (!MoveInputDirection.IsNearlyZero())
	{
		if (AnimInstance->Montage_IsPlaying(JumpMontage))
		{
			AnimInstance->Montage_StopWithBlendOut(JumpMontage->GetBlendOutArgs(), JumpMontage);
		}
		return;
	}

	// 섹션만 옮기지 않고 다시 재생해, 남은 점프 모션이 블렌드 아웃되고 착지 모션이 블렌드 인하게 합니다.
	if (AnimInstance->Montage_Play(JumpMontage) > 0.f)
	{
		AnimInstance->Montage_JumpToSection(JumpLandSection, JumpMontage);
	}
}

void AAgPlayerCharacter::ApplyDeathState()
{
	Super::ApplyDeathState();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	// 회피가 누른 키 쪽으로 구르고 Input.Move로 구르기 섹션을 고르므로, 이동을 무시할 때도 방향과 태그는 정합니다.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	MoveInputDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(AgGameplayTags::Input_Move, MoveInputDirection.IsNearlyZero() ? 0 : 1);

	if (CanMove())
	{
		AddMovementInput(Forward, Input.Y);
		AddMovementInput(Right, Input.X);
		StopLandingMotion();
	}
}

void AAgPlayerCharacter::StopLandingMotion()
{
	// 착지 모션은 제자리 모션이라 움직이면 미끄러져 보이고, 행동이 아니므로 움직이면 끝냅니다.
	UAnimMontage* JumpMontage = GetCharacterData()->FindMontage(JumpMontageId);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (JumpMontage && AnimInstance && AnimInstance->Montage_IsPlaying(JumpMontage) &&
		AnimInstance->Montage_GetCurrentSection(JumpMontage) == JumpLandSection)
	{
		AnimInstance->Montage_StopWithBlendOut(JumpMontage->GetBlendOutArgs(), JumpMontage);
	}
}

void AAgPlayerCharacter::MoveReleased()
{
	MoveInputDirection = FVector::ZeroVector;
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(AgGameplayTags::Input_Move, 0);
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
	if (GetLockOnTarget() || IsExecutionCameraActive())
	{
		return;
	}
	TurnOpponent.Reset();

	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AAgPlayerCharacter::HandleInputPressed(FGameplayTag InputTag)
{
	if (IsDead())
	{
		return;
	}
	if (InputTag == AgGameplayTags::Input_Guard)
	{
		bGuardHeld = true;
	}
	if (InputTag == AgGameplayTags::Input_Dodge)
	{
		bDodgeHeld = true;
	}
	if (InputTag == AgGameplayTags::Input_LockOn)
	{
		ToggleLockOn();
		return;
	}

	// 새로 누른 키가 기억한 입력을 대신하므로, 막히지 않은 키는 기억을 지웁니다.
	const bool bFallingJump = InputTag == AgGameplayTags::Input_Jump && GetCharacterMovement()->IsFalling();
	BufferedInput = !bFallingJump && InputTag != AgGameplayTags::Input_Execute && IsActionInputBlocked(InputTag)
	                    ? InputTag
	                    : FGameplayTag();
	BufferedInputTime = GetWorld()->GetTimeSeconds();

	// 막힌 동안에도 실행 중인 행동은 입력 이벤트를 받으므로(기본 공격의 다음 타) 키는 지금도 넣습니다.
	RunInput(InputTag);
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (InputTag == AgGameplayTags::Input_Attack)
	{
		const FGameplayAbilitySpec* BasicAttack = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_BasicAttack);
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Jumping))
		{
			ActivateByTag(AgGameplayTags::Ability_Action_JumpAttack);
		}
		else if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_DodgeCounterChance))
		{
			ActivateByTag(AgGameplayTags::Ability_Action_DodgeCounter);
		}
		else if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_GuardCounterChance))
		{
			ActivateByTag(AgGameplayTags::Ability_Action_GuardCounter);
		}
		else if (BasicAttack && BasicAttack->IsActive())
		{
			FGameplayEventData Payload;
			Payload.EventTag = InputTag;
			Payload.Instigator = this;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, InputTag, Payload);
		}
		else
		{
			ActivateByTag(AgGameplayTags::Ability_Action_BasicAttack);
		}
	}
	else if (InputTag == AgGameplayTags::Input_Guard)
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Guard);
	}
	else if (InputTag == AgGameplayTags::Input_Dodge)
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Dodge);
	}
	else if (InputTag == AgGameplayTags::Input_Skill)
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Skill);
	}
	else if (InputTag == AgGameplayTags::Input_Ultimate)
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Ultimate);
	}
	else if (InputTag == AgGameplayTags::Input_Execute)
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Execution);
	}
	else if (InputTag == AgGameplayTags::Input_Jump)
	{
		TryJump();
	}
}

void AAgPlayerCharacter::HandleInputReleased(FGameplayTag InputTag)
{
	if (InputTag == AgGameplayTags::Input_Guard && bGuardHeld)
	{
		// 키를 누르는 동안 이어지는 행동(가드, 달리기)도 엔진 태그 트리거(OwnedTagPresent) 대신 직접 발동·취소합니다. 태그
		// 트리거는 태그가 붙는 순간 막혀 있으면 다시 시도하지 않아, 태그를 실행 상태와 맞추는 코드가 따로 필요해집니다.
		bGuardHeld = false;
		GetAbilitySystemComponent()->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Guard));
	}
	if (InputTag == AgGameplayTags::Input_Dodge)
	{
		bDodgeHeld = false;
	}
	if (InputTag == AgGameplayTags::Input_Jump)
	{
		StopJumping();
	}
}

void AAgPlayerCharacter::ActivateByTag(const FGameplayTag& AbilityTag)
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AbilityTag))
	{
		ASC->TryActivateAbility(Spec->Handle);
	}
}

void AAgPlayerCharacter::TryJump()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	const UGameplayAbility* Ability = Spec ? Spec->GetPrimaryInstance() : nullptr;
	if (Ability && Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get()))
	{
		Jump();
	}
}

bool AAgPlayerCharacter::IsActionInputBlocked(const FGameplayTag& InputTag) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction))
	{
		return true;
	}
	if (InputTag == AgGameplayTags::Input_Attack && ASC->HasMatchingGameplayTag(AgGameplayTags::State_Jumping))
	{
		return false;
	}
	return ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy);
}

void AAgPlayerCharacter::HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 이번 프레임에 이어지는 상태(행동을 끊은 피격 반응, 기본 공격의 후딜 표시)가 정해진 뒤에 보도록
	// 타이머 처리(TG_PostPhysics 뒤)에서 확인합니다.
	if (NewCount != 0)
	{
		return;
	}
	if (Tag == AgGameplayTags::State_Busy && bGuardHeld)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunPendingInput);
	}
	else if (BufferedInput.IsValid())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunBufferedInput);
	}
}

void AAgPlayerCharacter::TryRunBufferedInput()
{
	if (!BufferedInput.IsValid() || IsActionInputBlocked(BufferedInput))
	{
		return;
	}

	const FGameplayTag InputTag = BufferedInput;
	BufferedInput = FGameplayTag();
	const bool bExpired = GetWorld()->GetTimeSeconds() - BufferedInputTime > InputBufferTime;
	if (IsDead() || bExpired || (InputTag == AgGameplayTags::Input_Guard && !bGuardHeld))
	{
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Buffered input runs: %s"), *InputTag.ToString());
	RunInput(InputTag);
}

bool AAgPlayerCharacter::CanMove() const
{
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Acting);
	Blockers.AddTag(AgGameplayTags::State_ParryWindow);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	Blockers.AddTag(AgGameplayTags::State_Groggy);
	Blockers.AddTag(AgGameplayTags::State_Execution);
	Blockers.AddTag(AgGameplayTags::State_Dead);
	return !GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(EndedData.AbilityThatEnded);
	if (ExecutionCamera && AgAbility && AgAbility->GetDefinitionTags().HasTag(AgGameplayTags::Ability_Action_Execution))
	{
		ExecutionCamera->Stop();
	}
	if (IsDead() || !AgAbility)
	{
		return;
	}

	const FGameplayTagContainer& EndedTags = AgAbility->GetDefinitionTags();
	if (!bGuardHeld || EndedTags.HasTag(AgGameplayTags::Ability_Action_Guard) ||
		(!EndedTags.HasTag(AgGameplayTags::Ability_Action) && !EndedTags.HasTag(AgGameplayTags::Ability_Reaction)))
	{
		return;
	}

	// 이 행동을 취소한 행동이 시작된 뒤에 보도록 타이머 처리(TG_PostPhysics 뒤)에서 확인합니다.
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunPendingInput);
}

void AAgPlayerCharacter::TryRunPendingInput()
{
	// 기억한 입력은 누르고 있는 가드 키보다 나중에 누른 것이므로 먼저 처리합니다.
	TryRunBufferedInput();
	if (!bGuardHeld || IsDead())
	{
		return;
	}

	// 패리 모션은 가드를 끝내지 않으므로, 가드 중이면 후딜에 든 패리 모션만 끝내 가드 자세로 돌아옵니다.
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Guard = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Guard);
	if (Guard && Guard->IsActive())
	{
		ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Parry));
		return;
	}

	FGameplayTagContainer Running;
	Running.AddTag(AgGameplayTags::State_Busy);
	Running.AddTag(AgGameplayTags::State_HitReaction);
	Running.AddTag(AgGameplayTags::State_Execution);
	if (!ASC->HasAnyMatchingGameplayTags(Running))
	{
		ActivateByTag(AgGameplayTags::Ability_Action_Guard);
	}
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	// 엔진 PreActivate 직전의 알림이라, 극한 회피 모션이 자기 발동 Task로 주는 회피 반격 기회는 지워지지 않습니다.
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockDown);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_GuardBreak);
	if (const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability); AgAbility && AgAbility->GetDefinitionTags().HasAny(Enders))
	{
		FGameplayTagContainer Chances;
		Chances.AddTag(AgGameplayTags::State_DodgeCounterChance);
		Chances.AddTag(AgGameplayTags::State_GuardCounterChance);
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(Chances);
	}
}

void AAgPlayerCharacter::HandleAbilityCommitted(UGameplayAbility* Ability)
{
	// 처형 대상은 발동 중(ActivateAbility)에 정해지므로, 발동 알림이 아니라 커밋 알림에서 읽습니다.
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const AActor* Target = AgAbility ? AgAbility->GetActionTarget() : nullptr;
	if (ExecutionCamera && Target && AgAbility->GetDefinitionTags().HasTag(AgGameplayTags::Ability_Action_Execution))
	{
		ExecutionCamera->Start(*this, *Target);
	}
}

void AAgPlayerCharacter::UpdateSprint()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bWantsSprint = bDodgeHeld && GetAttributeSet()->GetSP() > 0.f;
	const FGameplayAbilitySpec* Sprint = ASC->FindSpecByTag(AgGameplayTags::Ability_Sprint);
	if (Sprint && Sprint->IsActive())
	{
		if (!bWantsSprint)
		{
			ASC->CancelAbilityHandle(Sprint->Handle);
		}
		return;
	}

	const FGameplayAbilitySpec* Dodge = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Dodge);
	if (bWantsSprint && !MoveInputDirection.IsNearlyZero() && !bGuardHeld && !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy) && Dodge && Dodge->IsActive())
	{
		ActivateByTag(AgGameplayTags::Ability_Sprint);
	}
}

void AAgPlayerCharacter::UpdateLockOn()
{
	AAgCharacterBase* Target = GetLockOnTarget();
	if (Target != LockOnCameraTarget.Get())
	{
		LockOnCameraTarget = Target;
		LockOnPitchRate = 0.f;
		LockOnYawRate = 0.f;
		UpdateRotationMode();
	}

	// 카메라는 캐릭터의 모션이 아니므로 히트스톱(주인의 시간 배율)에 멈추지 않습니다.
	if (Target)
	{
		UpdateLockOnCamera(*Target, GetWorld()->GetDeltaSeconds());
	}
}

void AAgPlayerCharacter::ToggleLockOn()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* LockOn = ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn);
	if (LockOn && LockOn->IsActive())
	{
		ASC->CancelAbilityHandle(LockOn->Handle);
	}
	else
	{
		ActivateByTag(AgGameplayTags::Ability_LockOn);
	}
}

void AAgPlayerCharacter::UpdateLockOnCamera(const AAgCharacterBase& Target, float DeltaTime)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	const AAgBossCharacter* Boss = Cast<AAgBossCharacter>(&Target);
	const FVector LockOnPoint = Boss ? Boss->GetLockOnLocation() : Target.GetActorLocation();

	const FVector Pivot = CameraBoom->GetComponentLocation();
	FRotator Goal = (LockOnPoint - Pivot).Rotation();
	Goal.Pitch =
		FMath::Clamp(FRotator::NormalizeAxis(Goal.Pitch), LockOnPitchMin, LockOnPitchMax) - LockOnLookDown;
	Goal.Roll = 0.f;

	const FRotator Current = PlayerController->GetControlRotation();
	const float CurrentPitch = static_cast<float>(FRotator::NormalizeAxis(Current.Pitch));
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float GoalYaw = CurrentYaw + static_cast<float>(FMath::FindDeltaAngleDegrees(Current.Yaw, Goal.Yaw));
	const float Pitch = SpringToward(CurrentPitch, LockOnPitchRate, static_cast<float>(Goal.Pitch), DeltaTime,
		LockOnSmoothingTime, LockOnRotationSpeed);
	const float Yaw =
		SpringToward(CurrentYaw, LockOnYawRate, GoalYaw, DeltaTime, LockOnSmoothingTime, LockOnRotationSpeed);
	PlayerController->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void AAgPlayerCharacter::StartCameraTurn(const AActor& Opponent, float Angle)
{
	// 같은 프레임에 여러 회전이 시작되면 처리 순서와 관계없이 가장 가까운 상대 쪽으로 돕니다.
	if (TurnStartFrame == GFrameCounter && TurnOpponent.IsValid() &&
		GetSquaredDistanceTo(TurnOpponent.Get()) <= GetSquaredDistanceTo(&Opponent))
	{
		return;
	}
	if (!TurnOpponent.IsValid())
	{
		TurnYawRate = 0.f;
	}
	TurnOpponent = &Opponent;
	TurnAngle = Angle;
	TurnStartFrame = GFrameCounter;
}

void AAgPlayerCharacter::UpdateCameraTurn(float DeltaTime)
{
	const AActor* Opponent = TurnOpponent.Get();
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!Opponent || !PlayerController || GetLockOnTarget() || IsExecutionCameraActive())
	{
		TurnOpponent.Reset();
		return;
	}

	const FRotator Current = PlayerController->GetControlRotation();
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float OpponentYaw = static_cast<float>((Opponent->GetActorLocation() - CameraBoom->GetComponentLocation()).Rotation().Yaw);
	const float Offset = FMath::FindDeltaAngleDegrees(CurrentYaw, OpponentYaw);
	// 감쇠 접근은 목표 경계에 끝없이 다가가므로, 1° 안으로 들면 끝냅니다.
	if (FMath::Abs(Offset) <= TurnAngle + 1.f)
	{
		TurnOpponent.Reset();
		return;
	}
	const float GoalYaw = CurrentYaw + Offset - FMath::Sign(Offset) * TurnAngle;
	const float Yaw = SpringToward(CurrentYaw, TurnYawRate, GoalYaw, DeltaTime, LockOnSmoothingTime, LockOnRotationSpeed);
	PlayerController->SetControlRotation(FRotator(Current.Pitch, Yaw, 0.f));
}

bool AAgPlayerCharacter::IsExecutionCameraActive() const
{
	return ExecutionCamera && !ExecutionCamera->IsDisabled();
}

void AAgPlayerCharacter::HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateRotationMode();
}

void AAgPlayerCharacter::HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 달리기는 Shift만 누르고 멈춰 있어도 이어지므로, 이동 입력이 있을 때만 넓힙니다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bRunning = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting) && ASC->HasMatchingGameplayTag(AgGameplayTags::Input_Move);
	if (!FieldOfViewCamera || bRunning == bSprintFieldOfViewHeld)
	{
		return;
	}
	bSprintFieldOfViewHeld = bRunning;
	if (bRunning)
	{
		FieldOfViewCamera->Hold(SprintFieldOfView);
	}
	else
	{
		FieldOfViewCamera->Release(SprintFieldOfView);
	}
}

void AAgPlayerCharacter::HandlePerfectDodgeWindowChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		PerfectDodgeCapsule->SetWorldLocation(GetCapsuleComponent()->GetComponentLocation());
		PerfectDodgeCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		PerfectDodgeCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AAgPlayerCharacter::HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount)
{
	UAgTimeSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UAgTimeSubsystem>();
	if (NewCount > 0)
	{
		TimeSubsystem->StartTimeStop(*this);
		// 셰이크는 컷신 카메라에도 더해지므로, 진행 중인 연출을 여기서 끝냅니다.
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetController());
			PlayerController && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StopAllCameraShakes(true);
		}
		if (FieldOfViewCamera)
		{
			FieldOfViewCamera->StopAll();
		}
		TurnOpponent.Reset();
	}
	else
	{
		TimeSubsystem->EndTimeStop(*this);
	}
}
