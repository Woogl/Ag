// Copyright Woogle. All Rights Reserved.
#include "Animation/AgMontageTaskValidation.h"
#if WITH_EDITOR
#include "Animation/AgAnimNotifyState_AbilityTask.h"
#include "Animation/AgAnimNotify_SpawnProjectile.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimMontage.h"
#include "AnimNotifyState_MotionWarping.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"

void AgMontageTaskValidation::ValidateMontage(const UAnimMontage& Montage, const FAgAbilityData& Ability, TArray<FString>& Errors)
{
	for (int32 Index = 0; Index < Montage.Notifies.Num(); ++Index)
	{
		const FAnimNotifyEvent& Event = Montage.Notifies[Index];
		const FString Path = FString::Printf(TEXT("Abilities.%s %s notify %d"), *Ability.Id.ToString(), *Montage.GetPathName(), Index);
		if (const UAgAnimNotifyState_AbilityTask* Window = Cast<UAgAnimNotifyState_AbilityTask>(Event.NotifyStateClass))
		{
			FString Error;
			if (!Ability.ValidateTaskReference(Window->Task, Window->ParamsId, Error))
			{
				Errors.Add(Path + TEXT(": ") + Error);
			}
			if (Event.GetTime() < 0.f || Event.GetDuration() <= 0.f ||
			    Event.GetTime() + Event.GetDuration() > Montage.GetPlayLength() + KINDA_SMALL_NUMBER)
			{
				Errors.Add(Path + TEXT(": invalid interval"));
			}
			for (int32 OtherIndex = 0; OtherIndex < Index; ++OtherIndex)
			{
				const FAnimNotifyEvent& OtherEvent = Montage.Notifies[OtherIndex];
				const UAgAnimNotifyState_AbilityTask* Other = Cast<UAgAnimNotifyState_AbilityTask>(OtherEvent.NotifyStateClass);
				if (!Other || Event.GetTime() >= OtherEvent.GetTime() + OtherEvent.GetDuration() ||
				    OtherEvent.GetTime() >= Event.GetTime() + Event.GetDuration())
				{
					continue;
				}
				bool bConflict = Window->Task == TEXT("Dash") && Other->Task == TEXT("Dash");
				if (Window->Task == TEXT("SetCollisionResponseToChannel") && Other->Task == Window->Task)
				{
					const FAgCollisionResponseParams* A = Ability.Tasks.SetCollisionResponseToChannel.Find(Window->ParamsId);
					const FAgCollisionResponseParams* B = Ability.Tasks.SetCollisionResponseToChannel.Find(Other->ParamsId);
					bConflict |= A && B && A->Channel == B->Channel;
				}
				if (Window->Task == TEXT("TimeStop") && Other->Task == TEXT("TimeStop"))
				{
					bConflict = true;
				}
				if (bConflict)
				{
					Errors.Add(Path + FString::Printf(TEXT(": competing control with notify %d"), OtherIndex));
				}
			}
		}
		if (const UAnimNotifyState_MotionWarping* Warp = Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass))
		{
			const UAgRootMotionModifier_MotionWarping* Modifier = Cast<UAgRootMotionModifier_MotionWarping>(Warp->RootMotionModifier);
			if (!Modifier || Modifier->ParamsId.IsNone() || !Ability.Tasks.MotionWarping.Contains(Modifier->ParamsId))
			{
				Errors.Add(Path + TEXT(": Tasks.MotionWarping ParamsId missing or wrong modifier"));
			}
		}
		if (const UAgAnimNotify_SpawnProjectile* Shot = Cast<UAgAnimNotify_SpawnProjectile>(Event.Notify))
		{
			if (!Ability.Tasks.SpawnProjectile.Contains(Shot->ParamsId))
			{
				Errors.Add(Path + TEXT(": Tasks.SpawnProjectile.") + Shot->ParamsId.ToString() + TEXT(" missing"));
			}
		}
	}
}

void AgMontageTaskValidation::ValidateCharacter(const UAgCharacterData& Data, const AAgCharacterBase* Avatar, TArray<FString>& Errors)
{
	if (!Data.HasValidJsonData())
	{
		Errors.Add(Data.JsonFileName + TEXT(": no valid data"));
		return;
	}
	AgAbilityData::ValidateReferences(Data, Errors);
	if (Avatar && !Cast<UAgAbilitySystemComponent>(Avatar->GetAbilitySystemComponent()))
	{
		Errors.Add(Avatar->GetPathName() + TEXT(": requires AgAbilitySystemComponent"));
	}
	for (const FAgAbilityData& Ability : Data.Abilities)
	{
		TArray<FName> Ids = AgAbilityData::MontageIds(Data, Ability);
		for (FName Id : Ids)
		{
			const UAnimMontage* Montage = Data.Montages.FindRef(Id);
			if (!Montage)
			{
				continue;
			}
			ValidateMontage(*Montage, Ability, Errors);
			for (const FAnimNotifyEvent& Event : Montage->Notifies)
			{
				if (const UAgAnimNotify_SpawnProjectile* Shot = Cast<UAgAnimNotify_SpawnProjectile>(Event.Notify);
				    Shot && Shot->Origin == EAgProjectileOrigin::MeshSocket)
				{
					if (Shot->SocketName.IsNone() || (Avatar && !Avatar->GetMesh()->DoesSocketExist(Shot->SocketName)))
					{
						Errors.Add(Data.JsonFileName + TEXT(" Abilities.") + Ability.Id.ToString() + TEXT(".Tasks.SpawnProjectile.") +
						           Shot->ParamsId.ToString() + TEXT(": missing mesh socket"));
					}
				}
			}
		}
	}
}

bool AgMontageTaskValidation::CanValidate(const UObject& Asset)
{
	if (Asset.IsA<UAnimMontage>() || Asset.IsA<UAgCharacterData>())
	{
		return true;
	}
	const UBlueprint* BP = Cast<UBlueprint>(&Asset);
	return BP && BP->GeneratedClass && BP->GeneratedClass->IsChildOf(AAgCharacterBase::StaticClass());
}

void AgMontageTaskValidation::ValidateAsset(const UObject& Asset, TArray<FString>& Errors)
{
	if (const UAgCharacterData* Data = Cast<UAgCharacterData>(&Asset))
	{
		ValidateCharacter(*Data, nullptr, Errors);
	}
	else if (const UBlueprint* BP = Cast<UBlueprint>(&Asset))
	{
		const AAgCharacterBase* Avatar = BP->GeneratedClass ? Cast<AAgCharacterBase>(BP->GeneratedClass->GetDefaultObject()) : nullptr;
		if (Avatar && Avatar->GetCharacterData())
		{
			ValidateCharacter(*Avatar->GetCharacterData(), Avatar, Errors);
		}
	}
	else if (const UAnimMontage* Montage = Cast<UAnimMontage>(&Asset))
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(UAgCharacterData::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Entry : Assets)
		{
			const UAgCharacterData* ReferencingData = Cast<UAgCharacterData>(Entry.GetAsset());
			if (ReferencingData)
			{
				for (const TPair<FName, TObjectPtr<UAnimMontage>>& Pair : ReferencingData->Montages)
				{
					if (Pair.Value == Montage)
					{
						ValidateCharacter(*ReferencingData, nullptr, Errors);
						break;
					}
				}
			}
		}
	}
}

#endif
