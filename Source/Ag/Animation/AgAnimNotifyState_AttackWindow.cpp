// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotifyState_AttackWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"

float UAgAnimNotifyState_AttackWindow::GetTravelToNextWindow(const UAnimInstance& AnimInstance)
{
	// 판정 구간이 있는 몽타주가 공격 몽타주입니다. 덧입히는 움찔 몽타주에는 판정 구간이 없습니다.
	for (const FAnimMontageInstance* Instance : AnimInstance.MontageInstances)
	{
		if (!Instance || !Instance->IsActive())
		{
			continue;
		}
		const float Position = Instance->GetPosition();
		float NextWindowStart = -1.f;
		for (const FAnimNotifyEvent& Notify : Instance->Montage->Notifies)
		{
			const float WindowStart = Notify.GetTriggerTime();
			if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass) && WindowStart >= Position && (NextWindowStart < 0.f || WindowStart < NextWindowStart))
			{
				NextWindowStart = WindowStart;
			}
		}
		if (NextWindowStart >= 0.f)
		{
			return Instance->Montage->ExtractRootMotionFromTrackRange(Position, NextWindowStart, FAnimExtractContext()).GetTranslation().Size2D();
		}
	}
	return 0.f;
}

void UAgAnimNotifyState_AttackWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AAgCharacterBase* Character = Cast<AAgCharacterBase>(MeshComp->GetOwner()))
	{
		Character->BeginAttackWindow(HitIndex);
	}
}

void UAgAnimNotifyState_AttackWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AAgCharacterBase* Character = Cast<AAgCharacterBase>(MeshComp->GetOwner()))
	{
		Character->EndAttackWindow(HitIndex);

		FGameplayEventData Payload;
		Payload.EventTag = AgGameplayTags::Event_AttackWindowEnded;
		Payload.EventMagnitude = HitIndex;
		Payload.OptionalObject = Animation;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, AgGameplayTags::Event_AttackWindowEnded, Payload);
	}
}

FString UAgAnimNotifyState_AttackWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Attack Window %d"), HitIndex + 1);
}
