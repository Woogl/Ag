// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Looping.h"
#include "AgGameplayCueNotify_Looping.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UAgPostProcessComponent;

UCLASS(Abstract)
class AAgGameplayCueNotify_Looping : public AGameplayCueNotify_Looping
{
	GENERATED_BODY()

public:
	AAgGameplayCueNotify_Looping();

protected:
	virtual bool Recycle() override;
	virtual bool GameplayCuePendingRemove() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Screen Effect")
	TObjectPtr<UAgPostProcessComponent> PostProcess;

	/** 엔진은 캐릭터의 몸 메시에 붙이므로 무기에 붙일 때 켭니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	bool bAttachToWeapon = false;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	TMap<FName, FLinearColor> NiagaraColorParameters;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	TMap<FName, float> NiagaraFloatParameters;

	/**
	 * 비우지 않으면 효과를 붙이는 소켓(DefaultPlacementInfo의 SocketName)과 이 소켓의 가운데에 놓고 X축을 이 소켓 쪽으로
	 * 돌립니다. 두 소켓 사이 거리는 NiagaraSocketDistanceParameter에 넣습니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	FName PlacementEndSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	FName NiagaraSocketDistanceParameter;

	/** 나이아가라는 월드 시간으로 진행하므로, 대상의 시간 배율(히트스톱, 시간 정지 보정)을 따를 때 켭니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	bool bUseTargetTimeDilation = false;

	UPROPERTY(VisibleAnywhere, Category = "GCN Recurring Mesh (On Execute)")
	TObjectPtr<UStaticMeshComponent> RecurringMesh;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Recurring Mesh (On Execute)")
	FFloatInterval RecurringMeshDistance = FFloatInterval(0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "GCN Recurring Mesh (On Execute)")
	FFloatInterval RecurringMeshHeight = FFloatInterval(0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "GCN Recurring Mesh (On Execute)")
	FFloatInterval RecurringMeshYaw = FFloatInterval(0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "GCN Recurring Mesh (On Execute)")
	FFloatInterval RecurringMeshRoll = FFloatInterval(0.f, 0.f);

	/** 엔진은 꺼질 때 효과를 큐 위치 한 곳에만 재생하므로, 놓은 메시 자리마다 재생할 때 켭니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Recurring Mesh (On Execute)")
	bool bRemovalEffectsAtRecurringMeshes = false;

private:
	FGameplayCueParameters ResolveParameters(AActor* Target, const FGameplayCueParameters& Parameters) const;
	void ApplyNiagaraParameters(const FGameplayCueNotify_SpawnResult& SpawnResult) const;
	void PlaceBetweenSockets(UNiagaraComponent& Niagara) const;
	void ApplyTargetTimeDilation(const FGameplayCueNotify_SpawnResult& SpawnResult) const;
	void ReleaseCueTarget();
	void PlaceRecurringMesh(AActor* Target);
	void PlayRemovalEffectsAtRecurringMeshes(AActor* Target, const FGameplayCueParameters& Parameters);
	void ClearRecurringMeshes();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PlacedMeshes;

	TWeakObjectPtr<AActor> CueTarget;
};
