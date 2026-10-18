// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

class AAgPlayerCharacter;
class UWidgetComponent;

/**
 * 보스 커스터(보스 사양). 행동하지 않을 때 AI의 대상을 향해 돌고(보스 사양 '회전'), AI가 고른 패턴을 실행합니다. 그로기가 끝나면
 * PP를 채웁니다. 월드에 표시하는 HUD(전투 HUD '락온 마커', '처형 안내')를 들고 있으며, 위젯은 BP_AgBoss에서 정합니다.
 */
UCLASS(Abstract)
class AAgBossCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	/** 페이즈. 처음에는 1이고, 페이즈 전환이 끝나면 2입니다. */
	int32 GetPhase() const
	{
		return Phase;
	}

	/** 페이즈 전환이 끝남: 이제부터 2페이즈 변화를 적용합니다. */
	void EnterPhase2();

	/** 보스 AI의 현재 대상(구현 계획서 'AI 대상'). AI가 없거나 대상이 아직 없으면 null입니다. */
	AAgCharacterBase* GetTarget() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 보스는 행동하지 않는 동안(이동, 패턴 후딜레이) AI의 대상을 향해 돕니다. */
	virtual AActor* GetFacingTarget() const override
	{
		return GetTarget();
	}

private:
	/**
	 * 그로기가 끝나면(State.Groggy가 사라졌고 살아 있으면) PP를 MaxPP로 채우고, 다음 그로기와 섞이지 않게 남은 그로기 시간
	 * 효과를 지웁니다. 처형 피격이 State.Groggy를 이어 받으므로 처형 피격이 끝날 때 한 번 채웁니다.
	 */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	/** 처형 안내: 플레이어 처형 행동의 Targeting을 읽기 전용으로 평가해 이 보스가 그 대상입니다. */
	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

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
};
