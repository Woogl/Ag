// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"
#include "Combat/AgCombatLibrary.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgDamageFormulaTest, "Ag.Combat.DamageFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAgDamageFormulaTest::RunTest(const FString& Parameters)
{
	// 전투 시스템 'HP 감소량 계산식': ATK × DamageMultiplier × 100 / (100 + DEF), rounded.
	TestEqual(TEXT("DEF 100 halves the damage"), UAgCombatLibrary::CalculateHPDamage(100.f, 1.f, 100.f), 50);
	TestEqual(TEXT("DEF 0 takes full damage"), UAgCombatLibrary::CalculateHPDamage(150.f, 1.f, 0.f), 150);
	TestEqual(TEXT("Multiplier applies"), UAgCombatLibrary::CalculateHPDamage(100.f, 1.6f, 100.f), 80);
	TestEqual(TEXT("Rounds half up (66.67 -> 67)"), UAgCombatLibrary::CalculateHPDamage(100.f, 1.f, 50.f), 67);
	TestEqual(TEXT("Rounds down below half (93.33 -> 93)"), UAgCombatLibrary::CalculateHPDamage(140.f, 1.f, 50.f), 93);
	TestEqual(TEXT("Guard reduction applies before rounding (25)"), UAgCombatLibrary::CalculateHPDamage(100.f, 1.f, 100.f, 0.5f), 25);
	TestEqual(TEXT("Guard reduction rounding (33.33 -> 33)"), UAgCombatLibrary::CalculateHPDamage(100.f, 1.f, 50.f, 0.5f), 33);

	// 'PP 감소량 계산식': ATK × PoiseMultiplier × 100 / (100 + DEF), rounded.
	TestEqual(TEXT("PP with multiplier 2.4 vs DEF 100"), UAgCombatLibrary::CalculatePPDamage(100.f, 2.4f, 100.f), 120);
	TestEqual(TEXT("PP multiplier is separate from damage"), UAgCombatLibrary::CalculatePPDamage(100.f, 1.f, 100.f), 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgPatternPickTest, "Ag.Boss.PatternPick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAgPatternPickTest::RunTest(const FString& Parameters)
{
	// 보스 사양 '거리 구간별 가중치': chance = weight / sum of candidate weights. Weights 40 and 30 split at 40/70.
	const TArray<float> Weights = { 40.f, 30.f };
	TestEqual(TEXT("Low roll picks the first"), AAgBossAIController::PickWeighted(Weights, 0.f), 0);
	TestEqual(TEXT("Just below 40/70 picks the first"), AAgBossAIController::PickWeighted(Weights, 0.57f), 0);
	TestEqual(TEXT("Just above 40/70 picks the second"), AAgBossAIController::PickWeighted(Weights, 0.58f), 1);
	TestEqual(TEXT("Top roll picks the last"), AAgBossAIController::PickWeighted(Weights, 0.9999f), 1);

	const TArray<float> WithZero = { 0.f, 30.f };
	TestEqual(TEXT("Zero weight is never picked"), AAgBossAIController::PickWeighted(WithZero, 0.f), 1);
	TestEqual(TEXT("No positive weight picks nothing"), AAgBossAIController::PickWeighted(TArray<float>{ 0.f }, 0.5f), INDEX_NONE);
	return true;
}

#endif
