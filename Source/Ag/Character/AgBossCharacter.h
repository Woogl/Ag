// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

class UWidgetComponent;
struct FAgBossPattern;

/**
 * 보스 커스터(보스 사양). 늘 AI의 대상을 향해 돌고(보스 사양 '회전'), AI가 고른 패턴을 실행합니다.
 * 월드에 표시하는 HUD(전투 HUD '락온 마커', '처형 안내')를 들고 있으며, 위젯은 BP_AgBoss에서 정합니다.
 */
UCLASS(Abstract)
class AAgBossCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	/**
	 * 부여된 패턴 스펙(Handle)이 실행하는 패턴의 데이터(Boss.json). 패턴 스펙이 아니면 null입니다.
	 * 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다(구현 계획서 '다시 읽기').
	 */
	const FAgBossPattern* FindPattern(const FGameplayAbilitySpecHandle Handle) const;

	/** 페이즈. 처음에는 1이고, 페이즈 전환이 끝나면 2입니다. */
	int32 GetPhase() const { return Phase; }

	/** 페이즈 전환이 끝남: 이제부터 2페이즈 변화를 적용합니다. */
	void EnterPhase2();

	/** 그로기 시간을 세기 시작하거나 멈춥니다. 공중에서 그로기가 되면 착지할 때 시작합니다. */
	void SetGroggyTimerRunning(bool bRunning);

	/** 그로기 시간을 세기 시작한 뒤 흐른 게임 시간. 세지 않을 때는 음수입니다. */
	float GetGroggyElapsedTime() const;

	/** 보스 AI의 현재 대상(구현 계획서 'AI 대상'). AI가 없거나 대상이 아직 없으면 null입니다. */
	AAgCharacterBase* GetTarget() const;

	/** 공격 중 이동의 대상은 AI의 대상입니다. */
	virtual AAgCharacterBase* FindAttackTarget() const override { return GetTarget(); }

	/**
	 * 보스 사양 '회전': 회전하지 않는 동안 참입니다. 판정 구간과 그 앞의 회전 정지 구간, 회전하지 않는 상태(피격 반응, 그로기,
	 * 처형 피격, 회전 고정, 비전투)가 해당합니다.
	 */
	virtual bool IsFacingFixed() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GrantAbilities() override;

	/** 보스는 패턴 밖(이동, 패턴 후딜레이)에서도 늘 AI의 대상을 향해 돕니다. */
	virtual AActor* GetFacingTarget() const override { return GetTarget(); }

private:

	/** 락온 마커와 처형 안내를 캐릭터 데이터에 정한 본에 붙입니다. */
	void AttachWorldWidgets();

	/** 락온 마커와 처형 안내를 보이거나 숨깁니다(전투 HUD '상황별 표시 규칙'). */
	void UpdateWorldWidgets();

	/** 락온 마커. 플레이어가 이 보스에 락온한 동안 락온 지점에 표시합니다. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	/** 처형 안내. 플레이어가 이 보스를 처형할 수 있는 동안 머리 위에 표시합니다. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	int32 Phase = 1;

	/** 그로기 시간을 세기 시작한 월드 시간. 세지 않을 때는 음수입니다. */
	double GroggyStartTime = -1.0;
};
