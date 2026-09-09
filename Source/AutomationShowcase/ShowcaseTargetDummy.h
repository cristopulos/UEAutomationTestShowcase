// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShowcaseTargetDummy.generated.h"

/**
 * Pattern C fixture actor: a simple "target dummy" that delegates all damage
 * math to UShowcaseDamageSystem. Used by the Setup.* fixture tests to show a
 * world + actor + shared damage-system fixture scenario.
 *
 * NOTE (showcase): because ApplyDamageToTarget delegates to
 * UShowcaseDamageSystem::ApplyDamage, the seeded crit bug (1.5x) also affects
 * this actor. The fixture tests assert current behavior, including the seeded
 * crit bug, so they pass - the bug-catching tests in
 * ShowcaseDamageSystemTests.cpp are what fail.
 */
UCLASS()
class AUTOMATIONSHOWCASE_API AShowcaseTargetDummy : public AActor
{
	GENERATED_BODY()

public:
	AShowcaseTargetDummy();

	/** Current health of the dummy. */
	UPROPERTY(BlueprintReadOnly, Category = "Showcase|Damage")
	float Health = 100.0f;

	/**
	 * Applies damage via the showcase damage system and stores the resulting
	 * health. Returns the new health value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Showcase|Damage")
	float ApplyDamageToTarget(float Damage, bool bIsCritical);
};