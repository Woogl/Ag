// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Looping.h"
#include "AgGameplayCueNotify_Looping.generated.h"

class UStaticMeshComponent;

UCLASS(Abstract)
class AAgGameplayCueNotify_Looping : public AGameplayCueNotify_Looping
{
	GENERATED_BODY()

public:
	AAgGameplayCueNotify_Looping();

protected:
	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

	/** 엔진은 캐릭터의 몸 메시에 붙이므로 무기에 붙일 때 켭니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	bool bAttachToWeapon = false;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	TMap<FName, FLinearColor> NiagaraColorParameters;

	UPROPERTY(EditDefaultsOnly, Category = "GCN Defaults")
	TMap<FName, float> NiagaraFloatParameters;

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
	void PlaceRecurringMesh(AActor* Target);
	void PlayRemovalEffectsAtRecurringMeshes(AActor* Target, const FGameplayCueParameters& Parameters);
	void ClearRecurringMeshes();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PlacedMeshes;
};
