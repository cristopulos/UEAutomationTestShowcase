// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShowcaseDamageSystem.h"

float UShowcaseDamageSystem::ApplyDamage(float CurrentHealth, float IncomingDamage, bool bIsCriticalHit)
{
	// INTENTIONAL-BUG (showcase): critical hit multiplier is 1.5f instead of the
	// specified 2.0f. Expected behavior: crit damage = IncomingDamage * 2.0f.
	// Actual behavior: crit damage = IncomingDamage * 1.5f, so tests that expect
	// doubling on crit will fail (e.g. ApplyDamage(100, 25, true) -> 62.5 not 50).
	const float FinalDamage = bIsCriticalHit ? (IncomingDamage * 1.5f) : IncomingDamage;

	float NewHealth = CurrentHealth - FinalDamage;

	// Clamp overkill damage: health never drops below zero, never rises above CurrentHealth.
	// NOTE (showcase): negative IncomingDamage is intentionally NOT guarded here;
	// it passes through (e.g. 100 - (-25) = 125) and the upper clamp caps the
	// result at CurrentHealth. The EdgeCases test intentionally asserts the
	// missing validation to demonstrate a failing test.
	NewHealth = FMath::Clamp(NewHealth, 0.0f, CurrentHealth);

	return NewHealth;
}

bool UShowcaseDamageSystem::CanAffordCost(int32 CurrentCurrency, int32 Cost)
{
	// Correct implementation: afford means having at least the cost.
	return CurrentCurrency >= Cost;
}

FString UShowcaseDamageSystem::GetDamageTier(float Damage)
{
	// Boundary decision (tested consistently in ShowcaseEdgeCaseTests.cpp):
	// exactly 25.0f -> "Medium", exactly 75.0f -> "High".
	if (Damage >= 75.0f)
	{
		return TEXT("High");
	}
	if (Damage >= 25.0f)
	{
		return TEXT("Medium");
	}
	return TEXT("Low");
}

float UShowcaseDamageSystem::CalculateLootBonus(int32 PlayerLevel, float BaseLoot)
{
	// INTENTIONAL-BUG (showcase): scaling factor is 0.005f instead of the specified
	// 0.05f (typo-level magnitude error). Expected: BaseLoot * (1.0 + 0.05 * Level),
	// e.g. CalculateLootBonus(10, 100.0f) -> 150.0f.
	// Actual: CalculateLootBonus(10, 100.0f) -> 105.0f, so the scaling test fails.
	return BaseLoot * (1.0f + 0.005f * PlayerLevel);
}