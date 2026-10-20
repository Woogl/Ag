// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_DamageFloater.h"

#include "GameFramework/Pawn.h"
#include "UI/AgDamageFloater.h"
#include "UI/AgDamageFloaterWidget.h"

bool UAgGameplayCueNotify_DamageFloater::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	const APawn* Instigator = Cast<APawn>(Parameters.GetInstigator());
	if (Target && Instigator && Instigator->IsPlayerControlled() && Instigator->IsLocallyControlled())
	{
		AAgDamageFloater::Spawn(Target->GetWorld(), FloaterWidget, Parameters.Location, FMath::RoundToInt(Parameters.RawMagnitude));
	}
	return true;
}
