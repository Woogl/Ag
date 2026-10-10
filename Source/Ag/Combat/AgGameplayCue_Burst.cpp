// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_Burst.h"

#include "NiagaraFunctionLibrary.h"

bool UAgGameplayCue_Burst::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (MyTarget && Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, Effect, Parameters.Location, FRotator::ZeroRotator, Scale);
	}
	return true;
}
