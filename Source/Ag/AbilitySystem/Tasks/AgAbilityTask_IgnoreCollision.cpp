// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_IgnoreCollision.h"

#include "Character/AgCharacterBase.h"
#include "Components/PrimitiveComponent.h"

UAgAbilityTask_IgnoreCollision* UAgAbilityTask_IgnoreCollision::IgnoreCollision(UGameplayAbility* OwningAbility, ECollisionChannel InChannel)
{
	UAgAbilityTask_IgnoreCollision* Task = NewAbilityTask<UAgAbilityTask_IgnoreCollision>(OwningAbility);
	Task->Channel = InChannel;
	return Task;
}

void UAgAbilityTask_IgnoreCollision::Activate()
{
	Super::Activate();

	const AActor* Avatar = GetAvatarActor();
	UPrimitiveComponent* Root = Avatar ? Cast<UPrimitiveComponent>(Avatar->GetRootComponent()) : nullptr;
	if (!Root)
	{
		EndTask();
		return;
	}
	Component = Root;
	SavedResponse = Root->GetCollisionResponseToChannel(Channel);
	Root->SetCollisionResponseToChannel(Channel, ECR_Ignore);
}

void UAgAbilityTask_IgnoreCollision::OnDestroy(bool bInOwnerFinished)
{
	// 사망 처리도 충돌 응답을 바꾸므로, 죽었으면 되돌리지 않습니다.
	if (UPrimitiveComponent* Root = Component.Get())
	{
		const AAgCharacterBase* Character = Cast<AAgCharacterBase>(Root->GetOwner());
		if (!Character || !Character->IsDead())
		{
			Root->SetCollisionResponseToChannel(Channel, SavedResponse);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}
