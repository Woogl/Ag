// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgCombatLibrary.generated.h"

class AAgCharacterBase;
enum class EAgCameraShake : uint8;

/** 여러 곳이 쓰는 전투 함수. GE는 각 GE 클래스(UAgEffect_*)가 적용합니다. */
UCLASS()
class UAgCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 공격은 반대편(플레이어 ↔ 적)에만 맞습니다. */
	static bool AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B);

	/** Source에서 Target으로 향하는 수평 단위 벡터. 캐릭터는 캡슐 중심 기준이며, 전투 시스템의 피격 방향도 이것으로 구합니다. */
	static FVector GetHorizontalDirection(const AActor* Source, const AActor* Target);

	/** 두 액터 사이의 수평 거리. 캐릭터는 캡슐 중심 기준입니다(대상과의 간격, 거리 구간). */
	static float GetHorizontalDistance(const AActor* A, const AActor* B);

	/** 셰이크 단계 하나를 그 단계의 게임플레이 큐로 로컬 플레이어의 카메라에 재생합니다(카메라 '카메라 셰이크'). */
	static void PlayCameraShake(const UObject* WorldContext, EAgCameraShake Shake);
};
