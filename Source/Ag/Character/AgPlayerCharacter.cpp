// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Character/AgLockOnComponent.h"
#include "Combat/AgTimeSubsystem.h"
#include "Components/PostProcessComponent.h"
#include "Core/AgGameplayTags.h"
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
	// 캐릭터는 이동 방향을 바라보고, 카메라는 마우스로 돌립니다(플레이어 사양 '이동').
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

	LockOn = CreateDefaultSubobject<UAgLockOnComponent>(TEXT("LockOn"));

	TimeStopScreen = CreateDefaultSubobject<UPostProcessComponent>(TEXT("TimeStopScreen"));
	TimeStopScreen->SetupAttachment(RootComponent);
	TimeStopScreen->bUnbound = true;
	TimeStopScreen->BlendWeight = 0.f;
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
	GetWorld()->GetSubsystem<UAgTimeSubsystem>()->OnTimeStopChanged.AddUObject(this, &ThisClass::HandleTimeStopChanged);
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

	// 행동 키를 누르면 그 입력 태그를 넘깁니다. 키를 뗀 것은 가드와 회피·달리기 키만 봅니다.
	const TPair<const UInputAction*, FGameplayTag> ActionKeys[] = {
		{ AttackAction, AgGameplayTags::Input_Attack },
		{ GuardAction, AgGameplayTags::Input_Guard },
		{ DodgeAction, AgGameplayTags::Input_Dodge },
		{ JumpAction, AgGameplayTags::Input_Jump },
		{ SkillAction, AgGameplayTags::Input_Skill },
		{ UltimateAction, AgGameplayTags::Input_Ultimate },
		{ ExecuteAction, AgGameplayTags::Input_Execute },
		{ LockOnAction, AgGameplayTags::Input_LockOn },
	};
	for (const TPair<const UInputAction*, FGameplayTag>& ActionKey : ActionKeys)
	{
		EnhancedInput->BindAction(ActionKey.Key, ETriggerEvent::Started, this, &ThisClass::HandleInputPressed, ActionKey.Value);
	}
	EnhancedInput->BindAction(GuardAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased, FGameplayTag(AgGameplayTags::Input_Guard));
	EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased, FGameplayTag(AgGameplayTags::Input_Dodge));
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

	// X는 오른쪽, Y는 앞(카메라 기준)입니다. 회피가 누른 키 쪽으로 구르므로 이동을 무시할 때도 방향은 기억합니다.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	MoveInputDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();

	// 이동 입력은 행동 모션을 캔슬하지 않고, 모션이 끝날 때까지 무시됩니다(플레이어 입력 '모션 캔슬').
	if (CanMove())
	{
		AddMovementInput(Forward, Input.Y);
		AddMovementInput(Right, Input.X);
		StopLandingMotion();
	}
}

void AAgPlayerCharacter::StopLandingMotion()
{
	// 착지 모션은 제자리 모션이라 움직이면 미끄러져 보입니다. 행동이 아니므로 움직이면 끝냅니다. 블렌드 아웃은 몽타주의 값입니다.
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(GetCharacterData());
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (Data->LandMontage && AnimInstance && AnimInstance->Montage_IsPlaying(Data->LandMontage))
	{
		AnimInstance->Montage_StopWithBlendOut(Data->LandMontage->GetBlendOutArgs(), Data->LandMontage);
	}
}

void AAgPlayerCharacter::MoveReleased()
{
	MoveInputDirection = FVector::ZeroVector;
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
	// 락온 카메라는 시야 입력을 무시합니다(카메라 '카메라 모드').
	if (LockOn->IsLockedOn())
	{
		return;
	}

	// 지연 없이 바로 반영합니다(카메라 '자유 시점'의 회전 반응).
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
		// 락온 키는 행동 상태의 제한을 받지 않고 기억하지도 않으며, 사망 전까지 동작합니다(플레이어 입력 '공통 입력 처리').
		RunInput(InputTag);
		return;
	}

	// 선입력: 행동이 허용되지 않을 때 누른 키를 기억했다가 허용되면 실행합니다. 새로 누른 키가 대신합니다.
	BufferedInput = IsActionInputBlocked(InputTag) ? InputTag : FGameplayTag();
	BufferedInputTime = GetWorld()->GetTimeSeconds();

	// 키는 지금도 넣습니다. 아직 허용되지 않은 행동은 시작하지 않지만, 실행 중인 어빌리티는 받을 수 있습니다(패리 모션 중의 가드).
	RunInput(InputTag);
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	// 이 키를 직접 받는 어빌리티가 실행 중이면 그 어빌리티가 받습니다(평타의 다음 타, 패리 모션 중의 새 가드 시작).
	// 아니면 키가 그 행동을 시작하고, 지금 허용되는지는 행동이 확인합니다.
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
		// 평타 키의 처리 순서: 공중이면 점프 공격, 회피 반격 기회가 남아 있으면 회피 반격(지금 시작할 수 없으면 무시), 그 밖에는 평타입니다.
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Jumping))
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_JumpAttack));
		}
		else if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_DodgeCounterChance))
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
		// 락온하거나, 락온 중이면 해제합니다.
		PressOrActivate(AgGameplayTags::Ability_LockOn);
	}
}

void AAgPlayerCharacter::HandleInputReleased(FGameplayTag InputTag)
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

bool AAgPlayerCharacter::IsActionInputBlocked(const FGameplayTag& InputTag) const
{
	// 상태별 허용 입력에서 행동 키가 없는 줄: 후딜 전의 행동(공중 포함)과 행동을 멈추는 피격 반응입니다. 공중에서는
	// 평타 키(점프 공격)만 허용됩니다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (InputTag == AgGameplayTags::Input_Attack && ASC->HasMatchingGameplayTag(AgGameplayTags::State_Jumping))
	{
		return false;
	}
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Busy);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	return ASC->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 이번 프레임에 이어지는 상태(행동을 끊은 피격 반응, 평타의 후딜 표시)가 정해진 뒤에 보도록 다음 프레임에 확인합니다.
	if (NewCount == 0 && BufferedInput.IsValid())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunBufferedInput);
	}
}

void AAgPlayerCharacter::TryRunBufferedInput()
{
	// 아직 허용되지 않으면 다음 변화를 기다리고, 그때 시간이 지났으면 버립니다.
	if (!BufferedInput.IsValid() || IsActionInputBlocked(BufferedInput))
	{
		return;
	}

	const FGameplayTag InputTag = BufferedInput;
	BufferedInput = FGameplayTag();
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(GetCharacterData());
	const bool bExpired = GetWorld()->GetTimeSeconds() - BufferedInputTime > Data->InputBufferTime;
	// 가드는 그때도 가드 키를 누르고 있을 때만 실행합니다.
	if (IsDead() || bExpired || (InputTag == AgGameplayTags::Input_Guard && !bGuardHeld))
	{
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Buffered input runs: %s"), *InputTag.ToString());
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

	// 이 어빌리티를 취소한 행동이 시작된 뒤에 보도록 다음 프레임에 확인합니다. 가드 밀림 뒤에는 패리 구간 없이 가드로 돌아옵니다.
	const bool bAfterGuardPushback = EndedTags.HasTag(AgGameplayTags::Ability_Reaction_HitReaction_GuardPushback);
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::TryResumeGuard, bAfterGuardPushback));
}

void AAgPlayerCharacter::TryResumeGuard(bool bAfterGuardPushback)
{
	// 기억한 입력은 누르고 있는 가드 키보다 나중에 누른 것이므로 먼저 처리합니다.
	TryRunBufferedInput();

	// 대기나 이동에서만 돌아옵니다. 다른 것이 돌고 있으면 안 됩니다.
	FGameplayTagContainer Running;
	Running.AddTag(AgGameplayTags::State_Acting);
	Running.AddTag(AgGameplayTags::State_Busy);
	Running.AddTag(AgGameplayTags::State_HitReaction);
	Running.AddTag(AgGameplayTags::State_Guarding);
	Running.AddTag(AgGameplayTags::State_Execution_Executing);
	if (!bGuardHeld || IsDead() || GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Running))
	{
		return;
	}

	// 가드 밀림 뒤에는 패리 구간 없이 가드로 돌아옵니다(플레이어 사양 '패리 구간'). 그때만 이벤트로 시작해 가드 어빌리티가 구분합니다.
	if (bAfterGuardPushback)
	{
		FGameplayEventData Payload;
		Payload.EventTag = AgGameplayTags::Event_Guard_Resume;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AgGameplayTags::Event_Guard_Resume, Payload);
	}
	else
	{
		GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Action_Guard));
	}
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	// 이동 외의 행동(회피 반격 포함)이나 넉업·넉백이 시작되면 회피 반격 기회가 사라집니다(플레이어 사양 '회피 반격 기회').
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockBack);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp);
	if (Ability && Ability->GetAssetTags().HasAny(Enders))
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_DodgeCounterChance));
	}
}

void AAgPlayerCharacter::UpdateSprint()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bWantsSprint = bDodgeHeld && !MoveInputDirection.IsNearlyZero();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting))
	{
		if (!bWantsSprint || GetAttributeSet()->GetSP() <= 0.f)
		{
			const FGameplayTagContainer SprintTags(AgGameplayTags::Ability_Sprint);
			ASC->CancelAbilities(&SprintTags);
		}
		return;
	}

	// 회피 후딜(회피 중이고 행동 중 태그가 풀림)이면 달리기가 회피를 캔슬하고 시작합니다. 가드 키를 누르고 있으면
	// 가드를 따릅니다(플레이어 사양 '달리기', '가드').
	if (!bWantsSprint || bGuardHeld || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy))
	{
		return;
	}
	TArray<FGameplayAbilitySpec*> DodgeSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Dodge), DodgeSpecs, false);
	if (DodgeSpecs.ContainsByPredicate([](const FGameplayAbilitySpec* Spec) { return Spec->IsActive(); }))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AgGameplayTags::Ability_Sprint));
	}
}

void AAgPlayerCharacter::UpdateRotationMode()
{
	// 이동: 락온 중에는 대상(카메라가 보는 쪽)을, 가드 중에는 대상이나 카메라 정면을 바라봅니다. 그 밖에는, 그리고 락온 중이라도
	// 달릴 때는 이동 방향을 바라봅니다. 피격 반응 중에는 공격 쪽으로 돈 방향을 유지합니다(전투 시스템 '피격 반응').
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
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(GetCharacterData());
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding))
	{
		return Data->GuardMoveSpeedRatio;
	}
	return ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting) ? Data->SprintSpeedRatio : 1.f;
}

void AAgPlayerCharacter::HandleSPChanged(const FOnAttributeChangeData& Data)
{
	const UAgPlayerData* PlayerData = CastChecked<UAgPlayerData>(GetCharacterData());
	if (Data.NewValue < Data.OldValue)
	{
		RestartRegenDelay(SPRegenDelay, AgGameplayTags::State_Regen_SPDelay, Data.NewValue <= 0.f ? PlayerData->SPRegenDelayEmpty : PlayerData->SPRegenDelay);
	}
}

AAgCharacterBase* AAgPlayerCharacter::FindAttackTarget() const
{
	return FindEnemy(TNumericLimits<float>::Max(), [](const AAgCharacterBase&) { return true; });
}

AActor* AAgPlayerCharacter::GetFallbackFacingTarget() const
{
	return LockOn->GetTarget();
}

AAgCharacterBase* AAgPlayerCharacter::FindEnemy(float Range, TFunctionRef<bool(const AAgCharacterBase&)> Condition) const
{
	// 기획서의 거리는 수평 거리라, 프리셋의 정렬(3D 거리)을 따르지 않고 가장 가까운 적을 직접 고릅니다.
	AAgCharacterBase* Nearest = nullptr;
	float NearestDistance = Range;
	for (AAgCharacterBase* Enemy : FindEnemies(Range))
	{
		const float Distance = GetHorizontalDistanceTo(Enemy);
		if (Distance <= NearestDistance && Condition(*Enemy))
		{
			Nearest = Enemy;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

TArray<AAgCharacterBase*> AAgPlayerCharacter::FindEnemies(float Range) const
{
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(GetCharacterData());
	UTargetingSubsystem* Targeting = UTargetingSubsystem::Get(GetWorld());
	if (!Data->EnemyTargeting || !Targeting)
	{
		UE_LOG(LogAg, Error, TEXT("Enemy targeting preset is not set (DA_Player > EnemyTargeting)."));
		return {};
	}

	FTargetingSourceContext Source;
	Source.SourceActor = const_cast<AAgPlayerCharacter*>(this);
	Source.InstigatorActor = Source.SourceActor;
	FTargetingRequestHandle Request = UTargetingSubsystem::MakeTargetRequestHandle(Data->EnemyTargeting, Source);
	Targeting->ExecuteTargetingRequestWithHandle(Request);
	TArray<AActor*> Candidates;
	Targeting->GetTargetingResultsActors(Request, Candidates);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Request);

	TArray<AAgCharacterBase*> Enemies;
	for (AActor* Candidate : Candidates)
	{
		AAgCharacterBase* Enemy = Cast<AAgCharacterBase>(Candidate);
		if (Enemy && !Enemy->IsDead() && GetTeamAttitudeTowards(*Enemy) == ETeamAttitude::Hostile && GetHorizontalDistanceTo(Enemy) <= Range)
		{
			Enemies.Add(Enemy);
		}
	}
	return Enemies;
}

void AAgPlayerCharacter::HandleTimeStopChanged(bool bStopped)
{
	TimeStopScreen->BlendWeight = bStopped ? 1.f : 0.f;
}
