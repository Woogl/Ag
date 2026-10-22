// Copyright Woogle. All Rights Reserved.

#include "Data/AgMontageTimeline.h"
#include "Animation/AnimMontage.h"

const FAgMontageTimeline* UAgMontageTimelineData::Find(UAnimMontage* Montage) const
{
	return Timelines.Find(Montage);
}
