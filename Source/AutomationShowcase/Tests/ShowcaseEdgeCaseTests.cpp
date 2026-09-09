// Automation Showcase — original showcase code (UE 5.7.4).

#include "Misc/AutomationTest.h"
// Project-relative include path: UBT exposes the "Source" root as an include
// directory, so subfolder test files reference module headers via the module name.
#include "AutomationShowcase/ShowcaseDamageSystem.h"

// Edge-case tests for UShowcaseDamageSystem.
// NOTE (showcase): one of these tests is EXPECTED to fail because it carries a
// deliberately seeded wrong expectation (75). The failing test is the showcase
// deliverable - do NOT change assertion values or "fix" the gameplay code.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesExactTierBoundariesTest,
	"AutomationShowcase.EdgeCases.ExactTierBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesExactTierBoundariesTest::RunTest(const FString& Parameters)
{
	// Boundary contract (implemented consistently in GetDamageTier):
	// "Low"    : Damage < 25.0
	// "Medium" : 25.0 <= Damage < 75.0
	// "High"   : Damage >= 75.0
	TestEqual(TEXT("Zero damage is tier Low"), UShowcaseDamageSystem::GetDamageTier(0.0f), FString(TEXT("Low")));
	TestEqual(TEXT("Just below 25 is tier Low"), UShowcaseDamageSystem::GetDamageTier(24.999f), FString(TEXT("Low")));
	TestEqual(TEXT("Exactly 25 is tier Medium"), UShowcaseDamageSystem::GetDamageTier(25.0f), FString(TEXT("Medium")));
	TestEqual(TEXT("Just below 75 is tier Medium"), UShowcaseDamageSystem::GetDamageTier(74.999f), FString(TEXT("Medium")));
	TestEqual(TEXT("Exactly 75 is tier High"), UShowcaseDamageSystem::GetDamageTier(75.0f), FString(TEXT("High")));

	return true;
}

// EXPECTED-FAILURE (showcase): this INTENTIONAL-BUG (showcase) is a deliberately
// seeded wrong expectation (75) in the test itself - the failure is not the
// test "catching" a missing guard. Negative damage passes through ApplyDamage
// and the [0, CurrentHealth] clamp caps the result at CurrentHealth, so the
// actual result is 100 whether or not a negative-damage guard existed: the
// clamp masks the missing input validation, and the seeded assumption can
// never pass.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesNegativeDamageIsIgnoredTest,
	"AutomationShowcase.EdgeCases.NegativeDamageIsIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesNegativeDamageIsIgnoredTest::RunTest(const FString& Parameters)
{
	// INTENTIONAL-BUG (showcase): deliberately seeded wrong expectation (75) in
	// the test. Whatever the negative-damage policy, the [0, CurrentHealth]
	// clamp caps the result at CurrentHealth: with no guard, 100 - (-25) = 125
	// -> clamp -> 100; with a negative-as-zero guard, 100 - 0 = 100. The
	// asserted 75 is unreachable either way - the assumption contradicts the
	// implementation, and the clamp masks the missing input validation.
	const float ResultHealth = UShowcaseDamageSystem::ApplyDamage(100.0f, -25.0f, false);

	TestEqual(TEXT("Negative damage should be ignored, leaving 75 health"), ResultHealth, 75.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEdgeCasesZeroDamageKeepsHealthTest,
	"AutomationShowcase.EdgeCases.ZeroDamageKeepsHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseEdgeCasesZeroDamageKeepsHealthTest::RunTest(const FString& Parameters)
{
	const float ResultHealth = UShowcaseDamageSystem::ApplyDamage(100.0f, 0.0f, false);

	TestEqual(TEXT("Zero damage should leave health unchanged at 100"), ResultHealth, 100.0f, 0.001f);

	return true;
}