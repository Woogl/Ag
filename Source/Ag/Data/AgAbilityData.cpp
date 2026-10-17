// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgJsonRead.h"
#include "Data/AgMontageTimeline.h"
#include "Core/AgAssetManager.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "LevelSequence.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace AgJsonRead;

	bool Tags(const FJsonObject& Object, const FString& Key, const FString& Path, FGameplayTagContainer& Out, TArray<FString>& Errors)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.TryGetArrayField(Key, Values))
		{
			Errors.Add(Path + TEXT(".") + Key + TEXT(": expected tag array"));
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const FGameplayTag Tag =
			    Value->Type == EJson::String ? FGameplayTag::RequestGameplayTag(FName(*Value->AsString()), false) : FGameplayTag();
			if (!Tag.IsValid() || Out.HasTagExact(Tag))
			{
				Errors.Add(Path + TEXT(".") + Key + TEXT(": unknown or duplicate native tag"));
			}
			else
			{
				Out.AddTag(Tag);
			}
		}
		return true;
	}

	template <typename T>
	void ReadMap(const FJsonObject& Tasks, const TCHAR* Name, TMap<FName, T>& Out, const FString& Path, TArray<FString>& Errors)
	{
		if (!Tasks.HasField(Name))
		{
			return;
		}
		const FString Field = Path + TEXT(".") + Name;
		const FObject Map = ObjectAt(Tasks, Name, Field, Errors);
		if (!Map)
		{
			return;
		}
		if (Map->Values.IsEmpty())
		{
			Errors.Add(Field + TEXT(": requires at least one ParamsId"));
		}
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Map->Values)
		{
			const FName Id(*Pair.Key);
			if (Id.IsNone() || Out.Contains(Id))
			{
				Errors.Add(Field + TEXT(".") + Pair.Key + TEXT(": empty or duplicate ParamsId"));
				continue;
			}
			const FObject Params = ObjectAt(*Map, FString(Pair.Key), Field + TEXT(".") + Pair.Key, Errors);
			if (Params)
			{
				T Value;
				if (Read(*Params, Value, Field + TEXT(".") + Pair.Key, Errors))
				{
					Out.Add(Id, Value);
				}
			}
		}
	}

	template <typename T>
	void ReadSingle(const FJsonObject& Tasks, const TCHAR* Name, T& Out, bool& bPresent, const FString& Path, TArray<FString>& Errors)
	{
		bPresent = Tasks.HasField(Name);
		if (!bPresent)
		{
			return;
		}
		if (const FObject Object = ObjectAt(Tasks, Name, Path + TEXT(".") + Name, Errors))
		{
			Read(*Object, Out, Path + TEXT(".") + Name, Errors);
		}
	}

	void ReadTasks(const FJsonObject& Object, FAgAbilityTasks& Out, const FString& Path, TArray<FString>& Errors)
	{
		Keys(Object, Path, {},
		    {TEXT("PlayMontageAndWait"), TEXT("DrainResource"), TEXT("TimedTag"), TEXT("GrantResources"), TEXT("SlowMotion"),
		        TEXT("LaunchVertical"), TEXT("LaunchToTarget"), TEXT("TargetQuery"), TEXT("Attack"), TEXT("HoldTag"),
		        TEXT("SetCollisionResponseToChannel"), TEXT("TimeStop"), TEXT("LockOn"), TEXT("PlayCutscene")},
		    Errors);
		ReadMap(Object, TEXT("TimedTag"), Out.TimedTag, Path, Errors);
		ReadMap(Object, TEXT("GrantResources"), Out.GrantResources, Path, Errors);
		ReadMap(Object, TEXT("SlowMotion"), Out.SlowMotion, Path, Errors);
		ReadMap(Object, TEXT("TargetQuery"), Out.TargetQuery, Path, Errors);
		ReadMap(Object, TEXT("Attack"), Out.Attack, Path, Errors);
		ReadMap(Object, TEXT("HoldTag"), Out.HoldTag, Path, Errors);
		ReadMap(Object, TEXT("SetCollisionResponseToChannel"), Out.SetCollisionResponseToChannel, Path, Errors);
		ReadMap(Object, TEXT("PlayCutscene"), Out.PlayCutscene, Path, Errors);
		ReadSingle(Object, TEXT("DrainResource"), Out.DrainResource, Out.bHasDrainResource, Path, Errors);
		ReadSingle(Object, TEXT("LaunchVertical"), Out.LaunchVertical, Out.bHasLaunchVertical, Path, Errors);
		ReadSingle(Object, TEXT("LaunchToTarget"), Out.LaunchToTarget, Out.bHasLaunchToTarget, Path, Errors);
		Out.bHasPlayMontageAndWait = Object.HasField(TEXT("PlayMontageAndWait"));
		if (Out.bHasPlayMontageAndWait)
		{
			const FString Field = Path + TEXT(".PlayMontageAndWait");
			if (FObject Params = ObjectAt(Object, TEXT("PlayMontageAndWait"), Field, Errors))
			{
				Keys(*Params, Field, {TEXT("MontageId")}, {TEXT("RootMotionDistance")}, Errors);
				FString Id;
				if (!Params->TryGetStringField(TEXT("MontageId"), Id) || FName(*Id).IsNone())
				{
					Errors.Add(Field + TEXT(".MontageId: missing name"));
				}
				Out.PlayMontageAndWait.MontageId = FName(*Id);
				Out.PlayMontageAndWait.bHasRootMotionDistance = Params->HasField(TEXT("RootMotionDistance"));
				if (Out.PlayMontageAndWait.bHasRootMotionDistance)
				{
					Number(*Params, TEXT("RootMotionDistance"), Field, Out.PlayMontageAndWait.RootMotionDistance, Errors);
					if (Out.PlayMontageAndWait.RootMotionDistance <= 0.f)
					{
						Errors.Add(Field + TEXT(".RootMotionDistance: must be positive"));
					}
				}
			}
		}
		Out.bHasTimeStop = Object.HasField(TEXT("TimeStop"));
		Out.bHasLockOn = Object.HasField(TEXT("LockOn"));
		for (const TCHAR* Name : {TEXT("TimeStop"), TEXT("LockOn")})
		{
			if (Object.HasField(Name))
			{
				if (FObject Empty = ObjectAt(Object, Name, Path + TEXT(".") + Name, Errors))
				{
					Keys(*Empty, Path + TEXT(".") + Name, {}, {}, Errors);
				}
			}
		}
		if (Out.bHasDrainResource && Out.DrainResource.AmountPerSecond <= 0.f)
		{
			Errors.Add(Path + TEXT(".DrainResource.AmountPerSecond: must be positive"));
		}
		if (Out.bHasLaunchToTarget && Out.LaunchToTarget.AirTime <= 0.f)
		{
			Errors.Add(Path + TEXT(".LaunchToTarget.AirTime: must be positive"));
		}
		for (const TPair<FName, FAgTimedTagParams>& Pair : Out.TimedTag)
		{
			if (Pair.Value.Duration <= 0.f)
			{
				Errors.Add(Path + TEXT(".TimedTag.") + Pair.Key.ToString() + TEXT(".Duration: must be positive"));
			}
		}
		for (const TPair<FName, FAgGrantResourcesParams>& Pair : Out.GrantResources)
		{
			if (Pair.Value.SP + Pair.Value.MP + Pair.Value.UP <= 0.f)
			{
				Errors.Add(Path + TEXT(".GrantResources.") + Pair.Key.ToString() + TEXT(": reward must be positive"));
			}
		}
		for (const TPair<FName, FAgSlowMotionParams>& Pair : Out.SlowMotion)
		{
			if (Pair.Value.Speed <= 0.f || Pair.Value.Speed > 1.f || Pair.Value.RealDuration <= 0.f)
			{
				Errors.Add(Path + TEXT(".SlowMotion.") + Pair.Key.ToString() + TEXT(": invalid speed or duration"));
			}
		}
	}

}

FAgPlayerAttackHit FAgAttackParams::MakeHit(bool bFinisher) const
{
	FAgPlayerAttackHit Hit;
	Hit.DamageMultiplier = DamageMultiplier;
	Hit.PoiseMultiplier = PoiseMultiplier;
	Hit.HitReaction = HitReaction;
	Hit.bGuardable = bGuardable;
	Hit.MPCharge = MPCharge;
	Hit.UPCharge = UPCharge;
	Hit.bFinisher = bFinisher;
	return Hit;
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	return Id == Other.Id && Tags == Other.Tags &&
	       FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}

bool AgAbilityData::Parse(const FJsonObject& Json, TArray<FAgAbilityData>& OutAbilities, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Json.TryGetArrayField(TEXT("Abilities"), Entries))
	{
		Errors.Add(TEXT("Abilities: expected array"));
		return false;
	}
	TArray<FAgAbilityData> Candidate;
	TSet<FName> Ids;
	for (int32 Index = 0; Index < Entries->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* Entry = nullptr;
		FString Path = FString::Printf(TEXT("Abilities[%d]"), Index);
		if (!(*Entries)[Index]->TryGetObject(Entry))
		{
			Errors.Add(Path + TEXT(": expected object"));
			continue;
		}
		FAgAbilityData Data;
		FString Id;
		if (!(*Entry)->TryGetStringField(TEXT("Id"), Id) || FName(*Id).IsNone() || Ids.Contains(FName(*Id)))
		{
			Errors.Add(Path + TEXT(".Id: missing or duplicate ID"));
		}
		Data.Id = FName(*Id);
		Ids.Add(Data.Id);
		Path += TEXT("(") + Id + TEXT(")");
		Keys(**Entry, Path, {TEXT("Id"), TEXT("Tags"), TEXT("Cooldown"), TEXT("Activation"), TEXT("Cost"), TEXT("Tasks")},
		    {TEXT("PatternSelection")}, Errors);
		Tags(**Entry, TEXT("Tags"), Path, Data.Tags, Errors);
		Number(**Entry, TEXT("Cooldown"), Path, Data.Cooldown, Errors);
		if (FObject Activation = ObjectAt(**Entry, TEXT("Activation"), Path + TEXT(".Activation"), Errors))
		{
			Keys(*Activation, Path + TEXT(".Activation"),
			    {TEXT("RequiredTags"), TEXT("BlockedTags"), TEXT("OwnedTags"), TEXT("BlockTags"), TEXT("CancelTags"), TEXT("TriggerEvents"),
			        TEXT("bAllowRetrigger")},
			    {}, Errors);
			Tags(*Activation, TEXT("RequiredTags"), Path + TEXT(".Activation"), Data.Activation.RequiredTags, Errors);
			Tags(*Activation, TEXT("BlockedTags"), Path + TEXT(".Activation"), Data.Activation.BlockedTags, Errors);
			Tags(*Activation, TEXT("OwnedTags"), Path + TEXT(".Activation"), Data.Activation.OwnedTags, Errors);
			Tags(*Activation, TEXT("BlockTags"), Path + TEXT(".Activation"), Data.Activation.BlockTags, Errors);
			Tags(*Activation, TEXT("CancelTags"), Path + TEXT(".Activation"), Data.Activation.CancelTags, Errors);
			Tags(*Activation, TEXT("TriggerEvents"), Path + TEXT(".Activation"), Data.Activation.TriggerEvents, Errors);
			const TSharedPtr<FJsonValue> Retrigger = Activation->TryGetField(TEXT("bAllowRetrigger"));
			if (!Retrigger || Retrigger->Type != EJson::Boolean)
			{
				Errors.Add(Path + TEXT(".Activation.bAllowRetrigger: expected boolean"));
			}
			else
			{
				Data.Activation.bAllowRetrigger = Retrigger->AsBool();
			}
		}
		const TSharedPtr<FJsonValue> Cost = (*Entry)->TryGetField(TEXT("Cost"));
		Data.bHasCost = Cost && Cost->Type != EJson::Null;
		if (Data.bHasCost)
		{
			if (const FObject Params = ObjectAt(**Entry, TEXT("Cost"), Path + TEXT(".Cost"), Errors))
			{
				Read(*Params, Data.Cost, Path + TEXT(".Cost"), Errors);
			}
		}
		if (const FObject Tasks = ObjectAt(**Entry, TEXT("Tasks"), Path + TEXT(".Tasks"), Errors))
		{
			ReadTasks(*Tasks, Data.Tasks, Path + TEXT(".Tasks"), Errors);
		}
		Data.bHasPatternSelection = (*Entry)->HasField(TEXT("PatternSelection"));
		if (Data.bHasPatternSelection)
		{
			const FString Field = Path + TEXT(".PatternSelection");
			if (const FObject Pattern = ObjectAt(**Entry, TEXT("PatternSelection"), Field, Errors))
			{
				Keys(*Pattern, Field,
				    {TEXT("Phases"), TEXT("WeightNear"), TEXT("WeightMid"), TEXT("WeightFar"), TEXT("ResponseRange"),
				        TEXT("ResponseChance")},
				    {}, Errors);
				Number(*Pattern, TEXT("WeightNear"), Field, Data.PatternSelection.WeightNear, Errors);
				Number(*Pattern, TEXT("WeightMid"), Field, Data.PatternSelection.WeightMid, Errors);
				Number(*Pattern, TEXT("WeightFar"), Field, Data.PatternSelection.WeightFar, Errors);
				Number(*Pattern, TEXT("ResponseRange"), Field, Data.PatternSelection.ResponseRange, Errors);
				Number(*Pattern, TEXT("ResponseChance"), Field, Data.PatternSelection.ResponseChance, Errors);
				if (Data.PatternSelection.ResponseChance > 1.f)
				{
					Errors.Add(Field + TEXT(".ResponseChance: must be <= 1"));
				}
				const TArray<TSharedPtr<FJsonValue>>* Phases = nullptr;
				if (!Pattern->TryGetArrayField(TEXT("Phases"), Phases))
				{
					Errors.Add(Field + TEXT(".Phases: expected array"));
				}
				else
				{
					for (const TSharedPtr<FJsonValue>& Phase : *Phases)
					{
						if (Phase->Type != EJson::Number || (Phase->AsNumber() != 1 && Phase->AsNumber() != 2))
						{
							Errors.Add(Field + TEXT(".Phases: expected phase 1 or 2"));
						}
						else
						{
							Data.PatternSelection.Phases.Add(static_cast<int32>(Phase->AsNumber()));
						}
					}
				}
			}
		}
		Candidate.Add(MoveTemp(Data));
	}
	if (Before != Errors.Num())
	{
		return false;
	}
	OutAbilities = MoveTemp(Candidate);
	return true;
}

void AgAbilityData::ValidateReferences(const UAgCharacterData& Data, TArray<FString>& Errors)
{
	if (const UAgPlayerData* Player = Cast<UAgPlayerData>(&Data);
	    Player && Data.FindAbility(TEXT("BasicAttack")) && Player->BasicAttackMontageIds.IsEmpty())
	{
		Errors.Add(TEXT("BasicAttackMontageIds: requires at least one montage"));
	}
	for (const FAgAbilityData& Ability : Data.Abilities)
	{
		const FString Path = TEXT("Abilities.") + Ability.Id.ToString() + TEXT(".Tasks.");
		const FAgAbilityTasks& Tasks = Ability.Tasks;
		for (FName MontageId : MontageIds(Data, Ability))
		{
			if (!Data.Montages.FindRef(MontageId))
			{
				Errors.Add(Path + TEXT("MontageId: missing montage ") + MontageId.ToString());
			}
		}
		TFunction<void(bool, const TCHAR*)> Require = [&](bool bPresent, const TCHAR* Field)
		{
			if (!bPresent)
			{
				Errors.Add(Path + Field + TEXT(": required by C++ event"));
			}
		};
		const FName Id = Ability.Id;
		if (Id == TEXT("Dodge"))
		{
			Require(Tasks.GrantResources.Contains(TEXT("PerfectDodge")), TEXT("GrantResources.PerfectDodge"));
			Require(Tasks.TimedTag.Contains(TEXT("CounterChance")), TEXT("TimedTag.CounterChance"));
			Require(Tasks.SlowMotion.Contains(TEXT("PerfectDodge")), TEXT("SlowMotion.PerfectDodge"));
		}
		if (Id == TEXT("Guard"))
		{
			Require(Tasks.HoldTag.Contains(TEXT("Guarding")), TEXT("HoldTag.Guarding"));
			Require(Tasks.TimedTag.Contains(TEXT("ParryWindow")), TEXT("TimedTag.ParryWindow"));
			Require(Tasks.GrantResources.Contains(TEXT("Parry")), TEXT("GrantResources.Parry"));
		}
		if (Id == TEXT("KnockBack") || Id == TEXT("KnockUp"))
		{
			Require(Tasks.HoldTag.Contains(TEXT("Downed")), TEXT("HoldTag.Downed"));
		}
		if (Id == TEXT("Groggy"))
		{
			Require(Tasks.HoldTag.Contains(TEXT("Executed")), TEXT("HoldTag.Executed"));
			Require(Tasks.SetCollisionResponseToChannel.Contains(TEXT("PassThroughPawn")),
			    TEXT("SetCollisionResponseToChannel.PassThroughPawn"));
		}
		if (Id == TEXT("Ultimate"))
		{
			Require(Tasks.bHasTimeStop, TEXT("TimeStop"));
			Require(Tasks.PlayCutscene.Contains(TEXT("UltimateCamera")), TEXT("PlayCutscene.UltimateCamera"));
			Require(Tasks.TargetQuery.Contains(TEXT("Finish")), TEXT("TargetQuery.Finish"));
			Require(Tasks.Attack.Contains(TEXT("Finish")), TEXT("Attack.Finish"));
		}
		if (Id == TEXT("Execution"))
		{
			Require(Tasks.TargetQuery.Contains(TEXT("Execution")), TEXT("TargetQuery.Execution"));
			Require(Tasks.Attack.Contains(TEXT("Stab")), TEXT("Attack.Stab"));
			Require(Tasks.Attack.Contains(TEXT("Finish")), TEXT("Attack.Finish"));
		}
		if (Id == TEXT("LeapSlam"))
		{
			Require(Tasks.bHasLaunchToTarget, TEXT("LaunchToTarget"));
			Require(Tasks.TargetQuery.Contains(TEXT("Landing")), TEXT("TargetQuery.Landing"));
			Require(Tasks.Attack.Contains(TEXT("LandingHit")), TEXT("Attack.LandingHit"));
		}
		if (Id == TEXT("JumpAttack"))
		{
			Require(Tasks.bHasLaunchVertical, TEXT("LaunchVertical"));
		}
		if (Id == TEXT("LockOn"))
		{
			Require(Tasks.bHasLockOn, TEXT("LockOn"));
		}
		if (Tasks.bHasPlayMontageAndWait)
		{
			const FAgPlayMontageAndWaitParams& Params = Tasks.PlayMontageAndWait;
			UAnimMontage* Montage = Data.Montages.FindRef(Params.MontageId);
			if (!Montage)
			{
				Errors.Add(Path + TEXT("PlayMontageAndWait.MontageId: missing montage ") + Params.MontageId.ToString());
			}
			else if (Params.bHasRootMotionDistance &&
			         Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext())
			                 .GetTranslation()
			                 .Size2D() <= UE_SMALL_NUMBER)
			{
				Errors.Add(Path + TEXT("PlayMontageAndWait.RootMotionDistance: montage has no horizontal root motion"));
			}
		}
	}
	// 이 캐릭터가 재생하는 몽타주의 시간표가 쏘는 투사체는 이 캐릭터의 Projectiles에 있어야 합니다.
	const UAgMontageTimelineData& Timelines = UAgAssetManager::Get().GetMontageTimelines();
	for (const TPair<FName, TObjectPtr<UAnimMontage>>& Pair : Data.Montages)
	{
		const FAgMontageTimeline* Timeline = Pair.Value ? Timelines.Find(*Pair.Value) : nullptr;
		if (!Timeline)
		{
			continue;
		}
		for (const FAgTimelineSpawnProjectile& Shot : Timeline->SpawnProjectile)
		{
			if (!Data.Projectiles.Contains(Shot.ProjectileId))
			{
				Errors.Add(TEXT("Montages.") + Pair.Key.ToString() + TEXT(": timeline projectile ") + Shot.ProjectileId.ToString() +
				           TEXT(" missing in Projectiles"));
			}
		}
	}
}

TArray<FName> AgAbilityData::MontageIds(const UAgCharacterData& Data, const FAgAbilityData& Ability)
{
	TArray<FName> Result;
	if (Ability.Tasks.bHasPlayMontageAndWait)
	{
		Result.AddUnique(Ability.Tasks.PlayMontageAndWait.MontageId);
	}
	const FName Id = Ability.Id;
	if (Id == TEXT("BasicAttack"))
	{
		if (const UAgPlayerData* Player = Cast<UAgPlayerData>(&Data))
		{
			Result.Append(Player->BasicAttackMontageIds);
		}
	}
	if (Id == TEXT("Dodge"))
	{
		Result.Append({TEXT("Roll"), TEXT("Backstep")});
	}
	if (Id == TEXT("Jump"))
	{
		Result.Append({TEXT("Jump"), TEXT("Land")});
	}
	if (Id == TEXT("Guard"))
	{
		Result.Add(TEXT("Parry"));
	}
	if (Id == TEXT("KnockUp"))
	{
		Result.Append({TEXT("KnockUp"), TEXT("KnockUpFall"), TEXT("GetUp")});
	}
	if (Id == TEXT("Groggy"))
	{
		Result.Append({TEXT("Groggy"), TEXT("Executed")});
	}
	if (Id == TEXT("Death") && Data.IsA<UAgBossData>())
	{
		Result.Append({TEXT("Executed"), TEXT("ExecutedDeath")});
	}
	return Result;
}
