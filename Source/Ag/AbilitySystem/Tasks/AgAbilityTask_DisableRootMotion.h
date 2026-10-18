// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_DisableRootMotion.generated.h"

struct FAnimMontageInstance;

/**
 * 몽타주 루트 모션 끄기(DisableRootMotion Task의 Params). 구간에만 두며 값이 없습니다. 루트 모션 소스(Dash, Launch)로 움직이는
 * 동안과 모션의 루트 이동을 쓰지 않는 부분에 둡니다.
 */
USTRUCT()
struct FAgDisableRootMotionParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 도는 동안 그 재생(몽타주 인스턴스)의 루트 모션을 끕니다. 엔진 AnimNotifyState_DisableRootMotion과 같은 인스턴스 카운터를 쓰므로,
 * 캐릭터 무브먼트는 그동안 몽타주 루트 모션 대신 루트 모션 소스를 씁니다. 같은 재생에서 구간이 끝나면 되돌리고, 행동이 재생을
 * 내려놓으면(다시 재생, 끊김, 행동 종료) 블렌드 아웃이 끝날 때까지 꺼 둡니다. 재생 중간에 시작하는 구간은 그 시각을 지난 프레임의
 * 판정(몽타주 진행 뒤)에서 켜지므로, 그 프레임에서 시각 뒤의 몽타주 루트 모션은 남습니다.
 */
UCLASS()
class UAgAbilityTask_DisableRootMotion : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_DisableRootMotion* DisableRootMotion(UGameplayAbility* Owner, int32 MontageInstanceId);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	FAnimMontageInstance* FindInstance() const;

	int32 MontageInstanceId = INDEX_NONE;
	bool bDisabled = false;
};
