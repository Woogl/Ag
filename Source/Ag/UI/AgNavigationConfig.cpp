// Copyright Woogle. All Rights Reserved.

#include "UI/AgNavigationConfig.h"

FAgNavigationConfig::FAgNavigationConfig()
{
	// 슬레이트 기본 Accept 키에서 Space를 제외해 메뉴 버튼 실행과 캐릭터 점프 키를 분리합니다.
	KeyActionRules.Remove(EKeys::SpaceBar);
}
