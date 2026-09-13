# Requirements — Dependency Version Environment

Spec: `specs/2026-09-13-dependency-version-env/requirements.md` · Branch: `develop` · Roadmap: Phase 1

## Goal

Make `versions.env` the single source of truth for third-party dependency
versions and base images, wire the build and operational consumers to it, and
pin `neuriplo-tasks` to `v0.8.2` instead of the floating `master` ref.

## In Scope

- [R-1] A repo-root `versions.env` exists in plain `KEY=VALUE` form (no shell
  evaluation) and pins: `neuriplo-tasks` repository + version (`v0.8.2`),
  toolchain minimums (CMake, CUDA, TensorRT, GStreamer, OpenCV), platform
  baselines (Ubuntu, JetPack, L4T), DeepStream install root + baseline/fallback
  versions, all NGC DeepStream base image tags, and the CI CUDA image.
- [R-2] `CMakeLists.txt` reads `versions.env` and uses `NEURIPLO_TASKS_REPO` /
  `NEURIPLO_TASKS_VERSION` for the `FetchContent` declaration; `GIT_TAG master`
  is gone. DeepStream search paths are derived from `DEEPSTREAM_INSTALL_ROOT`
  plus the baseline/fallback versions.
- [R-3] `scripts/docker/common.sh` sources `versions.env`; profile resolution
  takes `BASE_IMAGE` from the pinned NGC keys and derives `DEEPSTREAM_VERSION`
  and `DEEPSTREAM_DIR` without duplicating version literals. Existing
  `${VAR:-default}` override semantics are preserved.
- [R-4] Container/CI surfaces that cannot source `versions.env` (root
  `Dockerfile`, `.devcontainer/*`, `.github/workflows/ci.yml`) are updated to the
  pinned baseline values and remain consistent with `versions.env`.
- [R-5] A committed checker, `scripts/check_dependency_pins.sh`, validates the
  pinning invariants and is runnable locally and in CI without a GPU, CUDA, or
  DeepStream.
- [R-6] `README.md` documents `versions.env` as the source of truth and states
  the pinned `neuriplo-tasks` version.

## Out of Scope

- Full end-to-end build/run verification against the DeepStream 9.1 container
  (Phase 2; requires GPU/SDK).
- Generating JSON/YAML consumers from `versions.env` (deferred; `tech-stack.md`
  Q-1).
- Adopting new `neuriplo-tasks` 0.8 features (polygon output, UINT8 input
  handling, `decodeImage`, YOLO depth).
- Changing application behavior, CLI, or result decoding.

## Decisions

- [D-1] `versions.env` is plain `KEY=VALUE`, not shell: one format consumable by
  both `source` and CMake string parsing. Rationale: avoids a second parser and
  shell-evaluation hazards.
- [D-2] The specifier owns `versions.env` and the acceptance checker; workers
  only wire consumers. Rationale: acceptance must not be editable by what it
  scores (`orchestrate-ai-coding-workflows` contract).
- [D-3] `neuriplo-tasks` is pinned to the release tag `v0.8.2` (latest release,
  2026-09-11), not `develop`/`master`. Rationale: reproducible builds.
- [D-4] JSON/YAML consumers keep checked literals validated by the checker
  rather than being generated now. Rationale: GitHub Actions `container.image`
  and devcontainer `build.args` cannot read a repo file at parse time.
- [D-5] `DEEPSTREAM_INSTALL_ROOT` + version constructs install dirs; the
  existing `/opt/nvidia/deepstream/deepstream-<ver>` layout is unchanged.

## Constraints

- C++17, CMake >= 3.19, existing `FetchContent` mechanism (no new package
  manager).
- DeepStream-absent builds must still configure to the stub pipeline.
- Shell scripts stay `set -euo pipefail`-safe; values in `versions.env` contain
  no spaces, `$`, backticks, or quotes.
- README updated per AGENTS.md.
- Do not touch `src/` application code; the pinned API surface is unchanged
  between v0.6.1 and v0.8.2 (verified: `result_types.hpp`, `tensor_utils.hpp`,
  `vision/geometry.hpp`, `yolo_pose_postprocessor.hpp`,
  `rfdetr_segmentation_postprocessor.hpp` all diff empty across those tags).

## Dependencies

- Codebase: `CMakeLists.txt` (FetchContent), `scripts/docker/common.sh` and its
  callers, `Dockerfile`, `.devcontainer/Dockerfile`,
  `.devcontainer/devcontainer.json`, `.github/workflows/ci.yml`, `README.md`.
- External: `https://github.com/olibartfast/neuriplo-tasks.git` tag `v0.8.2`.

## Context

- `build/_deps/neuriplo-tasks-src` is a stale v0.6.1 checkout; `GIT_TAG master`
  means local builds silently float. (evidence: `build/_deps/.../VERSION` =
  `0.6.1`.)
- `scripts/docker/common.sh:47-95` hardcodes per-profile NGC image tags and
  DeepStream versions. (evidence: file contents.)
- No host CUDA/DeepStream/GStreamer is installed, so acceptance is static plus a
  header/API compile check, not a full build. (evidence: `which`/`ls` probes.)

## Assumptions & Open Questions

- [A-1] The `v0.8.2` tag exists and contains the public headers the app uses —
  Basis: `git ls-remote --tags` and `git ls-tree` confirmed both.
- [A-2] CI may fetch `neuriplo-tasks` from GitHub during `cmake` configure —
  Basis: existing `ci.yml` already configures without vendoring.
- [Q-1] Should CI run `scripts/check_dependency_pins.sh` as a gate? Assumed yes
  for R-4 consistency; maintainer may drop it.

## Definition of Done (requirements level)

- [ ] Every [R-n] is implemented or explicitly deferred with a tracked location
- [ ] No In Scope behavior silently dropped; no Out of Scope work smuggled in
