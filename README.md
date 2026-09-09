# Automation Showcase — Unreal Engine Automation Test Demo

An Unreal Engine **5.7.4** project demonstrating the built-in **Automation Test system** end-to-end: a small stub gameplay "damage system" (`UShowcaseDamageSystem`) covered by **13 C++ automation tests** — 10 verify correct behavior, **3 intentionally fail** because the gameplay code and one test contain **deliberately seeded bugs** (marked `// INTENTIONAL-BUG` / `// EXPECTED-FAILURE`).

**Purpose:** show what automation tests are for — catching logic regressions early, documenting intent, running headless in CI — how they are written and registered, what failing tests look like, and tests that require initial setup before assertions.

> 📄 **Full documentation (live, rendered):** https://cristopulos.github.io/UEAutomationTestShowcase/AutomationTests.html — purpose, code walkthrough, setup patterns, trigger commands, Epic resources. A captured report of a real run ships in [`Docs/AutomationReport/`](Docs/AutomationReport/index.html) ([view rendered](https://cristopulos.github.io/UEAutomationTestShowcase/AutomationReport/index.html)).

## What's in the suite

| Group | Tests | Status |
|---|---|---|
| Pure-function damage/currency/tier/loot tests | 8 | 5 ✅ pass · 3 ❌ intentional fail |
| Setup-pattern tests (world, latent, fixture) | 5 | 5 ✅ pass |

The 3 failing tests demonstrate what failing automation tests look like. Each carries a seeded bug:

| Failing test | Seeded defect | Expected vs actual |
|---|---|---|
| `Damage.CritDoublesDamage` | crit multiplier is `1.5x` instead of `2.0x` (`ShowcaseDamageSystem.cpp:11`) | 50 vs **62.5** |
| `Loot.LootBonusScalesWithLevel` | scaling typo `0.005f` instead of `0.05f` (`ShowcaseDamageSystem.cpp:52`) | 150 vs **105** |
| `EdgeCases.NegativeDamageIsIgnored` | the **test itself** asserts an unreachable expectation (75) — negatives pass through, the `[0, CurrentHealth]` clamp caps at 100 either way (`ShowcaseEdgeCaseTests.cpp:32`) | 75 vs **100** |

## Setup-pattern tests (require initial setup)

Five `AutomationShowcase.Setup.*` tests demonstrate work that must happen before assertions can run:

- **Pattern A — per-test world setup/teardown** (`ShowcaseSetupWorldTests.cpp`): each test creates a transient `UWorld` (+ world context, mirroring Epic's `FTestWorldWrapper` lifecycle), spawns actors/components, asserts, and explicitly destroys the world — no state leaks between tests.
- **Pattern B — latent asynchronous setup** (`Setup.LatentTickSequence`): `ADD_LATENT_AUTOMATION_COMMAND` enqueues work that runs *after* `RunTest` returns; assertions on deferred results must themselves be latent commands (FIFO queue).
- **Pattern C — shared actor fixture** (`ShowcaseTargetDummy.{h,cpp}` + `ShowcaseFixtureTests.cpp`): an `AShowcaseTargetDummy` actor delegating to `UShowcaseDamageSystem` — world + dummy + damage-system fixture, then cleanup verification.

## Running the tests

**Build first:**
```sh
"/mnt/data/UE 5.7.4/Engine/Binaries/ThirdParty/DotNet/8.0.412/linux-x64/dotnet" \
  "/mnt/data/UE 5.7.4/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" \
  AutomationShowcaseEditor Linux Development \
  -project="$(pwd)/AutomationShowcase.uproject" -WaitMutex
```

**Headless run (CI-friendly):**
```sh
"/mnt/data/UE 5.7.4/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "/.../AutomationShowcase.uproject" \
  -ExecCmds="Automation RunTests AutomationShowcase; quit" \
  -unattended -nopause -nosplash -nullrhi -log \
  -ReportExportPath=/tmp/autoshowcase-report
```
The filter `AutomationShowcase` matches every test whose pretty-name contains it — all 13 tests. Exit code is non-zero when tests fail, which makes this usable as a CI gate. The report path also produces `index.html` + `index.json`.

**List only (no run):** same command with `-ExecCmds="Automation List; quit"`.

**In-editor:** Tools → Test Automation (requires the Functional Testing Editor plugin), or Tools → Session Frontend → Automation tab; filter `AutomationShowcase`, tick tests, Start Tests.

> Engine path in the examples reflects this machine (`/mnt/data/UE 5.7.4`) — adjust to your install.

## Project layout

```
Source/AutomationShowcase/
├── ShowcaseDamageSystem.{h,cpp}    stub gameplay — 2 seeded bugs (INTENTIONAL-BUG)
├── ShowcaseTargetDummy.{h,cpp}     Pattern C fixture actor
└── Tests/
    ├── ShowcaseDamageSystemTests.cpp   5 pure-function tests (2 fail by design)
    ├── ShowcaseEdgeCaseTests.cpp       3 edge-case tests (1 fail by design)
    ├── ShowcaseSetupWorldTests.cpp     Patterns A + B
    └── ShowcaseFixtureTests.cpp        Pattern C
Docs/
├── AutomationTests.html            full documentation page
└── AutomationReport/               captured report from a real headless run
```

## Seeded bugs are the point

The 3 failing tests and 2 seeded gameplay bugs are **by design** — they demonstrate what failing tests look like and how failure messages pinpoint expected vs actual. Each is marked `// INTENTIONAL-BUG` or `// EXPECTED-FAILURE` in code. **This project does not cover fixing them.**

## Further resources

Official Epic docs on the automation test framework:
[Write C++ Tests](https://dev.epicgames.com/documentation/unreal-engine/write-cplusplus-tests-in-unreal-engine) ·
[Automation Test Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine) ·
[Automation System User Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-system-user-guide-in-unreal-engine) ·
[Run Automation Tests](https://dev.epicgames.com/documentation/en-us/unreal-engine/run-automation-tests-in-unreal-engine) ·
[Automation Spec](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-spec-in-unreal-engine) ·
[Screenshot Comparison Tool](https://dev.epicgames.com/documentation/en-us/unreal-engine/screenshot-comparison-tool-in-unreal-engine) ·
[Gauntlet Automation Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gauntlet-automation-framework-in-unreal-engine)

---
*Unreal Engine 5.7.4 · module `AutomationShowcase` · captured run: 13 tests, 10 passed / 3 failed, 0 warnings (2026-09-09)*