// Copyright Woogle. All Rights Reserved.

#include "Camera/AgPlayerCameraManager.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Camera/AgExecutionCameraModifier.h"
#include "Camera/AgFieldOfViewCameraModifier.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCameraData.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

namespace AgPlayerCameraManagerPrivate
{
	// 기획에서 자주 고칠 값이 아니라 데이터 대신 코드에 둡니다.
	constexpr double LockOnFlickTime = 0.1;
	constexpr float LockOnFlickAngle = 30.f;
	constexpr float LockOnFlickRestAngle = 5.f;

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

AAgPlayerCameraManager::AAgPlayerCameraManager()
{
	// 스프링 암(TG_PostPhysics)보다 앞이라 바꾼 회전이 그 프레임 화면에 보입니다. 뒤로 옮기면 프레임 간격이 고르지 않을 때
	// 대상이 흔들립니다. 캐릭터 이동과의 순서는 회전 스무딩이 걸러 주므로 정하지 않습니다.
	PrimaryActorTick.bCanEverTick = true;

	DefaultModifiers.Add(UAgExecutionCameraModifier::StaticClass());
	DefaultModifiers.Add(UAgFieldOfViewCameraModifier::StaticClass());
}

void AAgPlayerCameraManager::InitializeFor(APlayerController* PC)
{
	Super::InitializeFor(PC);
	PC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	const AAgPlayerCharacter* Player = GetPlayerCharacter();
	SetObservedAbilitySystem(Player ? Player->GetAbilitySystemComponent() : nullptr);
}

void AAgPlayerCameraManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AAgPlayerCharacter* Player = GetPlayerCharacter())
	{
		const FVector PivotLocation = Player->GetCameraBoom()->GetComponentLocation();
		AAgCharacterBase* Target = Player->GetLockOnTarget();
		if (Target != PreviousLockOnTarget.Get())
		{
			PreviousLockOnTarget = Target;
			LockOnPitchRate = 0.f;
			LockOnYawRate = 0.f;
		}
		if (Target)
		{
			UpdateLockOnCamera(*Player, *Target, PivotLocation, DeltaSeconds);
		}
		UpdateCameraTurn(*Player, PivotLocation, DeltaSeconds);
	}
}

void AAgPlayerCameraManager::ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	// 엔진은 입력이 없는 프레임에도 부르므로, 판정과 취소는 입력이 있을 때만 합니다.
	AAgPlayerCharacter* Player = GetPlayerCharacter();
	if (Player && !OutDeltaRot.IsZero())
	{
		const bool bLockedOn = Player->GetLockOnTarget() != nullptr;
		const bool bCutscene = Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Cutscene);
		if (bLockedOn && !IsExecutionCameraActive() && !bCutscene)
		{
			UpdateLockOnFlick(*Player, static_cast<float>(OutDeltaRot.Yaw));
		}
		else
		{
			FlickSamples.Reset();
			FlickDirection = 0.f;
		}
		if (bLockedOn || IsExecutionCameraActive())
		{
			OutDeltaRot = FRotator::ZeroRotator;
		}
		else
		{
			TurnOpponent.Reset();
		}
	}
	Super::ProcessViewRotation(DeltaTime, OutViewRotation, OutDeltaRot);
}

void AAgPlayerCameraManager::PlayCameraReaction(EAgCameraSituation Situation, const AActor* TurnTarget)
{
	const FAgCameraReactionSettings* ReactionSettings = UAgAssetManager::Get().GetCameraData().ReactionsBySituation.Find(Situation);
	const AAgPlayerCharacter* Player = GetPlayerCharacter();
	if (!ReactionSettings || !Player)
	{
		return;
	}
	for (const TSubclassOf<UCameraShakeBase>& Shake : ReactionSettings->CameraShakes)
	{
		StartCameraShake(Shake);
	}
	FieldOfViewCamera->Play(ReactionSettings->FieldOfView);
	if (ReactionSettings->bTurnToOpponent && TurnTarget)
	{
		StartCameraTurn(*Player, *TurnTarget, ReactionSettings->TurnAngle);
	}
}

void AAgPlayerCameraManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ExecutionCamera = CastChecked<UAgExecutionCameraModifier>(FindCameraModifierByClass(UAgExecutionCameraModifier::StaticClass()));
	FieldOfViewCamera = CastChecked<UAgFieldOfViewCameraModifier>(FindCameraModifierByClass(UAgFieldOfViewCameraModifier::StaticClass()));
}

void AAgPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PCOwner)
	{
		PCOwner->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	SetObservedAbilitySystem(nullptr);
	Super::EndPlay(EndPlayReason);
}

AAgPlayerCharacter* AAgPlayerCameraManager::GetPlayerCharacter() const
{
	return PCOwner ? PCOwner->GetPawn<AAgPlayerCharacter>() : nullptr;
}

void AAgPlayerCameraManager::SetObservedAbilitySystem(UAgAbilitySystemComponent* NewAbilitySystem)
{
	if (NewAbilitySystem && ObservedAbilitySystem.Get() == NewAbilitySystem)
	{
		return;
	}
	if (UAgAbilitySystemComponent* ASC = ObservedAbilitySystem.Get())
	{
		ASC->AbilityCommittedCallbacks.RemoveAll(this);
		ASC->OnAbilityEnded.RemoveAll(this);
		ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		ASC->RegisterGameplayTagEvent(AgGameplayTags::Input_Move, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Cutscene, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}
	ResetCameraState();
	ObservedAbilitySystem = NewAbilitySystem;
	if (!NewAbilitySystem)
	{
		return;
	}

	UAgAbilitySystemComponent* ASC = NewAbilitySystem;
	ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleAbilityCommitted);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Sprinting, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleSprintFieldOfViewChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::Input_Move, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleSprintFieldOfViewChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Cutscene, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleCutsceneChanged);
	UpdateSprintFieldOfView();
	HandleCutsceneChanged(AgGameplayTags::State_Cutscene, ASC->GetTagCount(AgGameplayTags::State_Cutscene));
}

void AAgPlayerCameraManager::ResetCameraState()
{
	ExecutionCamera->DisableModifier(true);
	FieldOfViewCamera->StopAll();
	PreviousLockOnTarget.Reset();
	TurnOpponent.Reset();
	FlickSamples.Reset();
	FlickDirection = 0.f;
}

void AAgPlayerCameraManager::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(NewPawn);
	SetObservedAbilitySystem(Player ? Player->GetAbilitySystemComponent() : nullptr);
}

void AAgPlayerCameraManager::UpdateLockOnCamera(AAgPlayerCharacter& Player, const AAgCharacterBase& Target, const FVector& PivotLocation, float DeltaTime)
{
	const FAgLockOnCameraSettings& Settings = UAgAssetManager::Get().GetCameraData().LockOn;
	FRotator Goal = (Target.GetTargetLocation(&Player) - PivotLocation).Rotation();
	Goal.Pitch =
		FMath::Clamp(FRotator::NormalizeAxis(Goal.Pitch), Settings.PitchMin, Settings.PitchMax) - Settings.LookDown;

	const FRotator Current = PCOwner->GetControlRotation();
	const float CurrentPitch = static_cast<float>(FRotator::NormalizeAxis(Current.Pitch));
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float GoalYaw = CurrentYaw + static_cast<float>(FMath::FindDeltaAngleDegrees(Current.Yaw, Goal.Yaw));
	const float Pitch = AgPlayerCameraManagerPrivate::SpringToward(CurrentPitch, LockOnPitchRate, static_cast<float>(Goal.Pitch), DeltaTime,
		Settings.SmoothingTime, Settings.RotationSpeed);
	const float Yaw =
		AgPlayerCameraManagerPrivate::SpringToward(CurrentYaw, LockOnYawRate, GoalYaw, DeltaTime, Settings.SmoothingTime, Settings.RotationSpeed);
	PCOwner->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void AAgPlayerCameraManager::UpdateLockOnFlick(AAgPlayerCharacter& Player, float YawDelta)
{
	const double Now = GetWorld()->GetRealTimeSeconds();
	FlickSamples.RemoveAll([Now](const TPair<double, float>& Sample)
	{ return Now - Sample.Key > AgPlayerCameraManagerPrivate::LockOnFlickTime; });
	float Angle = 0.f;
	for (const TPair<double, float>& Sample : FlickSamples)
	{
		Angle += Sample.Value;
	}
	// 판정 각도 아래로만 내려가도 다시 받으면 한 번 튕기는 도중 속도가 출렁일 때 두 번 바뀌므로, 거의 멈추거나 반대로
	// 움직여야 다시 받습니다. 새 입력을 더하기 전에 보아야 멈췄다가 다시 튕긴 첫 입력부터 판정합니다.
	if (Angle * FlickDirection < AgPlayerCameraManagerPrivate::LockOnFlickRestAngle)
	{
		FlickDirection = 0.f;
	}
	FlickSamples.Emplace(Now, YawDelta);
	Angle += YawDelta;
	if (FlickDirection == 0.f && FMath::Abs(Angle) >= AgPlayerCameraManagerPrivate::LockOnFlickAngle)
	{
		FlickDirection = FMath::Sign(Angle);
		Player.SwitchLockOnTarget(FlickDirection);
	}
}

void AAgPlayerCameraManager::StartCameraTurn(const AAgPlayerCharacter& Player, const AActor& Opponent, float Angle)
{
	// 같은 프레임에 여러 회전이 시작되면 처리 순서와 관계없이 가장 가까운 상대 쪽으로 돕니다.
	if (TurnStartFrame == GFrameCounter && TurnOpponent.IsValid() &&
		Player.GetSquaredDistanceTo(TurnOpponent.Get()) <= Player.GetSquaredDistanceTo(&Opponent))
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

void AAgPlayerCameraManager::UpdateCameraTurn(const AAgPlayerCharacter& Player, const FVector& PivotLocation, float DeltaTime)
{
	const AActor* Opponent = TurnOpponent.Get();
	if (!Opponent || Player.GetLockOnTarget() || IsExecutionCameraActive())
	{
		TurnOpponent.Reset();
		return;
	}

	const FRotator Current = PCOwner->GetControlRotation();
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float OpponentYaw = static_cast<float>((Opponent->GetActorLocation() - PivotLocation).Rotation().Yaw);
	const float Offset = FMath::FindDeltaAngleDegrees(CurrentYaw, OpponentYaw);
	// 감쇠 접근은 목표 경계에 끝없이 다가가므로, 1° 안으로 들면 끝냅니다.
	if (FMath::Abs(Offset) <= TurnAngle + 1.f)
	{
		TurnOpponent.Reset();
		return;
	}
	const float GoalYaw = CurrentYaw + Offset - FMath::Sign(Offset) * TurnAngle;
	const FAgLockOnCameraSettings& Settings = UAgAssetManager::Get().GetCameraData().LockOn;
	const float Yaw = AgPlayerCameraManagerPrivate::SpringToward(CurrentYaw, TurnYawRate, GoalYaw, DeltaTime, Settings.SmoothingTime, Settings.RotationSpeed);
	PCOwner->SetControlRotation(FRotator(Current.Pitch, Yaw, 0.f));
}

bool AAgPlayerCameraManager::IsExecutionCameraActive() const
{
	return !ExecutionCamera->IsDisabled();
}

void AAgPlayerCameraManager::HandleAbilityCommitted(UGameplayAbility* Ability)
{
	// 처형 대상은 발동 중(ActivateAbility)에 정해지므로, 발동 알림이 아니라 커밋 알림에서 읽습니다.
	const AAgPlayerCharacter* Player = GetPlayerCharacter();
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const AActor* Target = AgAbility ? AgAbility->GetActionTarget() : nullptr;
	if (Player && Target && AgAbility->GetDefinition().AbilityTags.HasTag(AgGameplayTags::Ability_Action_Execution))
	{
		ExecutionCamera->Start(*Player, *Target);
	}
}

void AAgPlayerCameraManager::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(EndedData.AbilityThatEnded);
	if (AgAbility && AgAbility->GetDefinition().AbilityTags.HasTag(AgGameplayTags::Ability_Action_Execution))
	{
		ExecutionCamera->Stop();
	}
}

void AAgPlayerCameraManager::UpdateSprintFieldOfView()
{
	// 달리기는 Shift만 누르고 멈춰 있어도 이어지므로, 이동 입력이 있을 때만 넓힙니다.
	const UAbilitySystemComponent& ASC = *ObservedAbilitySystem;
	if (ASC.HasMatchingGameplayTag(AgGameplayTags::State_Sprinting) && ASC.HasMatchingGameplayTag(AgGameplayTags::Input_Move))
	{
		FieldOfViewCamera->Hold(UAgAssetManager::Get().GetCameraData().SprintFieldOfView);
	}
	else
	{
		FieldOfViewCamera->Release();
	}
}

void AAgPlayerCameraManager::HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateSprintFieldOfView();
}

void AAgPlayerCameraManager::HandleCutsceneChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount == 0)
	{
		return;
	}
	// 셰이크는 컷신 카메라에도 더해지므로 진행 중인 연출을 끝냅니다.
	StopAllCameraShakes(true);
	FieldOfViewCamera->StopAll();
	TurnOpponent.Reset();
}
