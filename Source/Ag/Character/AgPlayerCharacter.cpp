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
#include "Camera/AgPlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Combat/AgTimeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

namespace
{
	const FName JumpLandSection(TEXT("Land"));
}

AAgPlayerCharacter::AAgPlayerCharacter()
{
	Team = EAgTeam::Ally;

	// 락온 카메라와 상대 쪽 회전이 이번 프레임의 이동을 마친 위치를 보도록 이동 뒤에 돕니다.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

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

AAgPlayerCameraManager* AAgPlayerCharacter::GetPlayerCameraManager() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	return PlayerController ? Cast<AAgPlayerCameraManager>(PlayerController->PlayerCameraManager) : nullptr;
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

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleMoveStateChanged);
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
	UpdateRotationMode();
	if (AAgPlayerCameraManager* CameraManager = GetPlayerCameraManager())
	{
		// 카메라는 캐릭터 히트스톱의 영향을 받지 않도록 월드 시간으로 갱신합니다.
		CameraManager->UpdateGameplayRotation(*this, CameraBoom->GetComponentLocation(), GetWorld()->GetDeltaSeconds());
	}

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
		EnhancedInput->BindAction(ActionKey.Key, ETriggerEvent::Completed, this, &ThisClass::HandleInputReleased, ActionKey.Value);
	}
}

void AAgPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 키를 누른 뒤 실제 점프 전에 시작한 행동·피격 반응의 모션을 끊지 않도록, 점프 행동이 실행 중일 때만 재생합니다.
	const FGameplayAbilitySpec* Jump = GetAbilitySystemComponent()->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
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
	// 엔진은 이동 모드를 바꾸기 전에 알리므로, 공중 태그로 끝나는 점프 행동이 아직 실행 중입니다.
	const FGameplayAbilitySpec* Jump = GetAbilitySystemComponent()->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	const bool bJumpAbility = Jump && Jump->IsActive();
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

	MoveReleased();
	HeldInputs.Reset();
	BufferedInput = FGameplayTag();
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
	if (AAgPlayerCameraManager* CameraManager = GetPlayerCameraManager())
	{
		CameraManager->ApplyLookInput(Value.Get<FVector2D>());
	}
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

	// 새로 누른 키가 기억한 입력을 대신하므로, 막히지 않은 키는 기억을 지웁니다.
	const bool bFallingJump = InputTag == AgGameplayTags::Input_Jump && GetCharacterMovement()->IsFalling();
	BufferedInput = !bFallingJump && InputTag != AgGameplayTags::Input_Execute && IsActionInputBlocked(InputTag)
	                    ? InputTag
	                    : FGameplayTag();
	BufferedInputTime = GetWorld()->GetTimeSeconds();

	RunInput(InputTag);
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	if (IsActionInputBlocked(InputTag))
	{
		return;
	}

	if (InputTag == AgGameplayTags::Input_Jump)
	{
		TryJump();
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
	// 키를 누르는 동안 이어지는 행동(가드)은 엔진 태그 트리거(OwnedTagPresent) 대신 직접 발동·취소합니다. 태그 트리거는
	// 태그가 붙는 순간 막혀 있으면 다시 시도하지 않아, 태그를 실행 상태와 맞추는 코드가 따로 필요해집니다.
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

bool AAgPlayerCharacter::IsHoldingReleaseInput() const
{
	for (const FGameplayTag& InputTag : HeldInputs)
	{
		if (IsReleaseInput(InputTag))
		{
			return true;
		}
	}
	return false;
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
	const UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction))
	{
		return true;
	}
	// 점프 행동의 Busy만 점프 공격을 막지 않습니다. 다른 행동 도중 걸어서 떨어졌으면 그 행동의 Busy가 막습니다.
	const FGameplayAbilitySpec* Jump = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	if (InputTag == AgGameplayTags::Input_Attack && Jump && Jump->IsActive())
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
	if (Tag == AgGameplayTags::State_Busy && IsHoldingReleaseInput())
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
	if (IsDead() || bExpired || (IsReleaseInput(InputTag) && !HeldInputs.Contains(InputTag)))
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
	if (IsDead() || !AgAbility)
	{
		return;
	}

	const FGameplayTagContainer& EndedTags = AgAbility->GetDefinition().AbilityTags;
	if (!IsHoldingReleaseInput() || AgAbility->GetDefinition().End == EAgAbilityEnd::InputReleased ||
		(!EndedTags.HasTag(AgGameplayTags::Ability_Action) && !EndedTags.HasTag(AgGameplayTags::Ability_Reaction)))
	{
		return;
	}

	// 이 행동을 취소한 행동이 시작된 뒤에 보도록 타이머 처리(TG_PostPhysics 뒤)에서 확인합니다.
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryRunPendingInput);
}

void AAgPlayerCharacter::TryRunPendingInput()
{
	// 기억한 입력은 누르고 있는 키보다 나중에 누른 것이므로 먼저 처리합니다.
	TryRunBufferedInput();
	if (IsDead())
	{
		return;
	}

	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	for (const FGameplayTag& InputTag : HeldInputs.Array())
	{
		if (!IsReleaseInput(InputTag) || IsActionInputBlocked(InputTag))
		{
			continue;
		}
		TArray<FGameplayAbilitySpecHandle> Handles;
		FindInputAbilities(InputTag, Handles);
		const bool bActive = Handles.ContainsByPredicate([ASC](FGameplayAbilitySpecHandle Handle)
		{
			const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
			return Spec && Spec->IsActive();
		});
		if (!bActive)
		{
			ActivateInputAbilities(InputTag);
		}
		else if (InputTag == AgGameplayTags::Input_Guard)
		{
			// 패리 모션은 가드를 끝내지 않으므로, 가드 중이면 후딜에 든 패리 모션만 끝내 가드 자세로 돌아옵니다.
			ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Parry));
		}
	}
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	// 엔진 PreActivate 안에서 ActivateAbility보다 먼저 오는 알림이라, 극한 회피 모션이 자기 발동 Task로 주는 회피 반격 기회는 지워지지 않습니다.
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockDown);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_GuardBreak);
	if (const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability); AgAbility && AgAbility->GetDefinition().AbilityTags.HasAny(Enders))
	{
		FGameplayTagContainer Removed;
		Removed.AddTag(AgGameplayTags::State_DodgeCounterChance);
		Removed.AddTag(AgGameplayTags::State_GuardCounterChance);
		// 기본 공격은 발동한 뒤에 이 태그를 보고 시작할 타를 고르므로 남겨 둡니다.
		if (!AgAbility->GetDefinition().AbilityTags.HasTag(AgGameplayTags::Ability_Action_BasicAttack))
		{
			Removed.AddTag(AgGameplayTags::State_GuardCounterRecovery);
		}
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(Removed);
	}
}

void AAgPlayerCharacter::UpdateSprint()
{
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bWantsSprint = HeldInputs.Contains(AgGameplayTags::Input_Dodge) && GetAttributeSet()->GetSP() > 0.f;
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
	const FGameplayAbilitySpec* PerfectDodge = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_PerfectDodge);
	const bool bDodging = (Dodge && Dodge->IsActive()) || (PerfectDodge && PerfectDodge->IsActive());
	if (bWantsSprint && !MoveInputDirection.IsNearlyZero() && !HeldInputs.Contains(AgGameplayTags::Input_Guard) &&
		!ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy) && bDodging)
	{
		ActivateByTag(AgGameplayTags::Ability_Sprint);
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
