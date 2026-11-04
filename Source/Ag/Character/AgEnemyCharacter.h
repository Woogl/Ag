// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/AgPatrolRoute.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgEnemyCharacter.generated.h"

class AAgPlayerCharacter;
class APlayerController;
class FTokenizedMessage;
class UAnimMontage;
class UAgEnemySightPreviewComponent;
class UAgNameplateComponent;
class UAgPatrolPreviewComponent;
class UStateTree;
class UWidgetComponent;
struct FAbilityEndedData;

UCLASS(Abstract)
class AAgEnemyCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgEnemyCharacter();

	virtual bool CanLosePP() const override;

	/** 들어선 페이즈로의 전환을 아직 시작하지 않았습니다. 시작한 전환이 취소되어도 같은 페이즈로는 다시 전환하지 않습니다. */
	bool IsPhaseTransitionPending() const;

	/** 페이즈 전환 어빌리티를 발동하고, 시작했으면 들어선 페이즈로의 전환을 시작한 것으로 기록합니다. */
	bool StartPhaseTransition();

	bool IsPhaseTransitionActive() const;

	bool CanActivatePattern(FName Id) const;

	/** 대상과의 거리와 현재 페이즈에서 지금 발동할 수 있는 패턴을 가중치와 함께 모읍니다. 반응형 패턴은 반응 확률이 가중치입니다. */
	void GatherPatterns(float TargetDistance, bool bResponse, TArray<TPair<FName, float>>& OutCandidates) const;

	/** 현재 페이즈에서 쓰고 쿨다운이 끝난 패턴(반응형 패턴 제외)의 발동 가능 거리를 모읍니다. 거리와 대상 찾기는 보지 않습니다. */
	void GatherReadyPatternRanges(TArray<FFloatInterval>& OutRanges) const;

	virtual bool CanReturnToNonCombat() const
	{
		return true;
	}

	virtual bool HasNameplate() const
	{
		return true;
	}

	AAgPatrolRoute* GetPatrolRoute() const
	{
		return PatrolRoute;
	}

	const FTransform& GetHomeTransform() const
	{
		return HomeTransform;
	}

	virtual void PreInitializeComponents() override;

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	virtual FVector GetTargetLocation(AActor* RequestedBy = nullptr) const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void CheckForErrors() override;
#endif

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void HandleDeathEvent(const FGameplayEventData* Payload) override;

private:
	/** 현재 HP로 들어선 마지막 페이즈입니다. 페이즈 전환이 끝나야 현재 페이즈가 됩니다. */
	int32 GetReachedPhase() const;

	void DieWithDeathMotion(UAnimMontage& DeathMontage, float StartPosition);
	void HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	/** 처형 시작으로 그로기가 끝나면 PP 회복은 처형 피격 종료까지 미룹니다. */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleGroggyEvent(const FGameplayEventData* Payload);

	void HandleAbilityEnded(const FAbilityEndedData& Data);

	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

	void UpdateWorldWidgets();

	/** 네임플레이트 기준점이 화면 안에 있고, 카메라에서 몸 중심까지 벽·지형에 가리지 않았습니다. */
	bool IsNameplateInView(const APlayerController& PlayerController) const;

#if WITH_EDITOR
	/** 정찰 경로를 쓸 수 없는 MOV와, 처음 자리에서 가장 가까운 정찰 지점까지 걸어서 갈 수 없는 배치를 모읍니다. */
	void CollectPatrolProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;
#endif

	UPROPERTY(EditInstanceOnly, Category = "AI")
	TObjectPtr<AAgPatrolRoute> PatrolRoute;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UAgNameplateComponent> Nameplate;

	FTransform HomeTransform;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UAgEnemySightPreviewComponent> SightPreview;

	UPROPERTY()
	TObjectPtr<UAgPatrolPreviewComponent> PatrolPreview;
#endif

	int32 Phase = 1;
	int32 StartedTransitionPhase = 1;

	int32 DeathMotionInstanceId = INDEX_NONE;

	FDelegateHandle GroggyEventHandle;
};
