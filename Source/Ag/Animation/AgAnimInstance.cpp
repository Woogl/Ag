// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** 캐릭터 데이터의 Animations에서 찾는 ID: 이동, 가드 중 이동(블렌드 스페이스), 공중에서 반복하는 모션 */
	const FName MoveAnimationId(TEXT("Move"));
	const FName GuardMoveAnimationId(TEXT("GuardMove"));
	const FName AirborneAnimationId(TEXT("Airborne"));
}

void UAgAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 블렌드 스페이스와 공중 모션은 캐릭터 데이터에서 가져옵니다. 애니메이션 에디터의 미리보기에는 캐릭터가 없습니다.
	Character = Cast<AAgCharacterBase>(TryGetPawnOwner());
	if (const UAgCharacterData* Data = Character ? Character->GetCharacterData() : nullptr)
	{
		MoveBlendSpace = Data->FindAnimation<UBlendSpace>(MoveAnimationId);
		GuardMoveBlendSpace = Data->FindAnimation<UBlendSpace>(GuardMoveAnimationId);
		AirborneAnimation = Data->FindAnimation<UAnimSequenceBase>(AirborneAnimationId);
	}
}

void UAgAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	if (GroundSpeed > UE_KINDA_SMALL_NUMBER)
	{
		// 서 있을 때는 마지막 방향을 유지해 블렌드가 튀지 않게 합니다.
		const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Velocity);
		MoveDirection = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}

	bIsInAir = Character->GetCharacterMovement()->IsFalling();

	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	bIsGuarding = GuardMoveBlendSpace && AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AgGameplayTags::State_Guarding);
}
