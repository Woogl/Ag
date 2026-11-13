// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Camera/CameraComponent.h"
#include "Combat/AgCombatStatsComponent.h"
#include "Combat/AgTimeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"

AAgPlayerCharacter::AAgPlayerCharacter()
{
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

	CombatStats = CreateDefaultSubobject<UAgCombatStatsComponent>(TEXT("CombatStats"));
}

bool AAgPlayerCharacter::IsMoveInputIgnored() const
{
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Acting);
	Blockers.AddTag(AgGameplayTags::State_MoveLocked);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	Blockers.AddTag(AgGameplayTags::State_Groggy);
	Blockers.AddTag(AgGameplayTags::State_Execution);
	Blockers.AddTag(AgGameplayTags::State_Dead);
	return Super::IsMoveInputIgnored() || GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::FindInputAbilities(const FGameplayTag& InputTag, TArray<FGameplayAbilitySpecHandle>& OutHandles) const
{
	OutHandles.Reset();
	for (const FGameplayAbilitySpec& Spec : GetAbilitySystemComponent()->GetActivatableAbilities())
	{
		const FAgAbilityData* Ability = FindAbility(Spec.Handle);
		if (Ability && Ability->InputTag == InputTag)
		{
			OutHandles.Add(Spec.Handle);
		}
	}
}

void AAgPlayerCharacter::SwitchLockOnTarget(float Direction)
{
	const FGameplayAbilitySpec* Spec = GetAbilitySystemComponent()->FindSpecByTag(AgGameplayTags::Ability_LockOn);
	UAgGameplayAbility* LockOn = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	const AActor* Current = LockOn && LockOn->IsActive() ? LockOn->GetActionTarget() : nullptr;
	if (!Current || !LockOn->GetDefinition().Targeting)
	{
		return;
	}
	if (AActor* Next = AgTargeting::FindSideTarget(*LockOn->GetDefinition().Targeting, *this, *Current, Direction))
	{
		LockOn->SetActionTarget(Next);
	}
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGuardingChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_LockedOn, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Busy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Airborne, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_PerfectDodgeWindow, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandlePerfectDodgeWindowChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_TimeStop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleTimeStopChanged);
	PerfectDodgeCapsule->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	UpdateRotationMode();
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
		{Skill1Action, AgGameplayTags::Input_Skill1},
		{Skill2Action, AgGameplayTags::Input_Skill2},
		{Skill3Action, AgGameplayTags::Input_Skill3},
		{Skill4Action, AgGameplayTags::Input_Skill4},
		{UltimateAction, AgGameplayTags::Input_Ultimate},
		{ExecuteAction, AgGameplayTags::Input_Execute},
		{LockOnAction, AgGameplayTags::Input_LockOn},
		{PotionAction, AgGameplayTags::Input_Potion},
	};
	for (const TPair<const UInputAction*, FGameplayTag>& ActionKey : ActionKeys)
	{
		EnhancedInput->BindAction(ActionKey.Key, ETriggerEvent::Started, this, &ThisClass::HandleInputPressed, ActionKey.Value);
		EnhancedInput->BindAction(ActionKey.Key, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased, ActionKey.Value);
	}
}

bool AAgPlayerCharacter::CanJumpInternal_Implementation() const
{
	const UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	const UGameplayAbility* Ability = Spec ? Spec->GetPrimaryInstance() : nullptr;
	return Super::CanJumpInternal_Implementation() && Ability && Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get());
}

void AAgPlayerCharacter::ApplyDeathState()
{
	Super::ApplyDeathState();

	MoveReleased();
	HeldInputs.Reset();
	BufferedInput = FGameplayTag();
}

void AAgPlayerCharacter::UpdateRotationMode()
{
	// 락온 카메라는 대상을 보므로, 카메라 정면을 바라보면 대상을 바라봅니다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bSprinting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting);
	const bool bLockedOn = ASC->HasMatchingGameplayTag(AgGameplayTags::State_LockedOn);
	const bool bFaceCamera = (bLockedOn && !bSprinting) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
	GetCharacterMovement()->bOrientRotationToMovement = !bFaceCamera;
	GetCharacterMovement()->bUseControllerDesiredRotation = bFaceCamera;
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || IsDead())
	{
		return;
	}

	// 회피가 누른 키 쪽으로 미끄러지고 Input.Move로 회피 섹션을 고르므로, 이동을 무시할 때도 방향과 태그는 정합니다.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	MoveInputDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(AgGameplayTags::Input_Move, MoveInputDirection.IsNearlyZero() ? 0 : 1);

	// 키를 누르는 동안 매 프레임 불리므로 후딜이 시작되는 순간을 따로 알리지 않아도 됩니다. 풀린 이동이 이번 프레임부터 먹도록 먼저 봅니다.
	TryMoveCancel();

	AddMovementInput(Forward, Input.Y);
	AddMovementInput(Right, Input.X);
}

void AAgPlayerCharacter::MoveReleased()
{
	MoveInputDirection = FVector::ZeroVector;
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(AgGameplayTags::Input_Move, 0);
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
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
	HeldInputs.Add(InputTag);
	if (InputTag == AgGameplayTags::Input_LockOn)
	{
		ToggleLockOn();
		return;
	}

	// 새로 누른 키가 기억한 입력을 대신합니다. 막히지 않은 키는 바로 실행하고, 막힌 키는 기억합니다.
	BufferedInput = FGameplayTag();
	if (!IsActionInputBlocked(InputTag))
	{
		RunInput(InputTag);
		return;
	}
	const bool bFallingJump = InputTag == AgGameplayTags::Input_Jump && GetCharacterMovement()->IsFalling();
	const bool bUnbuffered = bFallingJump || InputTag == AgGameplayTags::Input_Execute || InputTag == AgGameplayTags::Input_Potion;
	if (!bUnbuffered)
	{
		BufferedInput = InputTag;
		BufferedInputTime = GetWorld()->GetTimeSeconds();
	}
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	if (InputTag == AgGameplayTags::Input_Jump)
	{
		Jump();
		return;
	}
	// 같은 키의 어빌리티는 발동 조건이 서로 배타적이라 하나만 발동합니다. 발동한 것이 없으면 실행 중인 행동이 이 입력을
	// 기다리는 것이므로(기본 공격의 다음 타) 이벤트로 알립니다.
	if (!ActivateInputAbilities(InputTag))
	{
		FGameplayEventData Payload;
		Payload.EventTag = InputTag;
		Payload.Instigator = this;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, InputTag, Payload);
	}
}

bool AAgPlayerCharacter::ActivateInputAbilities(const FGameplayTag& InputTag)
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	FindInputAbilities(InputTag, Handles);
	bool bActivated = false;
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		bActivated = GetAbilitySystemComponent()->TryActivateAbility(Handle) || bActivated;
	}
	return bActivated;
}

void AAgPlayerCharacter::HandleInputReleased(FGameplayTag InputTag)
{
	if (HeldInputs.Remove(InputTag) > 0)
	{
		TArray<FGameplayAbilitySpecHandle> Handles;
		FindInputAbilities(InputTag, Handles);
		for (const FGameplayAbilitySpecHandle Handle : Handles)
		{
			const FAgAbilityData* Ability = FindAbility(Handle);
			if (Ability && Ability->End == EAgAbilityEnd::InputReleased)
			{
				GetAbilitySystemComponent()->CancelAbilityHandle(Handle);
			}
		}
	}
	if (InputTag == AgGameplayTags::Input_Jump)
	{
		StopJumping();
	}
	else if (InputTag == AgGameplayTags::Input_Dodge)
	{
		GetAbilitySystemComponent()->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Sprint));
	}
}

bool AAgPlayerCharacter::IsReleaseInput(const FGameplayTag& InputTag) const
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	FindInputAbilities(InputTag, Handles);
	return Handles.ContainsByPredicate([this](FGameplayAbilitySpecHandle Handle)
	{
		const FAgAbilityData* Ability = FindAbility(Handle);
		return Ability && Ability->End == EAgAbilityEnd::InputReleased;
	});
}

void AAgPlayerCharacter::ActivateByTag(const FGameplayTag& AbilityTag)
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AbilityTag))
	{
		ASC->TryActivateAbility(Spec->Handle);
	}
}

bool AAgPlayerCharacter::IsActionInputBlocked(const FGameplayTag& InputTag) const
{
	const UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy))
	{
		return true;
	}
	// 점프 행동은 공중 태그가 붙는 순간에만 발동을 시도해 다른 행동·피격 반응 도중 뜨면 실행되지 않으므로,
	// 공중 입력 제한은 점프 행동이 아니라 공중 태그로 봅니다.
	return InputTag != AgGameplayTags::Input_Attack && ASC->HasMatchingGameplayTag(AgGameplayTags::State_Airborne);
}

void AAgPlayerCharacter::HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 이 프레임의 피격 반응 재발동, 다음 타 전환 시작, 무기 판정이 끝난 뒤의 상태를 보려고 타이머에서 확인합니다.
	if (NewCount == 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::HandleInputUnblocked);
	}
}

void AAgPlayerCharacter::HandleInputUnblocked()
{
	// 기억한 입력이 가드 자세로 돌아오는 것보다 우선합니다.
	TryRunBufferedInput();
	ReturnToGuardFromParry();
}

bool AAgPlayerCharacter::HasBufferedInput() const
{
	return BufferedInput.IsValid() && GetWorld()->GetTimeSeconds() - BufferedInputTime <= InputBufferTime;
}

void AAgPlayerCharacter::TryRunBufferedInput()
{
	if (!HasBufferedInput() || IsActionInputBlocked(BufferedInput))
	{
		return;
	}

	const FGameplayTag InputTag = BufferedInput;
	BufferedInput = FGameplayTag();
	if (IsDead() || (IsReleaseInput(InputTag) && !HeldInputs.Contains(InputTag)))
	{
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Buffered input runs: %s"), *InputTag.ToString());
	RunInput(InputTag);
}

void AAgPlayerCharacter::ReturnToGuardFromParry()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Guard = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Guard);
	if (!IsDead() && Guard && Guard->IsActive() && !IsActionInputBlocked(AgGameplayTags::Input_Guard))
	{
		// 패리 모션은 가드를 끝내지 않으므로, 가드 중이면 후딜에 든 패리 모션만 끝내 가드 자세로 돌아옵니다.
		ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Parry));
	}
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	// 엔진 PreActivate 안에서 ActivateAbility보다 먼저 오는 알림이라, 극한 회피 모션이 자기 발동 Task로 주는 회피 반격 기회는 지워지지 않습니다.
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockDown);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp);
	if (const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability); AgAbility && AgAbility->GetDefinition().AbilityTags.HasAny(Enders))
	{
		FGameplayTagContainer Removed;
		Removed.AddTag(AgGameplayTags::State_DodgeCounterChance);
		Removed.AddTag(AgGameplayTags::State_GuardCounterChance);
		// 기본 공격은 발동한 뒤에 이 태그를 보고 시작할 타를 고르므로 남겨 둡니다.
		if (!AgAbility->GetDefinition().AbilityTags.HasTag(AgGameplayTags::Ability_Action_BasicAttack))
		{
			Removed.AddTag(AgGameplayTags::State_NextAttack2);
		}
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(Removed);
	}
}

void AAgPlayerCharacter::TryMoveCancel()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	// 기억한 입력은 후딜이 시작된 뒤 타이머에서 실행되므로, 남아 있는 동안 기다려 그 입력이 먼저 나가게 합니다.
	if (!ASC->HasMatchingGameplayTag(AgGameplayTags::State_MoveCancel) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy) ||
		MoveInputDirection.IsNearlyZero() || HasBufferedInput())
	{
		return;
	}
	if (HeldInputs.Contains(AgGameplayTags::Input_Dodge))
	{
		ActivateByTag(AgGameplayTags::Ability_Sprint);
	}
	else
	{
		ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action));
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

void AAgPlayerCharacter::HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateRotationMode();
}

void AAgPlayerCharacter::HandleGuardingChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 가드로 막아 건 이동 제한은 가드 어빌리티가 걸지 않아 가드가 끝나도 남으므로 여기서 함께 끝냅니다.
	if (NewCount == 0)
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_MoveLocked));
	}
	UpdateRotationMode();
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
	}
	else
	{
		TimeSubsystem->EndTimeStop(*this);
	}
}
