// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgCombatLibrary.generated.h"

class AAgCharacterBase;
enum class EAgCameraShake : uint8;

/** 여러 곳이 쓰는 전투 함수. GameplayEffect는 각 GE 클래스(UAgEffect_*)가 적용합니다. */
UCLASS()
class UAgCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Attacks only hit the other side (player <-> boss). */
	static bool AreHostile(const AAgCharacterBase* A, const AAgCharacterBase* B);

	/** 공격 방향: horizontal unit vector from the source's capsule center to the target's. */
	static FVector GetAttackDirection(const AActor* Source, const AActor* Target);

	/** Actor를 Target 쪽(캡슐 중심 사이의 수평 방향)으로 돌립니다. 둘이 겹쳐 방향이 없으면 그대로 둡니다. */
	static void TurnToward(AActor* Actor, const AActor* Target);

	/** Horizontal distance between two actors' capsule centers (대상과의 간격, 거리 구간). */
	static float GetHorizontalDistance(const AActor* A, const AActor* B);

	/** Plays a camera shake level through its GameplayCue (카메라 '카메라 셰이크'). Context is any character. */
	static void PlayCameraShake(AAgCharacterBase* Context, EAgCameraShake Shake);
};
