# Run ledger — C++ unit-test suite

Revision: `bc06d8c13e68b314798fc3d6eb4c3f57742ad8de` (branch `develop`).
Workflow: `orchestrate-ai-coding-workflows`. Roles: orchestrator, planner,
architect, reviewer (see `.opencode/agent/`).

## Contract

- **Intent** — add a risk-based C++ unit-test suite for deep-infer's host-side
  logic and wire it into the build, CI, and the dependency-pin contract.
- **Constraints** — C++17; CMake >= 3.19 + Ninja; FetchContent only (no second
  package manager); `versions.env` is the single source of truth; no floating
  refs; surgical changes; README updated on build/CLI behavior change; the
  DeepStream-optional stub build must stay green; tests must not need a GPU or
  DeepStream runtime.
- **Roadmap**
  1. P1 — pin GoogleTest; extract `deep-infer-core`; `ConfigManager` seam;
     FetchContent + `gtest_discover_tests` plumbing; utils/config/config-manager
     tests. (orchestrator)
  2. P2 — logger + task-interface tests. (implementer)
  3. P3 — CI wiring, pin-checker generalization, README. (implementer)
  4. P2R — repair architect findings C2 + R2. (implementer)
  5. Architect + reviewer read-only adjudication; single scoreboard run.

## Scoreboard — C++ test suite

- **Command (repo-relative, single invocation)**:
  `rm -rf /tmp/opencode/scoreboard-build && cmake -S . -B /tmp/opencode/scoreboard-build -GNinja -DCMAKE_BUILD_TYPE=Release -DDEEPINFER_ENFORCE_TOOLCHAIN=OFF -DBUILD_TESTING=ON && cmake --build /tmp/opencode/scoreboard-build --parallel && ctest --test-dir /tmp/opencode/scoreboard-build --output-on-failure && bash scripts/check_dependency_pins.sh`
- **Adjudicates**: all unit tests pass; the pinned-ref contract holds; the
  project configures and rebuilds from a clean tree.
- **Run per attempt**: exactly once, as the final action.
- **Result**: `SCOREBOARD_EXIT=0` — 56/56 ctest cases passed;
  `check_dependency_pins.sh` → `RESULT: PASS (81 ok, 0 skipped)`.

## Attempt log

| # | Role | Model tier | Outcome | Notes |
|---|------|-----------|---------|-------|
| 1 | orchestrator (P1) | mid | pass | seam + core lib + plumbing + 3 test files |
| 2 | implementer (P2) | mid | pass | 2 test files; 12 targeted tests |
| 3 | implementer (P3) | mid | pass | CI + checker + README; checker PASS |
| 4 | architect (review) | mid | **reject** | C2 jbig not on tests; R2 pin over-permissive |
| 5 | reviewer (review) | mid | accept | packets compliant; no vacuous tests |
| 6 | implementer (P2R) | mid | pass | C2 + R2 fixed; 56/56 targeted ctest |
| 7 | orchestrator (scoreboard) | mid | **pass** | exit 0; 56/56 + checker 81 ok |

Same-model delegation is used here for isolation/reviewability, not token
savings (per the skill's same-model caveat); the durable output is the contract
and the role definitions, not the model split.

## Findings and resolution

- **C2 (medium, fixed)** — the OpenCV `libjbig` `--whole-archive` workaround was
  applied only to the executable. Fixed by an INTERFACE target `deep-infer-jbig`
  linked by both `deep-infer` and `deep-infer-tests`.
- **R2 (medium, fixed)** — the generalized `GIT_TAG` check did not validate
  `GOOGLETEST_VERSION` as a release tag. Fixed by a mirrored `^vX.Y.Z$` check and
  an anchored `GIT_TAG ${...}` match.

## Residual risk (accepted, low)

- Temp-file names in `tests/test_task_interface.cpp` are not fully hermetic
  under parallel `ctest -j`; tests use distinct names today.
- `${CUDA_INCLUDE_DIRS}` and GStreamer/GLib link libs are propagated PUBLIC from
  `deep-infer-core`; harmless but broader than strictly required.
- CI was not executed in this run (no GitHub runner); the CI file is validated
  by the pin checker's static assertions only.

## Phase 2 — reviewer fixes (modularity / DI)

Scope: M1 + M2 (blocking) and S1–S3 (should-fix) from the modularity/DI
review; C1–C3 explicitly deferred as optional.

- **FA (implementer)** — `ConfigManager`: `ParseResult{config, help_requested}`
  overload with caller-supplied `ostream`/`Logger&`; library never calls
  `exit`; `main` owns the help exit; numeric options throw `ConfigError`
  naming the option; legacy 2-arg wrapper preserved.
- **FB (implementer)** — tensor core: pure helpers + `PostprocessTensors`
  unconditional in `TensorPostprocess.cpp`, now compiled into
  `deep-infer-core`; only the `NvDsInferTensorMeta` adapter stays SDK-gated;
  `DeepStreamPipeline.cpp` classifier deduped to the shared functions;
  `tests/test_tensor_postprocess.cpp` (11 tests).
- **FC (implementer)** — `Logger`: injected sink (`SetOutputStream`, default
  `cout`) and clock (`SetClock`, default `now`); exact-format test;
  `GenerateRandomColors` uses a function-local `mt19937(42)` (no `srand`).

## Scoreboard — fix phase

Same single command as Phase 1, fresh `/tmp/opencode/scoreboard-build`.

- **Result**: `SCOREBOARD_EXIT=0` — 74/74 ctest cases passed
  (56 + 6 ConfigManager + 11 tensor + 1 logger exact-format);
  `check_dependency_pins.sh` → `RESULT: PASS (81 ok, 0 skipped)`.

## Attempt log (continued)

| # | Role | Model tier | Outcome | Notes |
|---|------|-----------|---------|-------|
| 8 | reviewer (modularity/DI) | meta-1.3 | **reject** | M1, M2 blocking; S1–S3 should-fix |
| 9 | implementer (FA) | deepseek-4.1 | pass | help seam + ConfigError; 74/74 targeted |
| 10 | implementer (FB) | deepseek-4.1 | pass | tensor core move; 74/74 targeted |
| 11 | implementer (FC) | deepseek-4.1 | pass | sink/clock + RNG; 31/31 filtered |
| 12 | reviewer (re-check) | meta-1.3 | **accept** | all M/S resolved; no scope creep |
| 13 | orchestrator (scoreboard) | deepseek-4.1 | **pass** | exit 0; 74/74 + checker 81 ok |

Residual (accepted, low): legacy 2-arg `ParseCommandLine` shim still exits
(by design, unused by main/tests); URI/source-type helpers stay private in the
pipeline; RF-DETR multi-tensor path has no dedicated unit test.
