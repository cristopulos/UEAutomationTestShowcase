// Automation Showcase — original showcase code (UE 5.7.4).

// Pattern A: per-test world setup/teardown
// Pattern B: latent asynchronous setup
//
// Tests in this file demonstrate automation tests that need INITIAL SETUP
// before running:
//   Pattern A - each test creates its own transient UWorld and actor(s),
//               asserts against them, and explicitly tears everything down
//               (following Epic's own pattern in
//               Engine/Source/Runtime/Engine/Private/Tests/EngineAutomationTests.cpp:
//               UWorld::CreateWorld(EWorldType::Game, false) + DestroyWorld(false)).
//   Pattern B - a latent (asynchronous) command is enqueued during RunTest;
//               the automation framework drains the latent queue over editor
//               frames after RunTest returns, so follow-up assertions live in
//               a second latent command that runs after the setup command.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
#include "Components/PointLightComponent.h"
// Project-relative include path: UBT exposes the "Source" root as an include
// directory, so subfolder test files reference module headers via the module name.
#include "AutomationShowcase/ShowcaseDamageSystem.h"

// Pattern A world lifecycle helper (mirrors Epic's FTestWorldWrapper in
// Engine/Source/Runtime/Engine/Private/Tests/AutomationCommon.cpp):
// CreateWorld + GEngine->CreateNewWorldContext + SetCurrentWorld, so spawned
// actors have a proper world context when later destroyed (avoids the benign
// but noisy "UWorld::DestroyActor: World has no context" warning).
static UWorld* CreateShowcasePatternAWorld()
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (IsValid(TestWorld) && GEngine)
	{
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(TestWorld);
	}
	return TestWorld;
}

static void DestroyShowcasePatternAWorld(UWorld* TestWorld)
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
// Pattern A, test 1: transient world + actor spawn + explicit teardown
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSetupWorldAndActorSpawnTest,
	"AutomationShowcase.Setup.WorldAndActorSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseSetupWorldAndActorSpawnTest::RunTest(const FString& Parameters)
{
	// ---- SETUP: create a transient game world (same lifecycle Epic's engine
	// tests use: CreateWorld + a dedicated world context, torn down with
	// DestroyWorldContext + DestroyWorld(false)).
	UWorld* TestWorld = CreateShowcasePatternAWorld();
	TestTrue(TEXT("Setup: transient test world was created"), IsValid(TestWorld));

	// ---- SETUP: spawn a plain actor into the fresh world.
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* SpawnedActor = TestWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	TestTrue(TEXT("Setup: actor spawned into the transient world"), IsValid(SpawnedActor));
	if (!IsValid(SpawnedActor))
	{
		// Tear down what exists before bailing out so we never leak the world.
		DestroyShowcasePatternAWorld(TestWorld);
		return false;
	}

	// ---- ASSERT: actor exists in the world it was spawned into.
	TestTrue(TEXT("Spawned actor reports a valid world (GetWorld())"), IsValid(SpawnedActor->GetWorld()));
	TestTrue(TEXT("Spawned actor belongs to the transient test world"), SpawnedActor->GetWorld() == TestWorld);

	// ---- TEARDOWN part 1: explicit actor destruction.
	SpawnedActor->Destroy();
	TestFalse(TEXT("Teardown: actor is no longer valid after Destroy()"), IsValid(SpawnedActor));

	// ---- TEARDOWN part 2: destroy the transient world itself so nothing leaks.
	DestroyShowcasePatternAWorld(TestWorld);

	return true;
}

// ============================================================================
// Pattern A, test 2: actor component registration lifecycle
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSetupActorComponentLifecycleTest,
	"AutomationShowcase.Setup.ActorComponentLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseSetupActorComponentLifecycleTest::RunTest(const FString& Parameters)
{
	// ---- SETUP: transient world (with its own world context) + owning actor.
	UWorld* TestWorld = CreateShowcasePatternAWorld();
	TestTrue(TEXT("Setup: transient test world was created"), IsValid(TestWorld));

	AActor* OwnerActor = nullptr;
	if (IsValid(TestWorld))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		OwnerActor = TestWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	}
	TestTrue(TEXT("Setup: owner actor spawned"), IsValid(OwnerActor));
	if (!IsValid(OwnerActor))
	{
		DestroyShowcasePatternAWorld(TestWorld);
		return false;
	}

	// ---- SETUP: create a component, attach it to the actor, register it.
	UPointLightComponent* LightComponent = NewObject<UPointLightComponent>(OwnerActor);
	TestTrue(TEXT("Setup: component created via NewObject"), IsValid(LightComponent));
	if (!IsValid(LightComponent))
	{
		OwnerActor->Destroy();
		DestroyShowcasePatternAWorld(TestWorld);
		return false;
	}

	OwnerActor->AddInstanceComponent(LightComponent);
	LightComponent->RegisterComponent();
	TestTrue(TEXT("Setup: component registered via RegisterComponent()"), LightComponent->IsRegistered());

	// ---- ASSERT lifecycle: registered -> destroyed -> unregistered.
	TestTrue(TEXT("Component is registered after RegisterComponent()"), LightComponent->IsRegistered());
	LightComponent->DestroyComponent();
	TestFalse(TEXT("Component is unregistered after DestroyComponent()"), LightComponent->IsRegistered());

	// ---- TEARDOWN: explicit actor + world cleanup (actor Destroy also runs
	// DestroyComponent on any remaining owned components).
	OwnerActor->Destroy();
	DestroyShowcasePatternAWorld(TestWorld);

	return true;
}

// ============================================================================
// Pattern B: latent asynchronous setup
// ============================================================================
//
// The framework drains latent commands AFTER RunTest returns, over subsequent
// editor frames (FAutomationWorkerModule::ExecuteLatentCommands pumps the FIFO
// queue each tick and only calls StopTest once the queue is empty). Therefore:
//   - command 1 performs the "setup work" incrementally (here: pump a counter
//     once per Update() call, N calls total - a world-free deterministic
//     stand-in for tick-based setup to stay robust in headless runs),
//   - command 2 runs after command 1 completes and performs the assertions
//     against the setup results via FAutomationTestFramework::GetCurrentTest().

// Shared counter for the Pattern B pump/verify pair. A latent test runs on the
// game thread, so no locking is required; file-static keeps it private while
// letting both command classes read it.
static int32 ShowcasePumpCounterValue = 0;

// Pattern B setup command: completes after N Update() calls, incrementing the
// shared counter once per call.
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FShowcasePumpTicksCommand, int32, TicksToPump);

bool FShowcasePumpTicksCommand::Update()
{
	++ShowcasePumpCounterValue;
	return ShowcasePumpCounterValue >= TicksToPump;
}

// Pattern B assertion command: runs after the setup command completed and
// verifies the setup side effects through the currently-executing test.
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FShowcaseVerifyTicksCommand, int32, ExpectedTicks);

bool FShowcaseVerifyTicksCommand::Update()
{
	// Report through the active test so failures land in the test report.
	if (FAutomationTestBase* CurrentTest = FAutomationTestFramework::Get().GetCurrentTest())
	{
		CurrentTest->TestEqual(TEXT("Latent setup ran the pump command once per frame for the expected number of updates"),
			static_cast<float>(ShowcasePumpCounterValue), static_cast<float>(ExpectedTicks), 0.001f);
	}
	return true; // assertions done - complete immediately
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSetupLatentTickSequenceTest,
	"AutomationShowcase.Setup.LatentTickSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShowcaseSetupLatentTickSequenceTest::RunTest(const FString& Parameters)
{
	// Reset the shared counter so repeated runs stay deterministic.
	ShowcasePumpCounterValue = 0;

	// ---- SETUP (deferred): enqueue the setup command; it performs one unit of
	// "setup work" per Update() call and completes after 5 calls.
	const int32 TicksToPump = 5;
	ADD_LATENT_AUTOMATION_COMMAND(FShowcasePumpTicksCommand(TicksToPump));

	// ---- ASSERT (deferred): enqueued after the setup command, so the FIFO
	// latent queue guarantees it runs only after setup completed.
	ADD_LATENT_AUTOMATION_COMMAND(FShowcaseVerifyTicksCommand(TicksToPump));

	// Nothing to assert synchronously - the demonstration IS the deferral.
	return true;
}