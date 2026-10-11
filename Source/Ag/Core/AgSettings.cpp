// Copyright Woogle. All Rights Reserved.

#include "Core/AgSettings.h"

#include "Data/AgCameraData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgDataFile.h"

namespace
{
	/** 공통 데이터 파일을 그 클래스의 기본 객체에 처음 한 번 읽어 넣습니다(구현 계획서 '데이터'). 읽지 못하면 기본값으로 남습니다. */
	template <typename TData>
	const TData& GetCommonData(bool& bLoaded, const FString& FileName)
	{
		TData& Data = *GetMutableDefault<TData>();
		if (!bLoaded)
		{
			bLoaded = true;
			AgDataFile::Load(Data, FileName);
		}
		return Data;
	}
}

const UAgCameraData& UAgSettings::GetCameraData() const
{
	return GetCommonData<UAgCameraData>(bCameraDataLoaded, CameraDataFile);
}

const UAgCombatRules& UAgSettings::GetCombatRules() const
{
	return GetCommonData<UAgCombatRules>(bCombatRulesLoaded, CombatRulesFile);
}

#if WITH_EDITOR
void UAgSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 바뀐 데이터 파일은 같은 기본 객체에 다시 읽습니다. 바뀐 TP_Enemy는 플레이어가 다음 PIE에서 로드합니다.
	const FName Property = PropertyChangedEvent.GetMemberPropertyName();
	if (Property == GET_MEMBER_NAME_CHECKED(UAgSettings, CameraDataFile) && bCameraDataLoaded)
	{
		AgDataFile::Load(*GetMutableDefault<UAgCameraData>(), CameraDataFile);
	}
	if (Property == GET_MEMBER_NAME_CHECKED(UAgSettings, CombatRulesFile) && bCombatRulesLoaded)
	{
		AgDataFile::Load(*GetMutableDefault<UAgCombatRules>(), CombatRulesFile);
	}
}
#endif
