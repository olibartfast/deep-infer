# Requirements — Dependency Contract Hardening

Spec: `specs/2026-09-13-dependency-contract-hardening/requirements.md` · Branch: `develop` · Roadmap: Phase 1b

## Goal

Close the gaps a skill-based review found in Phase 1: the `versions.env`
contract is not yet enforced (declared toolchain minimums ignored), not
overridable or typo-safe in CMake, and not honest about the CI image. Make the
contract self-consistent and its failures early and diagnosable.

## In Scope

- [R-1] CMake resolves pinned values with explicit precedence: an existing cache
  value (`-D<KEY>=...`) wins, then the environment (`ENV{<KEY>}`), then
  `versions.env`. File values are never written to the cache (so editing
  `versions.env` takes effect on the next configure). Resolved values are
  printed as configure diagnostics.
- [R-2] CMake fails at configure on a malformed `versions.env` line (non-blank,
  non-comment, not `KEY=VALUE`) and on any missing/empty consumed key, naming
  the file and the offending line or key.
- [R-3] The declared toolchain minimums are enforced at configure:
  `CMAKE_MIN_VERSION` always; `CUDA_MIN_VERSION`, `OPENCV_MIN_VERSION`, and
  `GSTREAMER_VERSION` when `DEEPINFER_ENFORCE_TOOLCHAIN` is `ON` (the default).
  The stub-only CI build passes `-DDEEPINFER_ENFORCE_TOOLCHAIN=OFF` because its
  build image is intentionally below the DeepStream baseline. `TensorRT` is
  documented as informational (no direct discovery in this project).
- [R-4] The `neuriplo-tasks` fetch is shallow (`GIT_SHALLOW`) while retaining
  the reviewable `v0.8.2` tag as the pin.
- [R-5] `README.md` documents the override precedence and an offline build path
  (`FETCHCONTENT_SOURCE_DIR_NEURIPLO-TASKS`); `versions.env` and
  `.github/workflows/ci.yml` state that `CI_CUDA_IMAGE` is a build-only stub
  toolchain intentionally below `CUDA_MIN_VERSION`.
- [R-6] `scripts/check_dependency_pins.sh` asserts R-1..R-4 against the real
  parser (no duplicated parser logic).

## Out of Scope

- Changing the pinned `neuriplo-tasks` version or any application code.
- Enforcing a TensorRT version (no discovery path in this repo).
- Bumping the CI CUDA image to the DeepStream baseline (Phase 2 / container work).
- Generating devcontainer/CI from `versions.env`.

## Decisions

- [D-1] Extract the parser into `cmake/LoadDependencyVersions.cmake` so CMake
  and the checker share one implementation. Rationale: the Phase 1 checker
  duplicated the parser, which can drift.
- [D-2] Precedence is `-D` cache > environment > file, using `DEFINED CACHE{}`
  (CMake >= 3.14; project floor is 3.19). Rationale: avoids the classic
  file-values-in-cache footgun where a later `versions.env` edit is ignored.
- [D-3] Keep the `v0.8.2` tag (user decision) but add `GIT_SHALLOW`; do not move
  to a commit SHA in this phase.
- [D-4] The specifier owns `versions.env`, `scripts/check_dependency_pins.sh`,
  and the specs; workers wire CMake and docs.
- [D-5] Toolchain enforcement is gated by an explicit
  `DEEPINFER_ENFORCE_TOOLCHAIN` option (default `ON`) rather than being
  unconditional. Rationale: the DeepStream baseline pins (CUDA 13.1 / OpenCV
  4.6 / GStreamer 1.24.2) exceed the stub-only CI image (CUDA 12.6.2 / OpenCV
  4.5.4 / GStreamer 1.20.3); the minimums describe the full pipeline, so the
  stub build must be able to opt out explicitly instead of failing.

## Constraints

- C++17, CMake >= 3.19, existing `FetchContent` mechanism.
- DeepStream-absent configure must still succeed to the stub pipeline.
- No changes under `src/` or `include/`; no behavior/CLI changes.
- `versions.env` stays plain `KEY=VALUE` (no spaces, `$`, quotes, backticks).

## Dependencies

- Codebase: `CMakeLists.txt`, `scripts/check_dependency_pins.sh`, `README.md`,
  `versions.env`, `.github/workflows/ci.yml`.
- External: `neuriplo-tasks` tag `v0.8.2` (unchanged).

## Context

- Phase 1 shipped `versions.env` + wiring, accepted at
  `specs/2026-09-13-dependency-version-env/validation.md`.
- Skill review (`engineer-modern-cmake`, `manage-cpp-dependencies`) found:
  toolchain minimums unenforced; README overpromises CMake overrides; the CMake
  parser silently drops malformed lines; `cmake_minimum_required` duplicates
  `CMAKE_MIN_VERSION`; CI image contradicts `CUDA_MIN_VERSION`; offline path
  undefined. (evidence: Phase 1 review.)

## Assumptions & Open Questions

- [A-1] `find_package(CUDA)` exposes `CUDA_VERSION` — Basis: documented
  FindCUDA behavior; the project already uses this module.
- [A-2] `DEFINED CACHE{}` is available at the 3.19 floor — Basis: CMake 3.14
  release notes.
- [Q-1] Should the CI image be bumped now? Assumed no (stub-only CI; Phase 2).

## Definition of Done (requirements level)

- [ ] Every [R-n] is implemented or explicitly deferred with a tracked location
- [ ] No In Scope behavior silently dropped; no Out of Scope work smuggled in
