// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
// Project-relative include path: UBT exposes the "Source" root as an include
// directory, so subfolder test files reference module headers via the module name.
#include "AutomationShowcase/ShowcaseDamageSystem.h"

// Core gameplay tests for UShowcaseDamageSystem.
// NOTE (showcase): two of these tests are EXPECTED to fail because of the
// deliberately seeded bugs in ShowcaseDamageSystem.cpp. The failing tests
// are the showcase deliverable - do NOT fix the gameplay code.

// EXPECTED-FAILURE (showcase): demonstrates a failing test caused by the seeded
// crit-multiplier bug in ApplyDamage (1.5x instead of 2.0x).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseDamageCritDoublesDamageTest,
	"AutomationShowcase.Damage.CritDoublesDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseDamageCritDoublesDamageTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	const float ResultHealth = DamageSystem->ApplyDamage(100.0f, 25.0f, true);

	TestEqual(TEXT("Critical hit 25 damage vs 100 health should double to 50 health remaining"), ResultHealth, 50.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseDamageNonCritReducesHealthTest,
	"AutomationShowcase.Damage.NonCritReducesHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseDamageNonCritReducesHealthTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	const float ResultHealth = DamageSystem->ApplyDamage(100.0f, 25.0f, false);

	TestEqual(TEXT("Non-crit 25 damage vs 100 health should leave 75 health"), ResultHealth, 75.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseDamageHealthNeverNegativeTest,
	"AutomationShowcase.Damage.HealthNeverNegative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseDamageHealthNeverNegativeTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	const float ResultHealth = DamageSystem->ApplyDamage(10.0f, 500.0f, false);

	TestEqual(TEXT("Massive overkill damage should clamp health to exactly zero, never negative"), ResultHealth, 0.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseCurrencyCanAffordCostTest,
	"AutomationShowcase.Currency.CanAffordCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseCurrencyCanAffordCostTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	TestTrue(TEXT("100 currency can afford a 50 cost"), DamageSystem->CanAffordCost(100, 50));
	TestTrue(TEXT("Exactly 50 currency can afford a 50 cost (inclusive boundary)"), DamageSystem->CanAffordCost(50, 50));
	TestFalse(TEXT("49 currency cannot afford a 50 cost"), DamageSystem->CanAffordCost(49, 50));

	return true;
}

// EXPECTED-FAILURE (showcase): demonstrates a failing test caused by the seeded
// loot-scaling typo in CalculateLootBonus (0.005 per level instead of 0.05).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseLootBonusScalesWithLevelTest,
	"AutomationShowcase.Loot.LootBonusScalesWithLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseLootBonusScalesWithLevelTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	const float LootBonus = DamageSystem->CalculateLootBonus(10, 100.0f);

	TestEqual(TEXT("Level 10 should scale 100 base loot by +50% to 150"), LootBonus, 150.0f, 0.001f);

	return true;
}