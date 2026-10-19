// Copyright Woogle. All Rights Reserved.

#include "Data/AgMontageTimeline.h"
#include "Data/AgTaskParams.h"
#include "Animation/AnimMontage.h"

const FAgMontageTimeline* UAgMontageTimelineData::Find(UAnimMontage* Montage) const
{
	return Timelines.Find(Montage);
}

bool AgMontageTimeline::ResolveTimes(const UAnimMontage& Montage, FName Section, const FAgTaskParams& Params, float& OutStart, float& OutEnd)
{
	const int32 SectionIndex = Montage.GetSectionIndex(Section);
	const int32 EndIndex = Params.EndSection.IsNone() ? SectionIndex : Montage.GetSectionIndex(Params.EndSection);
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
	OutStart = SectionStart + (Params.Start ? *Params.Start : Params.Time);
	OutEnd = Params.End ? EndSectionStart + *Params.End : EndSectionEnd;
	return true;
}
