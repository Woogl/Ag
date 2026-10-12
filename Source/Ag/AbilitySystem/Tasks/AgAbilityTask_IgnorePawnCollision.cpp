// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_IgnorePawnCollision.h"

#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"

UAgAbilityTask_IgnorePawnCollision* UAgAbilityTask_IgnorePawnCollision::IgnorePawnCollision(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAgAbilityTask_IgnorePawnCollision>(OwningAbility);
}

void UAgAbilityTask_IgnorePawnCollision::Activate()
{
	Super::Activate();

	AAgCharacterBase* Avatar = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Avatar)
	{
		EndTask();
		return;
	}
	Character = Avatar;
	UCapsuleComponent* Capsule = Avatar->GetCapsuleComponent();
	SavedResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void UAgAbilityTask_IgnorePawnCollision::OnDestroy(bool bInOwnerFinished)
{
	// 사망 처리도 캐릭터끼리 막지 않게 바꾸므로, 죽었으면 되돌리지 않습니다.
	if (AAgCharacterBase* Avatar = Character.Get(); Avatar && !Avatar->IsDead())
	{
		Avatar->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, SavedResponse);
	}

	Super::OnDestroy(bInOwnerFinished);
}
