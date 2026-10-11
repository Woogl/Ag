// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotifyState_AttackWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"

float UAgAnimNotifyState_AttackWindow::GetTravelBeforeFirstWindow(const UAnimMontage* Montage)
{
	float FirstWindowStart = Montage->GetPlayLength();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass))
		{
			FirstWindowStart = FMath::Min(FirstWindowStart, Notify.GetTriggerTime());
		}
	}
	const FTransform RootMotion = Montage->ExtractRootMotionFromTrackRange(0.f, FirstWindowStart, FAnimExtractContext());
	return RootMotion.GetTranslation().Size2D();
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
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, AgGameplayTags::Event_AttackWindowEnded, Payload);
	}
}

FString UAgAnimNotifyState_AttackWindow::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Attack Window %d"), HitIndex + 1);
}
