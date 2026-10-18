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
#include "Camera/CameraComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCameraData.h"
#include "Combat/AgTimeSubsystem.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
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
	/**
	 * Angle을 임계 감쇠 스프링으로 Target 쪽으로 옮기되(부드럽게 가속·감속) MaxSpeed보다 빨리 돌지 않습니다. 스프링은
	 * 한 스텝이 SmoothingTime의 절반보다 짧을 때만 정확하므로 긴 프레임은 나눠 계산합니다.
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

const FName AAgPlayerCharacter::JumpMontageId(TEXT("Jump"));
const FName AAgPlayerCharacter::JumpStartSection(TEXT("Start"));
const FName AAgPlayerCharacter::JumpLandSection(TEXT("Land"));

AAgPlayerCharacter::AAgPlayerCharacter()
{
	CharacterDataClass = UAgPlayerData::StaticClass();

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

	TimeStopScreen = CreateDefaultSubobject<UPostProcessComponent>(TEXT("TimeStopScreen"));
	TimeStopScreen->SetupAttachment(RootComponent);
	TimeStopScreen->bUnbound = true;
	TimeStopScreen->BlendWeight = 0.f;
}

TConstArrayView<FGameplayTag> AAgPlayerCharacter::GetInputAbilityTags()
{
	// 키와 행동 태그의 대응(구현 계획서 '입력 계층의 행동 찾기')
	static const FGameplayTag Tags[] = {AgGameplayTags::Ability_Action_JumpAttack,
	                                    AgGameplayTags::Ability_Action_DodgeCounter,
	                                    AgGameplayTags::Ability_Action_BasicAttack,
	                                    AgGameplayTags::Ability_Action_Guard,
	                                    AgGameplayTags::Ability_Action_Dodge,
	                                    AgGameplayTags::Ability_Sprint,
	                                    AgGameplayTags::Ability_Action_Jump,
	                                    AgGameplayTags::Ability_Action_Skill,
	                                    AgGameplayTags::Ability_Action_Ultimate,
	                                    AgGameplayTags::Ability_Action_Execution,
	                                    AgGameplayTags::Ability_LockOn,
	                                    AgGameplayTags::Ability_Action_Parry};
	return Tags;
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleMoveStateChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Busy, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleInputBlockChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetSPAttribute()).AddUObject(this, &ThisClass::HandleSPChanged);
	UpdateRotationMode();
	GetWorld()->GetSubsystem<UAgTimeSubsystem>()->OnTimeStopChanged.AddUObject(this, &ThisClass::UpdateTimeStopScreen);
}

void AAgPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSprint();
	UpdateLockOn();

#if WITH_EDITORONLY_DATA
	// 누른 키는 다음 프레임에 뗍니다. 같은 프레임에 떼면 점프 키는 엔진 점프가 시작되기 전에 StopJumping이 누름을 지웁니다.
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

	// 행동 키를 누르면 그 입력 태그를 넘깁니다. 키를 뗀 것은 가드, 회피·달리기, 점프 키만 봅니다.
	const TPair<const UInputAction*, FGameplayTag> ActionKeys[] = {
	    {AttackAction, AgGameplayTags::Input_Attack},   {GuardAction, AgGameplayTags::Input_Guard},
	    {DodgeAction, AgGameplayTags::Input_Dodge},     {JumpAction, AgGameplayTags::Input_Jump},
	    {SkillAction, AgGameplayTags::Input_Skill},     {UltimateAction, AgGameplayTags::Input_Ultimate},
	    {ExecuteAction, AgGameplayTags::Input_Execute}, {LockOnAction, AgGameplayTags::Input_LockOn},
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

bool AAgPlayerCharacter::CanMove() const
{
	// 행동 모션, 피격 반응, 가드 시작 직후에는 움직이지 않습니다(플레이어 입력 '상태별 허용 입력', '모션 캔슬').
	FGameplayTagContainer Blockers;
	Blockers.AddTag(AgGameplayTags::State_Acting);
	Blockers.AddTag(AgGameplayTags::State_ParryWindow);
	Blockers.AddTag(AgGameplayTags::State_HitReaction);
	Blockers.AddTag(AgGameplayTags::State_Groggy);
	Blockers.AddTag(AgGameplayTags::State_Execution);
	Blockers.AddTag(AgGameplayTags::State_Dead);
	return !GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(Blockers);
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	// X는 오른쪽, Y는 앞(카메라 기준)입니다. 회피가 누른 키 쪽으로 구르므로 이동을 무시할 때도 방향은 기억하고, 이동 입력이
	// 있는 동안 Input.Move를 붙입니다(회피의 Roll 선택).
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	MoveInputDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal2D();
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(AgGameplayTags::Input_Move, MoveInputDirection.IsNearlyZero() ? 0 : 1);

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
	// 락온 카메라는 시야 입력을 무시합니다(카메라 '카메라 모드').
	if (GetLockOnTarget())
	{
		return;
	}

	// 지연 없이 바로 반영합니다(카메라 '자유 시점'의 회전 반응).
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AAgPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 점프 행동은 이 태그로 시작하고(트리거 태그), 행동이 다른 까닭으로 끝나도 착지할 때까지 남깁니다(점프 공격의 시작 조건).
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	ASC->SetLooseGameplayTagCount(AgGameplayTags::State_Jumping, 1);

	// 점프 행동이 시작했을 때만 점프 모션을 재생합니다. 키를 누른 뒤 실제 점프 전에 시작한 행동·피격 반응의 모션은 끊지 않습니다.
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

	// 착지 모션은 점프 행동이 남아 있을 때만입니다. 점프 공격이나 피격 반응이 점프 행동을 끝냈으면 그 행동의 모션이 착지를 맡습니다.
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
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

	// 제자리 착지: 점프 모션을 Land 섹션부터 다시 재생해, 남은 점프 모션은 블렌드 아웃되고 착지 모션이 블렌드 인합니다.
	if (AnimInstance->Montage_Play(JumpMontage) > 0.f)
	{
		AnimInstance->Montage_JumpToSection(JumpLandSection, JumpMontage);
	}
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
		ToggleLockOn();
		return;
	}

	// 선입력: 행동이 허용되지 않을 때 누른 키를 기억했다가 허용되면 실행합니다. 새로 누른 키가 대신합니다.
	BufferedInput = IsActionInputBlocked(InputTag) ? InputTag : FGameplayTag();
	BufferedInputTime = GetWorld()->GetTimeSeconds();

	// 키는 지금도 넣습니다. 아직 허용되지 않은 행동은 시작하지 않지만, 실행 중인 행동은 입력 이벤트를 받을 수 있습니다(평타의 다음 타).
	RunInput(InputTag);
}

void AAgPlayerCharacter::ActivateByTag(const FGameplayTag& AbilityTag)
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	if (const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AbilityTag))
	{
		ASC->TryActivateAbility(Spec->Handle);
	}
}

void AAgPlayerCharacter::RunInput(const FGameplayTag& InputTag)
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	if (InputTag == AgGameplayTags::Input_Attack)
	{
		// 평타 키의 처리 순서: 공중이면 점프 공격, 회피 반격 기회가 남아 있으면 회피 반격(지금 시작할 수 없으면 무시), 그 밖에는
		// 평타입니다. 평타가 실행 중이면 평타가 입력 이벤트로 다음 타를 정합니다.
		const FGameplayAbilitySpec* BasicAttack = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_BasicAttack);
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Jumping))
		{
			ActivateByTag(AgGameplayTags::Ability_Action_JumpAttack);
		}
		else if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_DodgeCounterChance))
		{
			ActivateByTag(AgGameplayTags::Ability_Action_DodgeCounter);
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
		SetTriggerTag(AgGameplayTags::Input_Guard, AgGameplayTags::Ability_Action_Guard, bGuardHeld);
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

void AAgPlayerCharacter::TryJump()
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Jump);
	const UGameplayAbility* Ability = Spec ? Spec->GetPrimaryInstance() : nullptr;
	if (Ability && Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get()))
	{
		Jump();
	}
}

void AAgPlayerCharacter::SetTriggerTag(const FGameplayTag& TriggerTag, const FGameplayTag& AbilityTag, bool bWant)
{
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC->FindSpecByTag(AbilityTag);
	const bool bRunning = Spec && Spec->IsActive();
	if (ASC->HasMatchingGameplayTag(TriggerTag))
	{
		// 3. 붙어 있는데 행동이 끝났으면 뗍니다. 다시 시작할 수 있게 되면 그때 다시 붙입니다.
		if (!bWant || !bRunning)
		{
			ASC->SetLooseGameplayTagCount(TriggerTag, 0);
		}
		return;
	}

	// 1. 행동을 시작할 수 있을 때만 붙입니다. 엔진은 태그가 붙는 순간에만 발동을 시도하고, 막히면 다시 시도하지 않습니다.
	const UGameplayAbility* Ability = Spec ? Spec->GetPrimaryInstance() : nullptr;
	if (!bWant || bRunning || !Ability || !Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get()))
	{
		return;
	}
	const FGameplayAbilitySpecHandle Handle = Spec->Handle;
	ASC->SetLooseGameplayTagCount(TriggerTag, 1);

	// 2. 붙였는데 행동이 시작하지 않았으면 바로 뗍니다.
	const FGameplayAbilitySpec* Started = ASC->FindAbilitySpecFromHandle(Handle);
	if (!Started || !Started->IsActive())
	{
		ASC->SetLooseGameplayTagCount(TriggerTag, 0);
	}
}

void AAgPlayerCharacter::HandleInputReleased(FGameplayTag InputTag)
{
	if (InputTag == AgGameplayTags::Input_Guard && bGuardHeld)
	{
		bGuardHeld = false;
		SetTriggerTag(AgGameplayTags::Input_Guard, AgGameplayTags::Ability_Action_Guard, false);
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
	if (NewCount != 0)
	{
		return;
	}
	if (Tag == AgGameplayTags::State_Busy && bGuardHeld)
	{
		// 가드 키를 누른 채 후딜이 시작되면 가드로 돌아옵니다. 기억한 입력은 그 안에서 먼저 처리합니다.
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryResumeGuard);
	}
	else if (BufferedInput.IsValid())
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
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(EndedData.AbilityThatEnded);
	if (IsDead() || !AgAbility)
	{
		return;
	}

	// 트리거 태그 규칙 3: 태그가 남아 있는데 그 행동이 다른 까닭(CancelTags 등)으로 끝나면 뗍니다.
	const FGameplayTagContainer& EndedTags = AgAbility->GetDefinitionTags();
	const TPair<FGameplayTag, FGameplayTag> TriggerTags[] = {{AgGameplayTags::Input_Guard, AgGameplayTags::Ability_Action_Guard},
	                                                         {AgGameplayTags::Input_Sprint, AgGameplayTags::Ability_Sprint},
	                                                         {AgGameplayTags::Input_LockOn, AgGameplayTags::Ability_LockOn}};
	for (const TPair<FGameplayTag, FGameplayTag>& Trigger : TriggerTags)
	{
		if (EndedTags.HasTagExact(Trigger.Value))
		{
			SetTriggerTag(Trigger.Key, Trigger.Value, false);
		}
	}
	if (!bGuardHeld || EndedTags.HasTag(AgGameplayTags::Ability_Action_Guard) ||
	    (!EndedTags.HasTag(AgGameplayTags::Ability_Action) && !EndedTags.HasTag(AgGameplayTags::Ability_Reaction)))
	{
		return;
	}

	// 이 행동을 취소한 행동이 시작된 뒤에 보도록 다음 프레임에 확인합니다.
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::TryResumeGuard);
}

void AAgPlayerCharacter::TryResumeGuard()
{
	// 기억한 입력은 누르고 있는 가드 키보다 나중에 누른 것이므로 먼저 처리합니다.
	TryRunBufferedInput();
	if (!bGuardHeld || IsDead())
	{
		return;
	}

	// 가드 중이면(패리 모션은 가드를 끝내지 않음) 후딜에 든 패리 모션을 끝내 가드 자세로 돌아옵니다.
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Guard = ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Guard);
	if (Guard && Guard->IsActive())
	{
		ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Action_Parry));
		return;
	}

	// 가드 입력이 허용될 때(후딜, 대기, 이동)만 시작합니다. 후딜이면 진행 중인 행동은 가드가 모션 캔슬합니다.
	FGameplayTagContainer Running;
	Running.AddTag(AgGameplayTags::State_Busy);
	Running.AddTag(AgGameplayTags::State_HitReaction);
	Running.AddTag(AgGameplayTags::State_Execution);
	if (!ASC->HasAnyMatchingGameplayTags(Running))
	{
		SetTriggerTag(AgGameplayTags::Input_Guard, AgGameplayTags::Ability_Action_Guard, true);
	}
}

void AAgPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	// 이동 외의 행동(회피 반격 포함)이나 넉업·넉백이 시작되면 회피 반격 기회가 사라집니다(플레이어 사양 '회피 반격 기회').
	FGameplayTagContainer Enders;
	Enders.AddTag(AgGameplayTags::Ability_Action);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockBack);
	Enders.AddTag(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp);
	if (const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability); AgAbility && AgAbility->GetDefinitionTags().HasAny(Enders))
	{
		GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_DodgeCounterChance));
	}
}

void AAgPlayerCharacter::UpdateSprint()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bWantsSprint = bDodgeHeld && !MoveInputDirection.IsNearlyZero() && GetAttributeSet()->GetSP() > 0.f;
	if (ASC->HasMatchingGameplayTag(AgGameplayTags::Input_Sprint))
	{
		// Shift를 떼거나 이동 입력이 없거나 SP가 0이 되면 끝냅니다.
		SetTriggerTag(AgGameplayTags::Input_Sprint, AgGameplayTags::Ability_Sprint, bWantsSprint);
		return;
	}

	// 회피 후딜(회피 중이고 행동 중 태그가 풀림)이면 달리기가 회피를 캔슬하고 시작합니다. 가드 키를 누르고 있으면
	// 가드를 따릅니다(플레이어 사양 '달리기', '가드').
	const FGameplayAbilitySpec* Dodge = CastChecked<UAgAbilitySystemComponent>(ASC)->FindSpecByTag(AgGameplayTags::Ability_Action_Dodge);
	if (bWantsSprint && !bGuardHeld && !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Busy) && Dodge && Dodge->IsActive())
	{
		SetTriggerTag(AgGameplayTags::Input_Sprint, AgGameplayTags::Ability_Sprint, true);
	}
}

void AAgPlayerCharacter::ToggleLockOn()
{
	const bool bLockedOn = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::Input_LockOn);
	SetTriggerTag(AgGameplayTags::Input_LockOn, AgGameplayTags::Ability_LockOn, !bLockedOn);
}

void AAgPlayerCharacter::UpdateLockOn()
{
	// 락온의 대상 상실은 트리거 태그의 주인이 판단해야 행동이 끝나므로 여기서 봅니다: 대상이 죽거나 해제 거리 밖으로 나가면 뗍니다.
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::Input_LockOn))
	{
		const UAgPlayerData* Data = CastChecked<UAgPlayerData>(GetCharacterData());
		const AAgCharacterBase* Target = GetLockOnTarget();
		const bool bKeep = Target && !Target->IsDead() && GetHorizontalDistanceTo(Target) <= Data->LockOnReleaseRange;
		SetTriggerTag(AgGameplayTags::Input_LockOn, AgGameplayTags::Ability_LockOn, bKeep);
	}

	// 락온이 시작되거나 끝나면 회전 방식을 바꾸고, 락온할 때마다 카메라 회전 속도는 0에서 시작합니다.
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

void AAgPlayerCharacter::UpdateLockOnCamera(const AAgCharacterBase& Target, float DeltaTime)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}
	const UAgCameraData& CameraData = UAgAssetManager::Get().GetCameraData();

	// 락온 지점: 보스 데이터의 본이고, 없으면 캐릭터 위치입니다.
	const UAgBossData* TargetData = Cast<UAgBossData>(Target.GetCharacterData());
	const bool bHasBone =
	    TargetData && !TargetData->LockOnBone.IsNone() && Target.GetMesh()->GetBoneIndex(TargetData->LockOnBone) != INDEX_NONE;
	const FVector LockOnPoint = bHasBone ? Target.GetMesh()->GetBoneLocation(TargetData->LockOnBone) : Target.GetActorLocation();

	// 구도: 회전 중심(스프링 암의 시작점)에서 락온 지점을 보되, 피치를 제한한 뒤 더 내려다봅니다.
	const FVector Pivot = CameraBoom->GetComponentLocation();
	FRotator Goal = (LockOnPoint - Pivot).Rotation();
	Goal.Pitch =
	    FMath::Clamp(FRotator::NormalizeAxis(Goal.Pitch), CameraData.LockOnPitchMin, CameraData.LockOnPitchMax) - CameraData.LockOnLookDown;
	Goal.Roll = 0.f;

	// 회전: 목표로 부드럽게 가속·감속하며 최대 속도보다 빨리 돌지 않으므로, 락온을 걸거나 머리 위로 돌진·도약이 지나가도
	// 화면이 튀지 않습니다. Yaw는 짧은 쪽으로 돕니다.
	const FRotator Current = PlayerController->GetControlRotation();
	const float CurrentPitch = static_cast<float>(FRotator::NormalizeAxis(Current.Pitch));
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float GoalYaw = CurrentYaw + static_cast<float>(FMath::FindDeltaAngleDegrees(Current.Yaw, Goal.Yaw));
	const float Pitch = SpringToward(CurrentPitch, LockOnPitchRate, static_cast<float>(Goal.Pitch), DeltaTime,
	                                 CameraData.LockOnSmoothingTime, CameraData.LockOnRotationSpeed);
	const float Yaw =
	    SpringToward(CurrentYaw, LockOnYawRate, GoalYaw, DeltaTime, CameraData.LockOnSmoothingTime, CameraData.LockOnRotationSpeed);
	PlayerController->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void AAgPlayerCharacter::UpdateRotationMode()
{
	// 이동: 락온 중에는 대상(카메라가 보는 쪽)을, 가드 중에는 대상이나 카메라 정면을 바라봅니다. 그 밖에는, 그리고 락온 중이라도
	// 달릴 때는 이동 방향을 바라봅니다. 피격 반응 중에는 공격 쪽으로 돈 방향을 유지합니다(전투 시스템 '피격 반응').
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bReacting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction);
	const bool bSprinting = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Sprinting);
	const bool bFaceCamera = (GetLockOnTarget() && !bSprinting) || ASC->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
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
	if (Data.NewValue < Data.OldValue)
	{
		RestartRegenDelay(SPRegenDelay, Data.NewValue <= 0.f ? EAgEffect::SPRegenDelayEmpty : EAgEffect::SPRegenDelay);
	}
}

AActor* AAgPlayerCharacter::GetFallbackFacingTarget() const
{
	return GetLockOnTarget();
}

AAgCharacterBase* AAgPlayerCharacter::GetLockOnTarget() const
{
	const UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn) : nullptr;
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	return Instance ? Cast<AAgCharacterBase>(Instance->GetActionTarget()) : nullptr;
}

void AAgPlayerCharacter::UpdateTimeStopScreen(bool bStopped)
{
	TimeStopScreen->BlendWeight = bStopped ? 1.f : 0.f;
}
