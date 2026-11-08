// Copyright Woogle. All Rights Reserved.

#include "Ag.h"
#include "Data/AgCharacterData.h"
#include "GenericTeamAgentInterface.h"
#include "Modules/ModuleManager.h"

namespace AgPrivate
{
	ETeamAttitude::Type GetTeamAttitude(FGenericTeamId Team, FGenericTeamId OtherTeam)
	{
		if (Team == OtherTeam)
		{
			return ETeamAttitude::Friendly;
		}
		// 적 팀과 중립 팀은 서로 노리지 않습니다. 적중은 Friendly가 아니면 맞으므로, 공격 판정에 들어온 상대는 맞습니다.
		const FGenericTeamId EnemyTeam(static_cast<uint8>(EAgTeam::Enemy));
		const FGenericTeamId NeutralTeam(static_cast<uint8>(EAgTeam::Neutral));
		const bool bIgnored = (Team == EnemyTeam && OtherTeam == NeutralTeam) || (Team == NeutralTeam && OtherTeam == EnemyTeam);
		return bIgnored ? ETeamAttitude::Neutral : ETeamAttitude::Hostile;
	}
} // namespace AgPrivate

class FAgModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		FGenericTeamId::SetAttitudeSolver(&AgPrivate::GetTeamAttitude);
	}

	virtual void ShutdownModule() override
	{
		FGenericTeamId::ResetAttitudeSolver();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FAgModule, Ag, "Ag");

DEFINE_LOG_CATEGORY(LogAg)
