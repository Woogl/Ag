// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AgAnimInstance.generated.h"

class AAgCharacterBase;
class UAnimSequenceBase;
class UBlendSpace;
struct FAnimNotifyEventReference;

/**
 * 공통 그래프 ABP_Ag의 부모입니다. 자식 ABP_AgPlayer·ABP_AgBoss에서 모션 기본값을 정합니다. 애님 그래프가 읽는 값은 모두 여기서 계산하고,
 * 그래프는 블렌드만 합니다(이동, 가드 이동, 공중, 몽타주 슬롯, 덧입힘 슬롯).
 */
UCLASS()
class UAgAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** 단발 알림의 실제 재생 출처를 이 인스턴스의 엔진 몽타주 델리게이트로 전달합니다. */
	void BroadcastMontageNotify(UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference);

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 자식 ABP의 기본값에서 정하는 모션 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** 자식 ABP의 기본값에서 정하는 가드 이동 모션. 가드하지 않는 캐릭터는 비워 둡니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** 자식 ABP의 기본값에서 정하는 모션 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	/** 수평 속도 (cm/s) */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** 캐릭터 정면 기준 이동 방향 (도): 0 앞, 90 오른쪽, -90 왼쪽, ±180 뒤 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float MoveDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsGuarding = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<AAgCharacterBase> Character;
};
