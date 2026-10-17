// Copyright Woogle. All Rights Reserved.

#include "Data/AgMontageTimeline.h"
#include "Data/AgJsonRead.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#if WITH_EDITOR
#include "AnimNotify_GameplayCue.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Animation/AnimNotifies/AnimNotifyState_DisableRootMotion.h"
#endif

namespace
{
	using namespace AgJsonRead;

	/** 시간표 시각을 몽타주 길이나 몽타주에 남긴 노티파이의 시각과 비교할 때 같은 시각으로 보는 차이 */
	constexpr float TimeTolerance = 1.e-3f;

	/** 기능 이름의 배열을 읽습니다. 배열이 있으면 항목이 하나 이상 있어야 합니다. */
	template <typename T>
	void ReadEntries(const FJsonObject& Timeline, const TCHAR* Name, TArray<T>& Out, const FString& Path, TArray<FString>& Errors,
	    TFunctionRef<bool(const FJsonObject&, T&, const FString&)> ReadEntry)
	{
		if (!Timeline.HasField(Name))
		{
			return;
		}
		const FString Field = Path + TEXT(".") + Name;
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Timeline.TryGetArrayField(Name, Items) || Items->IsEmpty())
		{
			Errors.Add(Field + TEXT(": expected nonempty array"));
			return;
		}
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			const FString Item = FString::Printf(TEXT("%s[%d]"), *Field, Index);
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!(*Items)[Index]->TryGetObject(Object))
			{
				Errors.Add(Item + TEXT(": expected object"));
				continue;
			}
			T Value;
			if (ReadEntry(**Object, Value, Item))
			{
				Out.Add(Value);
			}
		}
	}

	/** 구간 항목: 고정 필드를 읽고 Start < End를 확인합니다. */
	template <typename T> bool ReadWindow(const FJsonObject& Object, T& Out, const FString& Path, TArray<FString>& Errors)
	{
		if (!Read(Object, Out, Path, Errors))
		{
			return false;
		}
		if (Out.Start >= Out.End)
		{
			Errors.Add(Path + TEXT(": Start must be before End"));
			return false;
		}
		return true;
	}

	bool ReadMotionWarping(const FJsonObject& Object, FAgTimelineMotionWarping& Out, const FString& Path, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		Keys(Object, Path, {TEXT("Start"), TEXT("End"), TEXT("TargetingRange"), TEXT("MaxWarpDistance"), TEXT("RotationMethod")}, {}, Errors);
		Number(Object, TEXT("Start"), Path, Out.Start, Errors);
		Number(Object, TEXT("End"), Path, Out.End, Errors);
		// null은 제한 없음입니다(어빌리티와 AbilityTask 'MotionWarping').
		for (const TCHAR* Key : {TEXT("TargetingRange"), TEXT("MaxWarpDistance")})
		{
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Key);
			const bool bLimited = Value && Value->Type != EJson::Null;
			float Limit = 0.f;
			if (bLimited)
			{
				Number(Object, Key, Path, Limit, Errors);
			}
			if (FString(Key) == TEXT("TargetingRange"))
			{
				Out.Params.bHasTargetingRange = bLimited;
				Out.Params.TargetingRange = Limit;
			}
			else
			{
				Out.Params.bHasMaxWarpDistance = bLimited;
				Out.Params.MaxWarpDistance = Limit;
			}
		}
		FString Method;
		Object.TryGetStringField(TEXT("RotationMethod"), Method);
		if (Method == TEXT("ConstantRate"))
		{
			Out.RotationMethod = EMotionWarpRotationMethod::ConstantRate;
		}
		else if (Method == TEXT("Slerp"))
		{
			Out.RotationMethod = EMotionWarpRotationMethod::Slerp;
		}
		else
		{
			Errors.Add(Path + TEXT(".RotationMethod: expected ConstantRate or Slerp"));
		}
		if (Errors.Num() == Before && Out.Start >= Out.End)
		{
			Errors.Add(Path + TEXT(": Start must be before End"));
		}
		return Errors.Num() == Before;
	}

	bool ReadSpawnProjectile(const FJsonObject& Object, FAgTimelineSpawnProjectile& Out, const FString& Path, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		Keys(Object, Path, {TEXT("Time"), TEXT("ProjectileId"), TEXT("Origin"), TEXT("Attack")}, {TEXT("SocketName")}, Errors);
		Number(Object, TEXT("Time"), Path, Out.Time, Errors);
		FString Text;
		if (!Object.TryGetStringField(TEXT("ProjectileId"), Text) || FName(*Text).IsNone())
		{
			Errors.Add(Path + TEXT(".ProjectileId: expected name"));
		}
		Out.ProjectileId = FName(*Text);
		Text.Reset();
		Object.TryGetStringField(TEXT("Origin"), Text);
		const int64 Origin = StaticEnum<EAgProjectileOrigin>()->GetValueByNameString(Text);
		if (Origin == INDEX_NONE)
		{
			Errors.Add(Path + TEXT(".Origin: invalid enum"));
		}
		else
		{
			Out.Origin = static_cast<EAgProjectileOrigin>(Origin);
		}
		Text.Reset();
		const bool bHasSocket = Object.TryGetStringField(TEXT("SocketName"), Text) && !FName(*Text).IsNone();
		if (bHasSocket != (Out.Origin == EAgProjectileOrigin::MeshSocket))
		{
			Errors.Add(Path + TEXT(".SocketName: required only for MeshSocket"));
		}
		Out.SocketName = bHasSocket ? FName(*Text) : NAME_None;
		if (const FObject Attack = ObjectAt(Object, TEXT("Attack"), Path + TEXT(".Attack"), Errors))
		{
			Read(*Attack, Out.Attack, Path + TEXT(".Attack"), Errors);
		}
		return Errors.Num() == Before;
	}
}

const FAgMontageTimeline* UAgMontageTimelineData::Find(const UAnimMontage& Montage) const
{
	return Timelines.Find(FName(*Montage.GetPathName()));
}

bool AgMontageTimeline::Parse(const FJsonObject& Json, TMap<FName, FAgMontageTimeline>& Out, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	TMap<FName, FAgMontageTimeline> Candidate;
	for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Json.Values)
	{
		const FString Path = FString(Pair.Key);
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Path.StartsWith(TEXT("/")) || !Pair.Value->TryGetObject(Object))
		{
			Errors.Add(Path + TEXT(": expected montage object path with an object value"));
			continue;
		}
		const FJsonObject& Entries = **Object;
		AgJsonRead::Keys(Entries, Path, {},
		    {TEXT("Attack"), TEXT("HoldTag"), TEXT("SetCollisionResponseToChannel"), TEXT("Dash"), TEXT("MotionWarping"),
		        TEXT("GameplayEvent"), TEXT("SpawnProjectile")},
		    Errors);
		if (Entries.Values.IsEmpty())
		{
			Errors.Add(Path + TEXT(": no entries"));
		}
		FAgMontageTimeline Timeline;
		ReadEntries<FAgTimelineAttack>(Entries, TEXT("Attack"), Timeline.Attack, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineAttack& Value, const FString& ItemPath)
		    {
			    return ReadWindow(Item, Value, ItemPath, Errors);
		    });
		ReadEntries<FAgTimelineHoldTag>(Entries, TEXT("HoldTag"), Timeline.HoldTag, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineHoldTag& Value, const FString& ItemPath)
		    {
			    return ReadWindow(Item, Value, ItemPath, Errors);
		    });
		ReadEntries<FAgTimelineCollisionResponse>(Entries, TEXT("SetCollisionResponseToChannel"), Timeline.SetCollisionResponseToChannel,
		    Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineCollisionResponse& Value, const FString& ItemPath)
		    {
			    return ReadWindow(Item, Value, ItemPath, Errors);
		    });
		ReadEntries<FAgTimelineDash>(Entries, TEXT("Dash"), Timeline.Dash, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineDash& Value, const FString& ItemPath)
		    {
			    if (!ReadWindow(Item, Value, ItemPath, Errors))
			    {
				    return false;
			    }
			    if (Value.Speed <= 0.f || Value.MaxDistance <= 0.f)
			    {
				    Errors.Add(ItemPath + TEXT(": Speed and MaxDistance must be positive"));
				    return false;
			    }
			    return true;
		    });
		ReadEntries<FAgTimelineMotionWarping>(Entries, TEXT("MotionWarping"), Timeline.MotionWarping, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineMotionWarping& Value, const FString& ItemPath)
		    {
			    return ReadMotionWarping(Item, Value, ItemPath, Errors);
		    });
		ReadEntries<FAgTimelineGameplayEvent>(Entries, TEXT("GameplayEvent"), Timeline.GameplayEvent, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineGameplayEvent& Value, const FString& ItemPath)
		    {
			    if (!AgJsonRead::Read(Item, Value, ItemPath, Errors))
			    {
				    return false;
			    }
			    if (!Value.Event.ToString().StartsWith(TEXT("Event.")))
			    {
				    Errors.Add(ItemPath + TEXT(".Event: expected a tag under Event"));
				    return false;
			    }
			    return true;
		    });
		ReadEntries<FAgTimelineSpawnProjectile>(Entries, TEXT("SpawnProjectile"), Timeline.SpawnProjectile, Path, Errors,
		    [&Errors](const FJsonObject& Item, FAgTimelineSpawnProjectile& Value, const FString& ItemPath)
		    {
			    return ReadSpawnProjectile(Item, Value, ItemPath, Errors);
		    });
		Candidate.Add(FName(*Path), MoveTemp(Timeline));
	}
	if (Errors.Num() != Before)
	{
		return false;
	}
	Out = MoveTemp(Candidate);
	return true;
}

#if WITH_EDITOR
void AgMontageTimeline::ValidateAssets(const TMap<FName, FAgMontageTimeline>& Timelines, TArray<FString>& Errors)
{
	for (const TPair<FName, FAgMontageTimeline>& Pair : Timelines)
	{
		const FString Path = Pair.Key.ToString();
		const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *Path, nullptr, LOAD_NoWarn);
		if (!Montage || Montage->GetPathName() != Path)
		{
			Errors.Add(Path + TEXT(": missing montage or redirected path"));
			continue;
		}
		const FAgMontageTimeline& Timeline = Pair.Value;
		const float Length = Montage->GetPlayLength();
		const TFunction<bool(float, float)> OverlapsLoop = [Montage](float Start, float End)
		{
			for (int32 Index = 0; Index < Montage->CompositeSections.Num(); ++Index)
			{
				const FCompositeSection& Section = Montage->CompositeSections[Index];
				float SectionStart = 0.f;
				float SectionEnd = 0.f;
				Montage->GetSectionStartAndEndTime(Index, SectionStart, SectionEnd);
				if (Section.NextSectionName == Section.SectionName && Start < SectionEnd && SectionStart <= End)
				{
					return true;
				}
			}
			return false;
		};
		const TFunction<void(const FString&, float, float)> CheckWindow = [&](const FString& Item, float Start, float End)
		{
			if (End > Length + TimeTolerance)
			{
				Errors.Add(FString::Printf(TEXT("%s.%s: End %.4f is after the montage length %.4f"), *Path, *Item, End, Length));
			}
		};
		const TFunction<void(const FString&, float)> CheckPoint = [&](const FString& Item, float Time)
		{
			if (Time > Length + TimeTolerance)
			{
				Errors.Add(FString::Printf(TEXT("%s.%s: Time %.4f is after the montage length %.4f"), *Path, *Item, Time, Length));
			}
			if (OverlapsLoop(Time, Time))
			{
				Errors.Add(Path + TEXT(".") + Item + TEXT(": a point can't be inside a looping section"));
			}
		};
		for (int32 Index = 0; Index < Timeline.Attack.Num(); ++Index)
		{
			CheckWindow(FString::Printf(TEXT("Attack[%d]"), Index), Timeline.Attack[Index].Start, Timeline.Attack[Index].End);
		}
		for (int32 Index = 0; Index < Timeline.HoldTag.Num(); ++Index)
		{
			CheckWindow(FString::Printf(TEXT("HoldTag[%d]"), Index), Timeline.HoldTag[Index].Start, Timeline.HoldTag[Index].End);
		}
		for (int32 Index = 0; Index < Timeline.SetCollisionResponseToChannel.Num(); ++Index)
		{
			const FAgTimelineCollisionResponse& Window = Timeline.SetCollisionResponseToChannel[Index];
			CheckWindow(FString::Printf(TEXT("SetCollisionResponseToChannel[%d]"), Index), Window.Start, Window.End);
			for (int32 Other = Index + 1; Other < Timeline.SetCollisionResponseToChannel.Num(); ++Other)
			{
				const FAgTimelineCollisionResponse& OtherWindow = Timeline.SetCollisionResponseToChannel[Other];
				if (OtherWindow.Channel == Window.Channel && OtherWindow.Start < Window.End && Window.Start < OtherWindow.End)
				{
					Errors.Add(FString::Printf(TEXT("%s.SetCollisionResponseToChannel[%d]: overlaps [%d] on the same channel"), *Path,
					    Index, Other));
				}
			}
		}
		for (int32 Index = 0; Index < Timeline.Dash.Num(); ++Index)
		{
			const FAgTimelineDash& Window = Timeline.Dash[Index];
			CheckWindow(FString::Printf(TEXT("Dash[%d]"), Index), Window.Start, Window.End);
			for (int32 Other = Index + 1; Other < Timeline.Dash.Num(); ++Other)
			{
				if (Timeline.Dash[Other].Start < Window.End && Window.Start < Timeline.Dash[Other].End)
				{
					Errors.Add(FString::Printf(TEXT("%s.Dash[%d]: overlaps [%d]"), *Path, Index, Other));
				}
			}
			// 돌진하는 동안 모션의 루트 이동을 쓰지 않도록, 몽타주에 남긴 루트 모션 끄기가 돌진과 함께 시작해야 합니다.
			const bool bRootMotionOff = Montage->Notifies.ContainsByPredicate(
			    [&Window](const FAnimNotifyEvent& Event)
			    {
				    return Cast<UAnimNotifyState_DisableRootMotion>(Event.NotifyStateClass) &&
				           FMath::IsNearlyEqual(Event.GetTime(), Window.Start, TimeTolerance);
			    });
			if (!bRootMotionOff)
			{
				Errors.Add(FString::Printf(TEXT("%s.Dash[%d]: needs a root motion off notify starting with the dash"), *Path, Index));
			}
		}
		for (int32 Index = 0; Index < Timeline.MotionWarping.Num(); ++Index)
		{
			const FAgTimelineMotionWarping& Window = Timeline.MotionWarping[Index];
			CheckWindow(FString::Printf(TEXT("MotionWarping[%d]"), Index), Window.Start, Window.End);
			if (OverlapsLoop(Window.Start, Window.End))
			{
				Errors.Add(FString::Printf(TEXT("%s.MotionWarping[%d]: a warping window can't overlap a looping section"), *Path, Index));
			}
		}
		for (int32 Index = 0; Index < Timeline.GameplayEvent.Num(); ++Index)
		{
			CheckPoint(FString::Printf(TEXT("GameplayEvent[%d]"), Index), Timeline.GameplayEvent[Index].Time);
		}
		for (int32 Index = 0; Index < Timeline.SpawnProjectile.Num(); ++Index)
		{
			CheckPoint(FString::Printf(TEXT("SpawnProjectile[%d]"), Index), Timeline.SpawnProjectile[Index].Time);
		}

		// 몽타주에는 연출과 루트 모션 노티파이만 남습니다. 프로젝트 노티파이나 엔진 워핑 노티파이가 함께 있으면 같은 일이 두 번
		// 일어납니다.
		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			const UObject* Notify = Event.Notify ? static_cast<const UObject*>(Event.Notify) : Event.NotifyStateClass;
			if (Notify && Notify->GetClass()->GetOutermost() == UAgMontageTimelineData::StaticClass()->GetOutermost())
			{
				Errors.Add(Path + TEXT(": a montage with a timeline can't have project notifies (") + Notify->GetClass()->GetName() +
				           TEXT(")"));
			}
			if (!Timeline.MotionWarping.IsEmpty() && Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass))
			{
				Errors.Add(Path + TEXT(": warping windows belong to the timeline, not to engine notifies"));
			}
		}

		// 공격 예고는 판정 구간이 시작될 때(투사체는 발사할 때) 끝납니다(보스 사양 '공격 예고'). 판정을 몽타주가 정하지 않는
		// 패턴(LeapSlam의 착지)은 공격 구간도 발사도 없어 검사하지 않습니다.
		if (!Timeline.Attack.IsEmpty() || !Timeline.SpawnProjectile.IsEmpty())
		{
			for (const FAnimNotifyEvent& Event : Montage->Notifies)
			{
				if (!Cast<UAnimNotify_GameplayCueState>(Event.NotifyStateClass))
				{
					continue;
				}
				const float CueEnd = Event.GetTime() + Event.GetDuration();
				const bool bMatched = Timeline.Attack.ContainsByPredicate(
				                          [CueEnd](const FAgTimelineAttack& Attack)
				                          {
					                          return FMath::IsNearlyEqual(Attack.Start, CueEnd, TimeTolerance);
				                          }) ||
				                      Timeline.SpawnProjectile.ContainsByPredicate(
				                          [CueEnd](const FAgTimelineSpawnProjectile& Shot)
				                          {
					                          return FMath::IsNearlyEqual(Shot.Time, CueEnd, TimeTolerance);
				                          });
				if (!bMatched)
				{
					Errors.Add(FString::Printf(
					    TEXT("%s: the telegraph cue ending at %.4f must end when an attack window starts or a projectile spawns"), *Path,
					    CueEnd));
				}
			}
		}
	}
}
#endif
