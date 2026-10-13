// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/Abilities/AgAbility_Execution.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "AI/AgBossAIController.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/WidgetComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AAgBossCharacter::AAgBossCharacter()
{
	AIControllerClass = AAgBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 보스는 걷는 방향이 아니라 AI의 대상을 향해 스스로 돕니다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 화면 공간이라 거리와 관계없이 크기가 같습니다. 데이터의 본에는 BeginPlay에서 붙입니다.
	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	for (UWidgetComponent* WorldWidget : { LockOnMarker.Get(), ExecutionPrompt.Get() })
	{
		WorldWidget->SetupAttachment(GetMesh());
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	// 처형 안내는 본 바로 위(보스 머리 위)에 섭니다.
	ExecutionPrompt->SetPivot(FVector2D(0.5f, 1.f));
}

void AAgBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	AttachWorldWidgets();
}

void AAgBossCharacter::AttachWorldWidgets()
{
	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	LockOnMarker->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->LockOnBone);
	ExecutionPrompt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->ExecutionPromptBone);
}

void AAgBossCharacter::GrantAbilities()
{
	Super::GrantAbilities();

	// 패턴 어빌리티는 부모가 부여합니다. 몽타주도 패턴 ID로 찾으므로, DA에 빠졌으면 여기서 알립니다.
	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	for (const FAgBossPattern& Pattern : Data->Patterns)
	{
		if (!Data->FindPatternMontage(Pattern.Id))
		{
			UE_LOG(LogAg, Error, TEXT("%s: pattern %s has no montage in %s's PatternMontages."), *GetName(), *Pattern.Id.ToString(), *Data->GetName());
		}
	}
}

const FAgBossPattern* AAgBossCharacter::FindPattern(const FGameplayAbilitySpecHandle Handle) const
{
	const FName Id = FindActionId(Handle);
	return Id.IsNone() ? nullptr : CastChecked<UAgBossData>(GetCharacterData())->FindPattern(Id);
}

void AAgBossCharacter::EnterPhase2()
{
	Phase = 2;
}

void AAgBossCharacter::SetGroggyTimerRunning(bool bRunning)
{
	GroggyStartTime = bRunning ? GetWorld()->GetTimeSeconds() : -1.0;
}

float AAgBossCharacter::GetGroggyElapsedTime() const
{
	return GroggyStartTime >= 0.0 ? static_cast<float>(GetWorld()->GetTimeSeconds() - GroggyStartTime) : -1.f;
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
}

AAgCharacterBase* AAgBossCharacter::GetTarget() const
{
	const AAgBossAIController* AIController = Cast<AAgBossAIController>(GetController());
	return AIController ? AIController->GetTarget() : nullptr;
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// 둘 다 플레이어나 보스의 사망 연출이 시작되는 순간부터, 처형 중, 궁극기 컷신(플레이어 컨트롤러의 시네마틱 모드) 동안
	// 숨깁니다. 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const bool bInCombat = Player && !Player->IsDead() && !IsDead()
		&& !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed)
		&& !(PlayerController && PlayerController->bCinematicMode);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOn()->GetTarget() == this));

	// 처형 발동 조건 1, 2(그로기, 범위 안)를 만족하는 처형 대상이면 표시합니다. 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도
	// 표시합니다. 범위 확인은 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && UAgAbility_Execution::FindExecutionTarget(Player) == this));
}

bool AAgBossCharacter::IsFacingFixed() const
{
	FGameplayTagContainer NoTurning;
	NoTurning.AddTag(AgGameplayTags::State_HitReaction);
	NoTurning.AddTag(AgGameplayTags::State_Groggy);
	NoTurning.AddTag(AgGameplayTags::State_Execution_Executed);
	NoTurning.AddTag(AgGameplayTags::State_Boss_RotationLocked);
	NoTurning.AddTag(AgGameplayTags::State_NonCombat);
	return Super::IsFacingFixed() || GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(NoTurning);
}
