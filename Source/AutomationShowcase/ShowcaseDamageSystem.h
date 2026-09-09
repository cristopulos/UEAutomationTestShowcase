// Automation Showcase — original showcase code (UE 5.7.4).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShowcaseDamageSystem.generated.h"

/**
 * Self-contained showcase damage system.
 * Pure calculation helpers - no Tick, no components, no actor required.
 *
 * NOTE (showcase): This class intentionally contains two seeded bugs
 * (ApplyDamage crit multiplier and CalculateLootBonus scaling) so that
 * automation tests can be demonstrated failing. See INTENTIONAL-BUG
 * comments in ShowcaseDamageSystem.cpp.
 */
UCLASS()
class AUTOMATIONSHOWCASE_API UShowcaseDamageSystem : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Applies IncomingDamage to CurrentHealth and returns the new health.
	 * Critical hits double the incoming damage (x2.0) (specified behavior —
	 * implementation currently deviates, see INTENTIONAL-BUG note in the .cpp).
	 * Result is clamped to [0, CurrentHealth] (overkill protection only;
	 * negative damage values are NOT guarded - see EdgeCase test).
	 */
	UFUNCTION(BlueprintCallable, Category = "Showcase|Damage")
	static float ApplyDamage(float CurrentHealth, float IncomingDamage, bool bIsCriticalHit);

	/** Returns true when CurrentCurrency >= Cost. */
	UFUNCTION(BlueprintCallable, Category = "Showcase|Damage")
	static bool CanAffordCost(int32 CurrentCurrency, int32 Cost);

	/**
	 * Maps a damage amount to a tier label:
	 *   "Low"    : Damage < 25.0
	 *   "Medium" : 25.0 <= Damage < 75.0
	 *   "High"   : Damage >= 75.0
	 */
	UFUNCTION(BlueprintCallable, Category = "Showcase|Damage")
	static FString GetDamageTier(float Damage);

	/** Returns BaseLoot scaled by player level: BaseLoot * (1.0 + 0.05 * PlayerLevel) (specified behavior — implementation currently deviates, see INTENTIONAL-BUG note in the .cpp). */
	UFUNCTION(BlueprintCallable, Category = "Showcase|Damage")
	static float CalculateLootBonus(int32 PlayerLevel, float BaseLoot);
};