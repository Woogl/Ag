// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Face.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgFaceParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Face::Face(&Context.Runner, To);
}

UAgAbilityTask_Face* UAgAbilityTask_Face::Face(UGameplayAbility* Owner, EAgFaceTo InTo)
{
	UAgAbilityTask_Face* Task = NewAbilityTask<UAgAbilityTask_Face>(Owner);
	Task->To = InTo;
	return Task;
}

void UAgAbilityTask_Face::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	FVector Direction = FVector::ZeroVector;
	if (Character && Runner && To == EAgFaceTo::Target)
	{
		if (const AActor* Target = Runner->GetActionTarget())
		{
			Direction = (Target->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		}
	}
	else if (Character)
	{
		Direction = Character->GetMoveInputDirection().GetSafeNormal2D();
	}
	if (!Direction.IsNearlyZero())
	{
		Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	EndTask();
}
