// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_DamageNumber.h"
#include "GameFramework/Pawn.h"
#include "UI/AgDamageFloater.h"
#include "UI/AgDamageFloaterWidget.h"

bool UAgGameplayCue_DamageNumber::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	// 이 기기에서 플레이어가 조종하는 캐릭터가 준 대미지만 띄웁니다.
	const APawn* Instigator = Cast<APawn>(Parameters.GetInstigator());
	if (Target && Instigator && Instigator->IsPlayerControlled() && Instigator->IsLocallyControlled())
	{
		AAgDamageFloater::Spawn(Target->GetWorld(), FloaterWidget, Parameters.Location, FMath::RoundToInt(Parameters.RawMagnitude));
	}
	return true;
}
