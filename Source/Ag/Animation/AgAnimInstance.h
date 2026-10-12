// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AgAnimInstance.generated.h"

class AAgCharacterBase;
class UAnimSequenceBase;
class UBlendSpace;

/**
 * ABP_Ag의 부모로, 플레이어와 보스가 같이 씁니다. 애님 그래프가 읽는 값은 모두 여기서 계산하고, 그래프는 블렌드만
 * 합니다(이동, 가드 이동, 공중, 몽타주 슬롯, 덧입힘 슬롯).
 */
UCLASS()
class UAgAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 캐릭터 데이터에서 가져온 값 */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** 캐릭터 데이터에서 가져온 값. 가드하지 않는 캐릭터는 비어 있습니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** 캐릭터 데이터에서 가져온 값 */
	UPROPERTY(BlueprintReadOnly, Category = "Assets")
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
