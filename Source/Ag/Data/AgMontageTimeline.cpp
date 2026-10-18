// Copyright Woogle. All Rights Reserved.

#include "Data/AgMontageTimeline.h"
#include "AbilitySystem/Tasks/AgAbilityTask_ApplyEffect.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Data/AgJsonRead.h"
#include "Data/AgTaskParams.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#if WITH_EDITOR
#include "AnimNotify_GameplayCue.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Animation/AnimNotifies/AnimNotifyState_DisableRootMotion.h"
#include "Core/AgGameplayTags.h"
#include "GameplayCueInterface.h"
#endif

namespace
{
	using namespace AgJsonRead;

	/** 시간표 시각을 몽타주 길이나 몽타주에 남긴 노티파이의 시각과 비교할 때 같은 시각으로 보는 차이 */
	constexpr float TimeTolerance = 1.e-3f;

	/** 항목 하나: 구간(Start, End)이나 지점(Time)과 그 Task의 Params */
	bool ReadItem(const FJsonObject& Object, FAgTimelineItem& Out, const FString& Path, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		const bool bPoint = Object.HasField(TEXT("Time"));
		Out.bWindow = Object.HasField(TEXT("Start"));
		if (bPoint == Out.bWindow)
		{
			Errors.Add(Path + TEXT(": expected Start (window) or Time (point)"));
			return false;
		}
		if (bPoint)
		{
			Number(Object, TEXT("Time"), Path, Out.Start, Errors);
		}
		else
		{
			Number(Object, TEXT("Start"), Path, Out.Start, Errors);
			// End를 적지 않으면 그 섹션 끝까지이고, 섹션을 넘는 구간은 {"Section", "Time"(선택)}으로 적습니다.
			const TSharedPtr<FJsonValue> End = Object.TryGetField(TEXT("End"));
			Out.bEndAtSectionEnd = !End.IsValid();
			if (End && End->Type == EJson::Number)
			{
				if (Number(Object, TEXT("End"), Path, Out.End, Errors) && Out.End <= Out.Start)
				{
					Errors.Add(Path + TEXT(": Start must be before End"));
				}
			}
			else if (End && End->Type == EJson::Object)
			{
				const TSharedPtr<FJsonObject> EndObject = End->AsObject();
				const FString EndPath = Path + TEXT(".End");
				Keys(*EndObject, EndPath, {TEXT("Section")}, {TEXT("Time")}, Errors);
				FString Section;
				if (!EndObject->TryGetStringField(TEXT("Section"), Section) || FName(*Section).IsNone())
				{
					Errors.Add(EndPath + TEXT(".Section: expected section name"));
				}
				Out.EndSection = FName(*Section);
				Out.bEndAtSectionEnd = !EndObject->HasField(TEXT("Time"));
				if (!Out.bEndAtSectionEnd)
				{
					Number(*EndObject, TEXT("Time"), EndPath, Out.End, Errors);
				}
			}
			else if (End)
			{
				Errors.Add(Path + TEXT(".End: expected a number or a section end object"));
			}
		}
		FString Task;
		Object.TryGetStringField(TEXT("Task"), Task);
		Out.Task = FName(*Task);
		static const TArray<FString> WindowKeys = {TEXT("Start"), TEXT("End")};
		static const TArray<FString> PointKeys = {TEXT("Time")};
		const EAgTaskPlacement Placement = Out.bWindow ? EAgTaskPlacement::Window : EAgTaskPlacement::Point;
		AgTaskParams::Read(Object, Placement, Out.Params, Path, Errors, Out.bWindow ? WindowKeys : PointKeys);
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
		FAgMontageTimeline Timeline;
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& SectionPair : (*Object)->Values)
		{
			const FString SectionName = FString(SectionPair.Key);
			const FString SectionPath = Path + TEXT(".") + SectionName;
			const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
			if (FName(*SectionName).IsNone() || !SectionPair.Value->TryGetArray(Items) || Items->IsEmpty())
			{
				Errors.Add(SectionPath + TEXT(": expected a section name with a nonempty item array"));
				continue;
			}
			FAgTimelineSection& Section = Timeline.Sections.AddDefaulted_GetRef();
			Section.Name = FName(*SectionName);
			for (int32 Index = 0; Index < Items->Num(); ++Index)
			{
				const FString ItemPath = FString::Printf(TEXT("%s[%d]"), *SectionPath, Index);
				const TSharedPtr<FJsonObject>* Item = nullptr;
				if (!(*Items)[Index]->TryGetObject(Item))
				{
					Errors.Add(ItemPath + TEXT(": expected object"));
					continue;
				}
				FAgTimelineItem Value;
				if (ReadItem(**Item, Value, ItemPath, Errors))
				{
					Section.Items.Add(MoveTemp(Value));
				}
			}
		}
		if (Timeline.Sections.IsEmpty())
		{
			Errors.Add(Path + TEXT(": no sections"));
		}
		Candidate.Add(FName(*Path), MoveTemp(Timeline));
	}
	if (Errors.Num() != Before)
	{
		return false;
	}
	Out = MoveTemp(Candidate);
	return true;
}

bool AgMontageTimeline::ResolveTimes(const UAnimMontage& Montage, FName Section, const FAgTimelineItem& Item, float& OutStart,
                                     float& OutEnd)
{
	const int32 SectionIndex = Montage.GetSectionIndex(Section);
	const int32 EndIndex = Item.EndSection.IsNone() ? SectionIndex : Montage.GetSectionIndex(Item.EndSection);
	if (SectionIndex == INDEX_NONE || EndIndex == INDEX_NONE)
	{
		return false;
	}
	float SectionStart = 0.f;
	float SectionEnd = 0.f;
	Montage.GetSectionStartAndEndTime(SectionIndex, SectionStart, SectionEnd);
	float EndSectionStart = 0.f;
	float EndSectionEnd = 0.f;
	Montage.GetSectionStartAndEndTime(EndIndex, EndSectionStart, EndSectionEnd);
	OutStart = SectionStart + Item.Start;
	OutEnd = Item.bEndAtSectionEnd ? EndSectionEnd : EndSectionStart + Item.End;
	return true;
}

TArray<FName> AgMontageTimeline::GetProjectileIds(const FAgMontageTimeline& Timeline)
{
	TArray<FName> Ids;
	for (const FAgTimelineSection& Section : Timeline.Sections)
	{
		for (const FAgTimelineItem& Item : Section.Items)
		{
			if (const FAgSpawnProjectileParams* Shot = Item.Params.GetPtr<FAgSpawnProjectileParams>())
			{
				Ids.AddUnique(Shot->ProjectileId);
			}
		}
	}
	return Ids;
}

void AgMontageTimeline::ValidateEffects(const TMap<FName, FAgMontageTimeline>& Timelines,
                                        const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, TArray<FString>& Errors)
{
	for (const TPair<FName, FAgMontageTimeline>& Pair : Timelines)
	{
		for (const FAgTimelineSection& Section : Pair.Value.Sections)
		{
			for (int32 Index = 0; Index < Section.Items.Num(); ++Index)
			{
				const FAgTimelineItem& Item = Section.Items[Index];
				if (const FAgApplyEffectParams* Apply = Item.Params.GetPtr<FAgApplyEffectParams>())
				{
					Apply->ValidateEffect(Effects, Item.bWindow ? EAgTaskPlacement::Window : EAgTaskPlacement::Point,
					                      FString::Printf(TEXT("%s.%s[%d]"), *Pair.Key.ToString(), *Section.Name.ToString(), Index),
					                      Errors);
				}
			}
		}
	}
}

#if WITH_EDITOR
namespace
{
	/**
	 * Start부터 End까지 몽타주가 루트 모션을 주는지. 몽타주의 어느 시퀀스에든 루트 모션이 있으면, 루트 모션이 없는 시퀀스의 구간도
	 * 엔진은 몽타주 루트 모션(이동 없음)으로 처리해 루트 모션 소스를 쓰지 않으므로, 그 구간을 덮는 루트 모션 끄기 노티파이가 있어야
	 * 루트 모션이 없습니다.
	 */
	bool GivesRootMotion(const UAnimMontage& Montage, float Start, float End)
	{
		return Montage.HasRootMotion() && !Montage.Notifies.ContainsByPredicate(
		                                      [Start, End](const FAnimNotifyEvent& Event)
		                                      {
			                                      return Cast<UAnimNotifyState_DisableRootMotion>(Event.NotifyStateClass) &&
			                                             Event.GetTime() <= Start + TimeTolerance &&
			                                             Event.GetTime() + Event.GetDuration() >= End - TimeTolerance;
		                                      });
	}

	/** 공격 예고 Cue(가드 불가 예고, 패리 예고)의 상태 노티파이입니다. 엔진 노티파이의 Cue 태그는 리플렉션으로만 읽을 수 있습니다. */
	bool IsTelegraphCue(const FAnimNotifyEvent& Event)
	{
		static const FStructProperty* CueProperty =
		    FindFProperty<FStructProperty>(UAnimNotify_GameplayCueState::StaticClass(), TEXT("GameplayCue"));
		UAnimNotifyState* State = Cast<UAnimNotify_GameplayCueState>(Event.NotifyStateClass);
		const FGameplayCueTag* Cue = State && CueProperty ? CueProperty->ContainerPtrToValuePtr<FGameplayCueTag>(State) : nullptr;
		return Cue && (Cue->GameplayCueTag == AgGameplayTags::GameplayCue_UnblockableTelegraph ||
		               Cue->GameplayCueTag == AgGameplayTags::GameplayCue_ParryTelegraph);
	}
}

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

		// 섹션과 길이를 확인하고, 아래 검사에 쓰도록 항목의 시각을 몽타주 시각으로 바꿉니다.
		struct FResolved
		{
			const FAgTimelineItem* Item = nullptr;
			FString Name;
			int32 SectionIndex = INDEX_NONE;
			float Start = 0.f;
			float End = 0.f;
		};
		TArray<FResolved> Items;
		for (const FAgTimelineSection& Section : Pair.Value.Sections)
		{
			const int32 SectionIndex = Montage->GetSectionIndex(Section.Name);
			if (SectionIndex == INDEX_NONE)
			{
				Errors.Add(Path + TEXT(".") + Section.Name.ToString() + TEXT(": the montage has no such section"));
				continue;
			}
			const float SectionLength = Montage->GetSectionLength(SectionIndex);
			for (int32 Index = 0; Index < Section.Items.Num(); ++Index)
			{
				const FAgTimelineItem& Item = Section.Items[Index];
				const FString Name = FString::Printf(TEXT("%s.%s[%d]"), *Path, *Section.Name.ToString(), Index);
				const int32 EndIndex = Item.EndSection.IsNone() ? SectionIndex : Montage->GetSectionIndex(Item.EndSection);
				if (EndIndex == INDEX_NONE)
				{
					Errors.Add(Name + TEXT(".End: the montage has no section ") + Item.EndSection.ToString());
					continue;
				}
				const float EndLength = Montage->GetSectionLength(EndIndex);
				if (!Item.bWindow && Item.Start > SectionLength + TimeTolerance)
				{
					Errors.Add(FString::Printf(TEXT("%s: Time %.4f is after the section length %.4f"), *Name, Item.Start, SectionLength));
				}
				if (Item.bWindow && Item.Start >= SectionLength)
				{
					Errors.Add(
					    FString::Printf(TEXT("%s: Start %.4f must be before the section end %.4f"), *Name, Item.Start, SectionLength));
				}
				if (Item.bWindow && !Item.bEndAtSectionEnd && Item.End > EndLength + TimeTolerance)
				{
					Errors.Add(FString::Printf(TEXT("%s: End %.4f is after the section length %.4f"), *Name, Item.End, EndLength));
				}

				// 전환할 섹션은 이 몽타주에 있고, 전환 지점은 섹션 끝보다 앞입니다(끝에서는 몽타주가 먼저 끝나거나 다음 섹션으로 넘어감).
				const FAgTransition* Transition = Item.Params.Get<FAgTaskParams>().GetTransition();
				if (Transition && Montage->GetSectionIndex(Transition->Section) == INDEX_NONE)
				{
					Errors.Add(Name + TEXT(".Next.Section: the montage has no section ") + Transition->Section.ToString());
				}
				if (Transition && !Item.bWindow && Item.Start >= SectionLength - TimeTolerance)
				{
					Errors.Add(FString::Printf(TEXT("%s: a transition point at %.4f must be before the section end %.4f"), *Name,
					                           Item.Start, SectionLength));
				}
				FResolved& Resolved = Items.AddDefaulted_GetRef();
				Resolved.Item = &Item;
				Resolved.Name = Name;
				Resolved.SectionIndex = SectionIndex;
				ResolveTimes(*Montage, Section.Name, Item, Resolved.Start, Resolved.End);
				if (Item.bWindow && Resolved.End <= Resolved.Start)
				{
					Errors.Add(Name + TEXT(": End must be after Start on the montage timeline"));
				}
			}
		}

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

		// 기다리는 구간은 반복 섹션을 처음부터 끝까지 덮어, 기다리는 동안 몽타주가 끝나거나 구간을 벗어나지 않습니다.
		const TFunction<bool(float, float)> CoversLoopToEnd = [Montage](float Start, float End)
		{
			for (int32 Index = 0; Index < Montage->CompositeSections.Num(); ++Index)
			{
				const FCompositeSection& Section = Montage->CompositeSections[Index];
				float SectionStart = 0.f;
				float SectionEnd = 0.f;
				Montage->GetSectionStartAndEndTime(Index, SectionStart, SectionEnd);
				if (Section.NextSectionName == Section.SectionName && Start <= SectionStart + TimeTolerance &&
				    FMath::IsNearlyEqual(End, SectionEnd, TimeTolerance))
				{
					return true;
				}
			}
			return false;
		};

		// 첫 섹션에서 섹션 링크로 이어지는 섹션. 그 밖의 섹션은 전환으로만 들어가 몽타주 시각으로 때를 정할 수 없습니다.
		TSet<int32> LinkedSections;
		for (int32 Index = Montage->GetSectionIndexFromPosition(0.f); Index != INDEX_NONE && !LinkedSections.Contains(Index);
		     Index = Montage->GetSectionIndex(Montage->CompositeSections[Index].NextSectionName))
		{
			LinkedSections.Add(Index);
		}

		bool bHasTrack = false;
		TArray<float> HitTimes;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			const FResolved& Window = Items[Index];
			const FInstancedStruct& Params = Window.Item->Params;
			const FAgTrackParams* Track = Params.GetPtr<FAgTrackParams>();
			const bool bDash = Params.GetPtr<FAgDashParams>() != nullptr;
			bHasTrack |= Track != nullptr;
			const bool bHit = Params.GetPtr<FAgAttackParams>() || (!Window.Item->bWindow && Params.GetPtr<FAgSpawnProjectileParams>());
			if (bHit && LinkedSections.Contains(Window.SectionIndex))
			{
				HitTimes.Add(Window.Start);
			}
			if (!Window.Item->bWindow && OverlapsLoop(Window.Start, Window.Start))
			{
				Errors.Add(Window.Name + TEXT(": a point can't be inside a looping section"));
			}
			if (Track && OverlapsLoop(Window.Start, Window.End))
			{
				Errors.Add(Window.Name + TEXT(": a tracking window can't overlap a looping section"));
			}
			if (Window.Item->bWindow && Params.Get<FAgTaskParams>().WaitsForTransition() && !CoversLoopToEnd(Window.Start, Window.End))
			{
				Errors.Add(Window.Name + TEXT(": a window waiting for its transition must cover a looping section to its end"));
			}

			// 경쟁: 돌진끼리는 겹치지 않습니다.
			for (int32 Other = Index + 1; Other < Items.Num() && bDash; ++Other)
			{
				const FResolved& OtherWindow = Items[Other];
				if (OtherWindow.Item->Params.GetPtr<FAgDashParams>() && OtherWindow.Start < Window.End && Window.Start < OtherWindow.End)
				{
					Errors.Add(Window.Name + TEXT(": overlaps ") + OtherWindow.Name);
				}
			}

			// 루트 모션 소스로 움직이는 돌진 구간에는 몽타주 루트 모션이 없어야 하고, 몽타주 루트 모션이 없는 추적 구간은 돌기만 합니다.
			if (bDash && GivesRootMotion(*Montage, Window.Start, Window.End))
			{
				Errors.Add(Window.Name + TEXT(": a dash window needs no montage root motion (a root motion off notify over the window)"));
			}
			if (Track && !GivesRootMotion(*Montage, Window.Start, Window.End) &&
			    !(Track->bHasMaxWarpDistance && Track->MaxWarpDistance <= 0.f))
			{
				Errors.Add(Window.Name + TEXT(": a tracking window without montage root motion only turns, so MaxWarpDistance must be 0"));
			}
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
			if (bHasTrack && Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass))
			{
				Errors.Add(Path + TEXT(": tracking windows belong to the timeline, not to engine warping notifies"));
			}
		}

		// 공격 예고는 판정 구간이 시작될 때(투사체는 발사할 때) 끝납니다(보스 사양 '공격 예고'). 전환으로만 들어가는 섹션의 판정
		// (LeapSlam의 착지)은 몽타주 시각으로 때를 정할 수 없어 검사하지 않습니다.
		if (!HitTimes.IsEmpty())
		{
			for (const FAnimNotifyEvent& Event : Montage->Notifies)
			{
				if (!IsTelegraphCue(Event))
				{
					continue;
				}
				const float CueEnd = Event.GetTime() + Event.GetDuration();
				if (!HitTimes.ContainsByPredicate(
				        [CueEnd](float Time)
				        {
					        return FMath::IsNearlyEqual(Time, CueEnd, TimeTolerance);
				        }))
				{
					Errors.Add(FString::Printf(
					    TEXT("%s: the telegraph cue ending at %.4f must end when an attack starts or a projectile spawns"), *Path, CueEnd));
				}
			}
		}
	}
}
#endif
