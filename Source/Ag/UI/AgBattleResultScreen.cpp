// Copyright Woogle. All Rights Reserved.

#include "UI/AgBattleResultScreen.h"

#include "Animation/WidgetAnimation.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatStatsComponent.h"
#include "CommonTextBlock.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCharacterData.h"
#include "Game/AgGameMode.h"
#include "Game/AgGameState.h"
#include "Game/AgSimulationRoomGameMode.h"
#include "UI/AgButton.h"

namespace AgBattleResultScreenPrivate
{
	/** 처음 전투에 들어간 순서로 이름을 잇고, 같은 이름이 둘 이상이면 처음 나온 자리에 '× 수'를 붙입니다. */
	FText FormatEnemies(const TArray<FAgCombatEnemyStats>& Enemies)
	{
		TArray<TPair<FText, int32>> Groups;
		for (const FAgCombatEnemyStats& Enemy : Enemies)
		{
			const UAgCharacterData* Data = UAgAssetManager::Get().FindCharacterData(Enemy.Id);
			const FText Name = Data ? Data->DisplayName : FText::FromName(Enemy.Id);
			if (TPair<FText, int32>* Group = Groups.FindByPredicate([&Name](const TPair<FText, int32>& Candidate)
			{ return Candidate.Key.EqualTo(Name); }))
			{
				++Group->Value;
			}
			else
			{
				Groups.Emplace(Name, 1);
			}
		}

		TArray<FText> Parts;
		for (const TPair<FText, int32>& Group : Groups)
		{
			Parts.Add(Group.Value > 1 ? FText::Format(INVTEXT("{0} × {1}"), Group.Key, FText::AsNumber(Group.Value)) : Group.Key);
		}
		return FText::Join(INVTEXT(", "), Parts);
	}
} // namespace AgBattleResultScreenPrivate

void UAgBattleResultScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		RestartButton->OnClicked().AddUObject(GameMode, &AAgGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

void UAgBattleResultScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	const bool bStageCleared = StageState && StageState->GetStageResult() == EAgStageResult::Clear;
	TitleText->SetText(bStageCleared ? StageClearTitle : GameOverTitle);
	ShowCombatStats();

	SetButtonsEnabled(false);
	PlayAnimation(FadeInAnimation);
}

void UAgBattleResultScreen::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == FadeInAnimation)
	{
		SetButtonsEnabled(true);
		RequestRefreshFocus();
	}
}

UWidget* UAgBattleResultScreen::NativeGetDesiredFocusTarget() const
{
	return RestartButton;
}

void UAgBattleResultScreen::ShowCombatStats()
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn());
	const FAgCombatStats* Stats = GetWorld()->GetAuthGameMode<AAgSimulationRoomGameMode>() && Player ? Player->GetCombatStats()->GetStageResultStats() : nullptr;
	CombatStatsTable->SetVisibility(Stats ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!Stats)
	{
		return;
	}
	AttemptText->SetText(FText::AsNumber(Stats->Attempt));
	const int32 Milliseconds = FMath::RoundToInt32(Stats->Duration * 1000.f);
	DurationText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d.%03d"), Milliseconds / 60000, Milliseconds / 1000 % 60, Milliseconds % 1000)));
	EnemiesText->SetText(AgBattleResultScreenPrivate::FormatEnemies(Stats->Enemies));
	ParryCountText->SetText(FText::AsNumber(Stats->ParryCount));
	PerfectDodgeCountText->SetText(FText::AsNumber(Stats->PerfectDodgeCount));
	HitCountText->SetText(FText::AsNumber(Stats->HitCount));
}

void UAgBattleResultScreen::SetButtonsEnabled(bool bEnabled)
{
	for (UAgButton* Button : {RestartButton.Get(), ReturnToTitleButton.Get(), QuitButton.Get()})
	{
		Button->SetIsEnabled(bEnabled);
	}
}
