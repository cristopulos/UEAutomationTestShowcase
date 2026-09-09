// Automation Showcase — original showcase code (UE 5.7.4).

#include "ShowcaseTargetDummy.h"
#include "ShowcaseDamageSystem.h"

AShowcaseTargetDummy::AShowcaseTargetDummy()
{
	// Minimal fixture actor: no root component or Tick required for the
	// showcase damage scenario; tests only exercise Health and damage math.
	PrimaryActorTick.bCanEverTick = false;
}

float AShowcaseTargetDummy::ApplyDamageToTarget(float Damage, bool bIsCritical)
{
	// Delegate all damage math to the showcase damage system (single source of
	// truth for the showcase, bugs included) and store the result.
	Health = UShowcaseDamageSystem::ApplyDamage(Health, Damage, bIsCritical);
	return Health;
}