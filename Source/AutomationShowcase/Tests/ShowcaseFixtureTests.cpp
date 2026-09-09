// Copyright Epic Games, Inc. All Rights Reserved.

// Pattern C: shared actor fixture
//
// Tests in this file build a full gameplay fixture: a transient world, a
// spawned AShowcaseTargetDummy, and a UShowcaseDamageSystem instance acting as
// the shared damage-math fixture. The dummy delegates to the damage system, so
// these tests exercise the whole chain (world -> actor -> system).
//
// NOTE (showcase): fixture tests assert current behavior, including the seeded
// crit bug (1.5x), so they pass - the bug-catching tests in
// ShowcaseDamageSystemTests.cpp are what fail.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
// Project-relative include path: UBT exposes the "Source" root as an include
// directory, so subfolder test files reference module headers via the module name.
#include "AutomationShowcase/ShowcaseDamageSystem.h"
#include "AutomationShowcase/ShowcaseTargetDummy.h"

// Fixture world helper, shared by both Pattern C tests. Follows Epic's own
// test-world lifecycle from
// Engine/Source/Runtime/Engine/Private/Tests/AutomationCommon.cpp (FTestWorldWrapper):
// CreateWorld + GEngine->CreateNewWorldContext + SetCurrentWorld, so spawned
// actors have a proper world context during DestroyActor.
static UWorld* CreateShowcaseFixtureWorld()
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (IsValid(TestWorld) && GEngine)
	{
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(TestWorld);
	}
	return TestWorld;
}

static void DestroyShowcaseFixtureWorld(UWorld* TestWorld)
{
	if (IsValid(TestWorld))
	{
		if (GEngine)
		{
			GEngine->DestroyWorldContext(TestWorld);
		}
		TestWorld->DestroyWorld(false);
	}
}

// ============================================================================
// Pattern C, test 1: world + dummy + shared damage-system fixture
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSetupActorFixtureAppliesDamageTest,
	"AutomationShowcase.Setup.ActorFixtureAppliesDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseSetupActorFixtureAppliesDamageTest::RunTest(const FString& Parameters)
{
	// ---- SETUP: transient world (with its own world context), target dummy,
	// shared damage-system instance.
	UWorld* TestWorld = CreateShowcaseFixtureWorld();

	AShowcaseTargetDummy* TargetDummy = nullptr;
	if (IsValid(TestWorld))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		TargetDummy = TestWorld->SpawnActor<AShowcaseTargetDummy>(AShowcaseTargetDummy::StaticClass(), FTransform::Identity, SpawnParams);
	}
	TestTrue(TEXT("Setup: target dummy spawned with 100 starting health"), IsValid(TargetDummy));
	if (IsValid(TargetDummy))
	{
		TestEqual(TEXT("Setup: dummy starts at 100 health"), TargetDummy->Health, 100.0f, 0.001f);
	}
	else
	{
		if (IsValid(TestWorld))
		{
			TestWorld->DestroyWorld(false);
		}
		return false;
	}

	UShowcaseDamageSystem* DamageSystem = NewObject<UShowcaseDamageSystem>();
	TestTrue(TEXT("Setup: shared UShowcaseDamageSystem fixture created"), IsValid(DamageSystem));

	// Sanity-check the fixture actually delegates to the showcase damage system.
	// NOTE: crits use the CURRENT (buggy) 1.5x multiplier - asserted to match
	// current behavior; the doubling contract is tested (and fails) elsewhere.
	const float ResultHealth = TargetDummy->ApplyDamageToTarget(30.0f, false);
	TestEqual(TEXT("ApplyDamageToTarget(30, non-crit) on 100 health delegates to the damage system and leaves 70"), ResultHealth, 70.0f, 0.001f);
	TestEqual(TEXT("Dummy stores the post-damage health"), TargetDummy->Health, 70.0f, 0.001f);

	// ---- TEARDOWN: explicit cleanup of the fixture (world owns dummy; system
	// is transient and dies with GC).
	if (IsValid(TargetDummy))
	{
		TargetDummy->Destroy();
	}
	DestroyShowcaseFixtureWorld(TestWorld);

	return true;
}

// ============================================================================
// Pattern C, test 2: fixture cleanup verification
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSetupActorFixtureCleanupVerifiesDestroyTest,
	"AutomationShowcase.Setup.ActorFixtureCleanupVerifiesDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseSetupActorFixtureCleanupVerifiesDestroyTest::RunTest(const FString& Parameters)
{
	// ---- SETUP: same fixture shape as the damage test.
	UWorld* TestWorld = CreateShowcaseFixtureWorld();
	TestTrue(TEXT("Setup: transient test world was created"), IsValid(TestWorld));

	// Baseline: a fresh world's level pre-populates a few default (non-null)
	// actors (WorldSettings, default components, etc.) - capture the count so
	// cleanup is verified as a DELTA, not an absolute zero.
	int32 BaselineActorCount = 0;
	if (IsValid(TestWorld) && TestWorld->GetCurrentLevel())
	{
		for (const TObjectPtr<AActor>& ActorEntry : TestWorld->GetCurrentLevel()->Actors)
		{
			if (IsValid(ActorEntry))
			{
				++BaselineActorCount;
			}
		}
	}
	TestTrue(TEXT("Setup: baseline actor count captured"), BaselineActorCount >= 0);

	AShowcaseTargetDummy* TargetDummy = nullptr;
	if (IsValid(TestWorld))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		TargetDummy = TestWorld->SpawnActor<AShowcaseTargetDummy>(AShowcaseTargetDummy::StaticClass(), FTransform::Identity, SpawnParams);
	}
	TestTrue(TEXT("Setup: target dummy spawned"), IsValid(TargetDummy));
	if (!IsValid(TargetDummy))
	{
		DestroyShowcaseFixtureWorld(TestWorld);
		return false;
	}

	// ---- SETUP exercise: apply damage so the fixture has state before teardown.
	const float ResultHealth = TargetDummy->ApplyDamageToTarget(30.0f, false);
	TestEqual(TEXT("ApplyDamageToTarget(30, non-crit) leaves 70 health before cleanup"), ResultHealth, 70.0f, 0.001f);

	// ---- TEARDOWN + verification: Destroy() is immediate for this use
	// (AActor::Destroy -> UWorld::DestroyActor -> MarkAsGarbage synchronously).
	TargetDummy->Destroy();
	TestFalse(TEXT("Teardown: dummy is no longer valid after Destroy()"), IsValid(TargetDummy));

	// ---- ASSERT world cleanup: UWorld::RemoveActor nulls the actor's slot in
	// the level's actor array (it does not shrink it), and a fresh world's
	// level pre-populates default actors - so verify the spawned dummy is gone
	// via the non-null actor-count DELTA returning to the baseline.
	int32 RemainingActorCount = 0;
	if (IsValid(TestWorld) && TestWorld->GetCurrentLevel())
	{
		for (const TObjectPtr<AActor>& ActorEntry : TestWorld->GetCurrentLevel()->Actors)
		{
			if (IsValid(ActorEntry))
			{
				++RemainingActorCount;
			}
		}
	}
	TestEqual(TEXT("Transient world actor count returned to baseline after dummy Destroy()"),
		RemainingActorCount, BaselineActorCount);

	// ---- TEARDOWN: destroy the transient world itself.
	DestroyShowcaseFixtureWorld(TestWorld);

	return true;
}