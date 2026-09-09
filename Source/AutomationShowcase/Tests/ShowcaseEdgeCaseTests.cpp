// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
// Project-relative include path: UBT exposes the "Source" root as an include
// directory, so subfolder test files reference module headers via the module name.
#include "AutomationShowcase/ShowcaseDamageSystem.h"

// Edge-case tests for UShowcaseDamageSystem.
// NOTE (showcase): one of these tests is EXPECTED to fail - it asserts a
// negative-damage validation guard that the implementation intentionally
// lacks. The failing test is the showcase deliverable - do NOT add the guard.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesExactTierBoundariesTest,
	"AutomationShowcase.EdgeCases.ExactTierBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesExactTierBoundariesTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	// Boundary contract (implemented consistently in GetDamageTier):
	// "Low"    : Damage < 25.0
	// "Medium" : 25.0 <= Damage < 75.0
	// "High"   : Damage >= 75.0
	TestEqual(TEXT("Zero damage is tier Low"), DamageSystem->GetDamageTier(0.0f), FString(TEXT("Low")));
	TestEqual(TEXT("Just below 25 is tier Low"), DamageSystem->GetDamageTier(24.999f), FString(TEXT("Low")));
	TestEqual(TEXT("Exactly 25 is tier Medium"), DamageSystem->GetDamageTier(25.0f), FString(TEXT("Medium")));
	TestEqual(TEXT("Just below 75 is tier Medium"), DamageSystem->GetDamageTier(74.999f), FString(TEXT("Medium")));
	TestEqual(TEXT("Exactly 75 is tier High"), DamageSystem->GetDamageTier(75.0f), FString(TEXT("High")));

	return true;
}

// EXPECTED-FAILURE (showcase): test assumes a negative-damage guard that the
// implementation lacks - shows tests catching missing validation. The test
// itself carries this INTENTIONAL-BUG (showcase): it assumes negative incoming
// damage is treated as zero before applying, but ApplyDamage does not guard
// negative values: 100 - (-25) = 125, clamped to CurrentHealth -> 100,
// which does not match the asserted 75, so the test fails.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesNegativeDamageIsIgnoredTest,
	"AutomationShowcase.EdgeCases.NegativeDamageIsIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesNegativeDamageIsIgnoredTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	// Assumes the system treats negative incoming damage as zero (a guard the
	// implementation lacks): asserted finalDamage = 0, so health stays 75 after
	// the 25 damage in this scenario. In reality the negative damage passes
	// through: 100 - (-25) = 125, then the [0, CurrentHealth] clamp yields 100.
	const float ResultHealth = DamageSystem->ApplyDamage(100.0f, -25.0f, false);

	TestEqual(TEXT("Negative damage should be ignored, leaving 75 health"), ResultHealth, 75.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesZeroDamageKeepsHealthTest,
	"AutomationShowcase.EdgeCases.ZeroDamageKeepsHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesZeroDamageKeepsHealthTest::RunTest(const FString& Parameters)
{
	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();

	const float ResultHealth = DamageSystem->ApplyDamage(100.0f, 0.0f, false);

	TestEqual(TEXT("Zero damage should leave health unchanged at 100"), ResultHealth, 100.0f, 0.001f);

	return true;
}