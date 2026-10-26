// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "AgPostProcessComponent.generated.h"

class APawn;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;

UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent))
class UAgPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgPostProcessComponent();
	bool Start(AActor* Target, float Duration = 0.f);
	/** 페이드 아웃을 시작했거나 이미 페이드 중이면 true를 돌려주며, 그때는 페이드가 끝난 뒤 OnFinished가 옵니다. */
	bool Stop();
	void Reset();
	bool IsPlaying() const;
	bool IsFadingOut() const;
	bool HasMaterial() const;
	bool ValidateSettings(FString& Error) const;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FSimpleDelegate OnFinished;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Screen Effect")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditDefaultsOnly, Category = "Screen Effect", meta = (ClampMin = "0"))
	float BlendIn = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Screen Effect", meta = (ClampMin = "0"))
	float BlendOut = 0.f;

private:
	float GetWeight(double Now) const;
	void UpdateMaterial(double Now);
	void Finish();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> Volume;

	TWeakObjectPtr<APawn> TargetPawn;
	double StartTime = 0.;
	double StopTime = 0.;
	float PlaybackDuration = 0.f;
	float PlaybackBlendIn = 0.f;
	float PlaybackBlendOut = 0.f;
	float StopWeight = 0.f;
	bool bFadingOut = false;
};
