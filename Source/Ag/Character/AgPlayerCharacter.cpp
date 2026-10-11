// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayEffect_Regen.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgLockOnComponent.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgCharacterData.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "TimerManager.h"

AAgPlayerCharacter::AAgPlayerCharacter()
{
	// The character faces where it moves; the camera turns with the mouse (플레이어 사양 '이동').
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;
	// The arm doesn't inherit the character's rotation. Actions turn the character at once, sometimes after the arm has
	// placed the camera for the frame (a buffered dodge runs from a timer), and the camera would turn with it for that frame.
	CameraBoom->SetUsingAbsoluteRotation(true);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	LockOn = CreateDefaultSubobject<UAgLockOnComponent>(TEXT("LockOn"));
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Guarding, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Busy, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetSPAttribute()).AddUObject(this, &ThisClass::HandleSPChanged);
	LockOn->OnLockOnChanged.AddUObject(this, &ThisClass::UpdateRotationMode);
	UpdateRotationMode();
}

void AAgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSprint();

#if WITH_EDITORONLY_DATA
	if (DevPressInput.IsValid())
	{
		const FGameplayTag InputTag = DevPressInput;
		DevPressInput = FGameplayTag();
		HandleInputPressed(InputTag);
		HandleInputReleased(InputTag);
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
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::MoveReleased);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::AttackPressed);
	EnhancedInput->BindAction(GuardAction, ETriggerEvent::Started, this, &ThisClass::GuardPressed);
	EnhancedInput->BindAction(GuardAction, ETriggerEvent::Completed, this, &ThisClass::GuardReleased);
	EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &ThisClass::DodgePressed);
	EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ThisClass::DodgeReleased);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::JumpPressed);
	EnhancedInput->BindAction(SkillAction, ETriggerEvent::Started, this, &ThisClass::SkillPressed);
	EnhancedInput->BindAction(UltimateAction, ETriggerEvent::Started, this, &ThisClass::UltimatePressed);
	EnhancedInput->BindAction(ExecuteAction, ETriggerEvent::Started, this, &ThisClass::ExecutePressed);
	EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::LockOnPressed);
}

bool AAgPlayerCharacter::CanMove() const
{
	// 행동, 피격 반응, 가드 시작 직후에는 움직이지 않습니다(플레이어 입력 '상태별 허용 입력', '모션 캔슬').
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Acting);
	Blockers.AddTag(AgGameplayTags::State_GuardMoveLock);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	Blockers.AddTag(AgGameplayTags::State_Groggy);
	Blockers.AddTag(AgGameplayTags::State_Execution_Executing);
	Blockers.AddTag(AgGameplayTags::State_Dead);
	return !GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	// X is right, Y is forward, relative to where the camera looks. The direction is kept even when movement is
	// ignored, because the dodge rolls toward the keys held.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	MoveInputDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();

	// Movement input never cancels an action motion; it is ignored until the motion ends (플레이어 입력 '모션 캔슬').
	if (CanMove())
	{
		AddMovementInput(Forward, Input.Y);
		AddMovementInput(Right, Input.X);
		StopLandingMotion();
	}
}

void AAgPlayerCharacter::StopLandingMotion()
{
	// The landing motion stays in place through its root motion, which would hold the character still. It is no action,
	// so moving ends it.
	const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData());
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (Data && Data->LandMontage && AnimInstance && AnimInstance->Montage_IsPlaying(Data->LandMontage))
	{
		AnimInstance->Montage_Stop(0.2f, Data->LandMontage);
	}
}

void AAgPlayerCharacter::MoveReleased()
{
	MoveInputDirection = FVector::ZeroVector;
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
	// The lock-on camera ignores look input (카메라 '카메라 모드').
	if (LockOn->IsLockedOn())
	{
		return;
	}

	// Applied directly with no smoothing (카메라 '자유 시점' 회전 반응).
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AAgPlayerCharacter::AttackPressed()
{
	HandleInputPressed(AgGameplayTags::Input_Attack);
}

void AAgPlayerCharacter::GuardPressed()
{
	HandleInputPressed(AgGameplayTags::Input_Guard);
}

void AAgPlayerCharacter::GuardReleased()
{
	HandleInputReleased(AgGameplayTags::Input_Guard);
}

void AAgPlayerCharacter::DodgePressed()
{
	HandleInputPressed(AgGameplayTags::Input_Dodge);
}

void AAgPlayerCharacter::DodgeReleased()
{
	HandleInputReleased(AgGameplayTags::Input_Dodge);
}

void AAgPlayerCharacter::JumpPressed()
{
	HandleInputPressed(AgGameplayTags::Input_Jump);
}

void AAgPlayerCharacter::SkillPressed()
{
	HandleInputPressed(AgGameplayTags::Input_Skill);
}

void AAgPlayerCharacter::UltimatePressed()
{
	HandleInputPressed(AgGameplayTags::Input_Ultimate);
}

void AAgPlayerCharacter::ExecutePressed()
{
	HandleInputPressed(AgGameplayTags::Input_Execute);
}

void AAgPlayerCharacter::LockOnPressed()
{
	HandleInputPressed(AgGameplayTags::Input_LockOn);
}

void AAgPlayerCharacter::HandleInputPressed(const FGameplayTag& InputTag)
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
		// The lock-on key isn't limited by the action state and isn't buffered; it works until death
		// (플레이어 입력 '공통 입력 처리').
		RunInput(InputTag);
		return;
	}

	// 선입력: a key pressed while actions aren't allowed is remembered and runs once they are. A new press replaces it.
	BufferedInput = IsActionInputBlocked() ? InputTag : FGameplayTag();
	BufferedInputTime = GetWorld()->GetTimeSeconds();

	// The key goes in now as well: an action that isn't allowed yet doesn't start, but a running ability may still take
	// it (the guard follows its key during the parry motion).
	RunInput(InputTag);
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	// A running ability that handles this key itself gets the press: the basic attack's next hit, or the guard's new
	// start during a parry motion. Otherwise the key starts its action, which checks whether it is allowed now.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	auto PressOrActivate = [this, ASC, &InputTag](const FGameplayTag& AbilityTag)
	{
		const FGameplayTagContainer AbilityTags(AbilityTag);
		TArray<FGameplayAbilitySpec*> Specs;
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, Specs, false);
		if (Specs.ContainsByPredicate([](const FGameplayAbilitySpec* Spec) { return Spec->IsActive(); }))
		{
			FGameplayEventData Payload;
			Payload.EventTag = InputTag;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, InputTag, Payload);
		}
		else
		{
			ASC->TryActivateAbilitiesByTag(AbilityTags);
		}
	};

	if (InputTag == AgGameplayTags::Input_Attack)
	{
		// 평타 키의 처리 순서 1: while the 회피 반격 기회 lasts, the key is the dodge counter (ignored if it can't start now).
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_DodgeCounterChance))
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_DodgeCounter));
		}
		else
		{
			PressOrActivate(AgGameplayTags::Ability_Action_BasicAttack);
		}
	}
	else if (InputTag == AgGameplayTags::Input_Guard)
	{
		PressOrActivate(AgGameplayTags::Ability_Action_Guard);
	}
	else if (InputTag == AgGameplayTags::Input_Dodge)
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Dodge));
	}
	else if (InputTag == AgGameplayTags::Input_Skill)
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Skill));
	}
	else if (InputTag == AgGameplayTags::Input_Ultimate)
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Ultimate));
	}
	else if (InputTag == AgGameplayTags::Input_Execute)
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Execution));
	}
	else if (InputTag == AgGameplayTags::Input_Jump)
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Jump));
	}
	else if (InputTag == AgGameplayTags::Input_LockOn)
	{
		// Locks on, or releases while locked on.
		PressOrActivate(AgGameplayTags::Ability_LockOn);
	}
}

void AAgPlayerCharacter::HandleInputReleased(const FGameplayTag& InputTag)
{
	if (InputTag == AgGameplayTags::Input_Guard && bGuardHeld)
	{
		bGuardHeld = false;
		FGameplayEventData Payload;
		Payload.EventTag = AgGameplayTags::Input_GuardReleased;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AgGameplayTags::Input_GuardReleased, Payload);
	}
	if (InputTag == AgGameplayTags::Input_Dodge)
	{
		bDodgeHeld = false;
	}
}

bool AAgPlayerCharacter::IsActionInputBlocked() const
{
	// The 상태별 허용 입력 rows without action keys: an action before its recovery (the air too) and the hit reactions
	// that stop actions.
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Busy);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	return GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Checked next frame, once whatever follows in this frame is set up (a knockback that cancelled the action, the
	// basic attack marking its recovery).
	if (NewCount == 0 && BufferedInput.IsValid())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunBufferedInput);
	}
}

void AAgPlayerCharacter::TryRunBufferedInput()
{
	// Still not allowed: the key waits for the next change and is dropped then if it has expired.
	if (!BufferedInput.IsValid() || IsActionInputBlocked())
	{
		return;
	}

	const FGameplayTag InputTag = BufferedInput;
	BufferedInput = FGameplayTag();
	const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData());
	const bool bExpired = !Data || GetWorld()->GetTimeSeconds() - BufferedInputTime > Data->InputBufferTime;
	// The guard runs only while its key is still held.
	if (IsDead() || bExpired || (InputTag == AgGameplayTags::Input_Guard && !bGuardHeld))
	{
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Buffered input runs: %s"), *InputTag.ToString());
	if (InputTag == AgGameplayTags::Input_Guard)
	{
		// A new press, so the guard opens a parry window even right after a guard pushback.
		bGuardResumeWithoutParry = false;
	}
	RunInput(InputTag);
}

void AAgPlayerCharacter::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	const UGameplayAbility* Ended = EndedData.AbilityThatEnded;
	if (IsDead() || !Ended)
	{
		return;
	}
	const FGameplayTagContainer& EndedTags = Ended->GetAssetTags();
	if (!bGuardHeld || EndedTags.HasTag(AgGameplayTags::Ability_Action_Guard) || (!EndedTags.HasTag(AgGameplayTags::Ability_Action) && !EndedTags.HasTag(AgGameplayTags::Ability_Reaction)))
	{
		return;
	}

	// After a guard pushback the guard comes back without a parry window. Check next frame, once whatever cancelled
	// this ability has started.
	bGuardResumeWithoutParry = EndedTags.HasTag(AgGameplayTags::Ability_Reaction_GuardPushback);
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryResumeGuard);
}

void AAgPlayerCharacter::TryResumeGuard()
{
	// A remembered key is a newer press than the held guard key, so it goes first.
	TryRunBufferedInput();

	// Only from standing or moving: nothing else may be running.
	FGameplayTagContainer Running;
	Running.AddTag(AgGameplayTags::State_Acting);
	Running.AddTag(AgGameplayTags::State_Busy);
	Running.AddTag(AgGameplayTags::State_HitReaction);
	Running.AddTag(AgGameplayTags::State_Guarding);
	Running.AddTag(AgGameplayTags::State_Execution_Executing);
	if (bGuardHeld && !IsDead() && !GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Running))
	{
		GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Guard));
	}
	bGuardResumeWithoutParry = false;
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action_Dodge);
	Enders.AddTag(AgGameplayTags::Ability_Action_Guard);
	Enders.AddTag(AgGameplayTags::Ability_Action_Jump);
	Enders.AddTag(AgGameplayTags::Ability_Action_Skill);
	Enders.AddTag(AgGameplayTags::Ability_Action_Ultimate);
	Enders.AddTag(AgGameplayTags::Ability_Action_Execution);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_KnockBack);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_KnockUp);
	if (Ability && Ability->GetAssetTags().HasAny(Enders))
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_DodgeCounterChance));
	}
}

void AAgPlayerCharacter::TryStartSprint()
{
	// With the guard key held the guard starts instead (플레이어 사양 '가드').
	if (!IsDead() && bDodgeHeld && !bGuardHeld && !MoveInputDirection.IsNearlyZero())
	{
		GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Sprint));
	}
}

void AAgPlayerCharacter::UpdateSprint()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting) && (!bDodgeHeld || MoveInputDirection.IsNearlyZero() || GetAttributeSet()->GetSP() <= 0.f))
	{
		const FGameplayTagContainer SprintTags(AgGameplayTags::Ability_Action_Sprint);
		ASC->CancelAbilities(&SprintTags);
	}
}

bool AAgPlayerCharacter::TakeGuardResume()
{
	const bool bResume = bGuardResumeWithoutParry;
	bGuardResumeWithoutParry = false;
	return bResume;
}

void AAgPlayerCharacter::UpdateRotationMode()
{
	// 이동: 락온 중에는 대상(카메라가 보는 쪽)을, 가드 중에는 대상이나 카메라 정면을 바라봅니다. 그 밖에는, 그리고 락온 중이라도
	// 달릴 때는 이동 방향을 바라봅니다. 피격 반응 중에는 공격 쪽으로 돈 방향을 유지합니다(전투 시스템 '피격 모션').
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bReacting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction);
	const bool bSprinting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting);
	const bool bFaceCamera = (LockOn->IsLockedOn() && !bSprinting) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
	GetCharacterMovement()->bOrientRotationToMovement = !bReacting && !bFaceCamera;
	GetCharacterMovement()->bUseControllerDesiredRotation = !bReacting && bFaceCamera;
}

void AAgPlayerCharacter::HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateRotationMode();
	UpdateMoveSpeed();
}

float AAgPlayerCharacter::GetMoveSpeedMultiplier() const
{
	const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData());
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!Data)
	{
		return 1.f;
	}
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding))
	{
		return Data->GuardMoveSpeedRatio;
	}
	return ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting) ? Data->SprintSpeedRatio : 1.f;
}

void AAgPlayerCharacter::StartRegen()
{
	Super::StartRegen();

	if (const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData()))
	{
		UAgCombatLibrary::ApplyRegen(this, UAgGameplayEffect_SPRegen::StaticClass(), AgGameplayTags::Data_SP, Data->SPRegenRate);
		UAgCombatLibrary::ApplyRegen(this, UAgGameplayEffect_SPRegenGuarding::StaticClass(), AgGameplayTags::Data_SP, Data->SPRegenRateGuarding);
		UAgCombatLibrary::ApplyRegen(this, UAgGameplayEffect_SprintSPCost::StaticClass(), AgGameplayTags::Data_SP, -Data->SprintSPCost);
	}
}

void AAgPlayerCharacter::HandleSPChanged(const FOnAttributeChangeData& Data)
{
	const UAgPlayerData* PlayerData = Cast<UAgPlayerData>(GetCharacterData());
	if (PlayerData && Data.NewValue < Data.OldValue)
	{
		RestartRegenDelay(SPRegenDelay, AgGameplayTags::State_Regen_SPDelay, Data.NewValue <= 0.f ? PlayerData->SPRegenDelayEmpty : PlayerData->SPRegenDelay);
	}
}

void AAgPlayerCharacter::PlayGuardFlinch()
{
	const UAgPlayerData* Data = Cast<UAgPlayerData>(GetCharacterData());
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (Data && Data->GuardFlinchMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(Data->GuardFlinchMontage);
	}
}

AAgBossCharacter* AAgPlayerCharacter::FindBoss(float Range) const
{
	const UTargetingPreset* Preset = UAgSettings::Get()->GetBossTargeting();
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
