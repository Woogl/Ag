// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PerfectDodgeWindow.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Components/BoxComponent.h"
#include "Core/AgCollision.h"
#include "Data/AgCombatRules.h"

UAbilityTask* FAgPerfectDodgeWindowParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_PerfectDodgeWindow* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_PerfectDodgeWindow>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_PerfectDodgeWindow::Activate()
{
	Super::Activate();
	if (IsFinished())
	{
		return;
	}
	AActor* Avatar = GetAvatarActor();
	Box = NewObject<UBoxComponent>(Avatar, NAME_None, RF_Transient);
	Box->SetBoxExtent(FVector(UAgCombatRules::Get().PerfectDodgeBoxSize * 0.5f), false);
	// 투사체와 충격파는 Pawn 오브젝트 종류로 찾고 응답은 보지 않으므로, 응답은 무기 판정 채널만 둡니다.
	Box->SetCollisionObjectType(ECC_Pawn);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->SetupAttachment(Avatar->GetRootComponent());
	Box->RegisterComponent();
}

void UAgAbilityTask_PerfectDodgeWindow::OnDestroy(bool bInOwnerFinished)
{
	if (Box)
	{
		Box->DestroyComponent();
		Box = nullptr;
	}
	Super::OnDestroy(bInOwnerFinished);
}
