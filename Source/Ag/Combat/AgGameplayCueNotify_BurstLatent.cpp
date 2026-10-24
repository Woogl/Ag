// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_BurstLatent.h"

#include "Ag.h"
#include "Camera/AgPostProcessComponent.h"
#include "Combat/AgTimeSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

AAgGameplayCueNotify_BurstLatent::AAgGameplayCueNotify_BurstLatent()
{
	PostProcess = CreateDefaultSubobject<UAgPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->OnFinished.BindUObject(this, &ThisClass::GameplayCueFinishedCallback);
}

bool AAgGameplayCueNotify_BurstLatent::Recycle()
{
	PostProcess->Reset();
	return Super::Recycle();
}

void AAgGameplayCueNotify_BurstLatent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PostProcess->Reset();
	Super::EndPlay(EndPlayReason);
}

bool AAgGameplayCueNotify_BurstLatent::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	PostProcess->Reset();
	SetActorHiddenInGame(false);
	if (!PostProcess->HasMaterial())
	{
		return Super::OnExecute_Implementation(Target, Parameters);
	}

	// 부모는 게임 시간으로 종료하므로, 같은 연출 순서를 실행하고 포스트프로세스의 실제 시간 완료로 회수합니다.
	GetWorld()->GetTimerManager().ClearTimer(FinishTimerHandle);
	FGameplayCueNotify_SpawnContext SpawnContext(GetWorld(), Target, Parameters);
	SpawnContext.SetDefaultSpawnCondition(&DefaultSpawnCondition);
	SpawnContext.SetDefaultPlacementInfo(&DefaultPlacementInfo);
	if (!DefaultSpawnCondition.ShouldSpawn(SpawnContext))
	{
		GameplayCueFinishedCallback();
		return false;
	}
	const float Duration = GetWorld()->GetSubsystem<UAgTimeSubsystem>()->GetSlowMotionDuration();
	if (Duration <= 0.f)
	{
		UE_LOG(LogAg, Error, TEXT("%s: screen effect needs a running slow motion for its duration"), *GetName());
		GameplayCueFinishedCallback();
		return false;
	}
	if (!PostProcess->Start(Target, Duration))
	{
		GameplayCueFinishedCallback();
		return false;
	}
	BurstEffects.ExecuteEffects(SpawnContext, BurstSpawnResults);
	OnBurst(Target, Parameters, BurstSpawnResults);
	return false;
}

#if WITH_EDITOR
EDataValidationResult AAgGameplayCueNotify_BurstLatent::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!PostProcess->ValidateSettings(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
