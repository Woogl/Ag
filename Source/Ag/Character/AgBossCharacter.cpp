// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName LockOnBone(TEXT("spine_05"));
	const FName ExecutionPromptBone(TEXT("head"));
} // namespace

AAgBossCharacter::AAgBossCharacter()
{
	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	LockOnMarker->SetupAttachment(GetMesh(), LockOnBone);
	ExecutionPrompt->SetupAttachment(GetMesh(), ExecutionPromptBone);
	for (UWidgetComponent* WorldWidget : {LockOnMarker.Get(), ExecutionPrompt.Get()})
	{
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	ExecutionPrompt->SetPivot(FVector2D(0.5f, 1.f));
}

FVector AAgBossCharacter::GetLockOnLocation() const
{
	return GetMesh()->GetBoneIndex(LockOnBone) != INDEX_NONE ? GetMesh()->GetBoneLocation(LockOnBone) : GetActorLocation();
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
}

bool AAgBossCharacter::IsExecutionTargetOf(AAgPlayerCharacter& Player) const
{
	const UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(Player.GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Execution) : nullptr;
	const FAgAbilityData* Execution = Spec ? Player.FindAbility(Spec->Handle) : nullptr;
	return Execution && Execution->Targeting && AgTargeting::FindTarget(*Execution->Targeting, Player) == this;
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const bool bInCombat = Player && !Player->IsDead() && !IsDead() &&
	                       !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution) &&
	                       !(PlayerController && PlayerController->bCinematicMode);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOnTarget() == this));

	// 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도 표시하므로 발동 가능 여부는 보지 않습니다. 범위 확인은
	// 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && IsExecutionTargetOf(*Player)));
}
