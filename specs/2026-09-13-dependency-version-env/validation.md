# Validation — Dependency Version Environment

> Written before implementation. Results are recorded in the Evidence Log after
> the single scoreboard run.

## Scoreboard — Dependency Version Environment

- **Command (repo-relative)**: `bash scripts/check_dependency_pins.sh`
- **Adjudicates**:
  - [R-1] `versions.env` exists, parses, and pins the required keys.
  - [R-2] CMake uses the pinned neuriplo-tasks repo/version; no `master` ref.
  - [R-3] docker scripts source `versions.env`; no hardcoded NGC image tags.
  - [R-4] container/CI literals match `versions.env`.
  - [R-5] the checker itself is present and runnable without GPU/SDK.
  - [R-6] README documents the source of truth and pinned version.
- **Run per attempt**: exactly once, as the final action.
- **Recorded with**: exit status + the Evidence Log row.

The checker additionally compiles a `neuriplo-tasks` API conformance translation
unit against a checkout at the pinned tag when one is discoverable via
`NEURIPLO_TASKS_SRC` (or the local FetchContent source when its version matches);
otherwise that rung is reported as `SKIP` with a warning.

## Automated Checks

Run from the repository root:

```text
bash scripts/check_dependency_pins.sh
```

- [x] [V-1] Static pin check: `versions.env` parses; required keys present;
      `NEURIPLO_TASKS_VERSION=v0.8.2`.
- [x] [V-2] CMake wiring: `CMakeLists.txt` references `NEURIPLO_TASKS_VERSION`
      and contains no `GIT_TAG ... master`.
- [x] [V-3] Shell wiring: `common.sh` sources `versions.env`, contains no
      `nvcr.io/nvidia/deepstream:` literal, and derives `DEEPSTREAM_VERSION`
      without per-profile literals; `bash -n` passes on every script.
- [x] [V-4] Consumer consistency: devcontainer + CI literals equal the pinned
      baseline / CI image.
- [x] [V-5] Docs: `README.md` mentions `versions.env` and the pinned version.
- [x] [V-6] API conformance: the translation unit that mirrors
      `src/deepstream/TensorPostprocess.cpp` usage compiles
      (`g++ -std=c++17 -fsyntax-only`) against the pinned tag.
- [x] [V-7] Build/packaging: `cmake -P` parse probe evaluates the CMake
      `versions.env` parser without a full configure (no CUDA/DeepStream on this
      host).

## Manual Checks

- [x] [M-1] `./scripts/docker/build_docker.sh x86-ds9.1 --print-config` prints
      the pinned image and `/opt/nvidia/deepstream/deepstream-9.1` without
      pulling (no daemon mutation).
- [x] [M-2] `./scripts/docker/build_docker.sh jetson-ds7.1 --print-config` still
      resolves a legacy profile from `versions.env`.
- [x] [M-3] Override semantics: `BASE_IMAGE=custom ./scripts/docker/build_docker.sh
      x86-ds9.1 --print-config` prints `custom`.

## Requirements-to-Evidence Matrix

| ID | Requirement / boundary | Evidence type | Exact command or check | Result | Where recorded |
|----|------------------------|---------------|------------------------|--------|----------------|
| R-1 | `versions.env` pins required keys | static | `scripts/check_dependency_pins.sh` | pass | Evidence Log V-1 |
| R-2 | CMake uses pinned ref | static + parse | checker + `cmake -P` probe | pass | Evidence Log V-2/V-7 |
| R-3 | docker scripts source env, no literals | static | checker + `bash -n` | pass | Evidence Log V-3 |
| R-4 | container/CI consistency | static | checker consumer section | pass | Evidence Log V-4 |
| R-5 | checker present/runnable | run | `bash scripts/check_dependency_pins.sh` | pass | Evidence Log V-1..V-7 |
| R-6 | README documents env | static | checker docs section | pass | Evidence Log V-5 |
| N-1 | out of scope: full container build | scope diff | diff inspection | confirmed absent | Evidence Log N-1 |
| N-2 | out of scope: 0.8 feature adoption | scope diff | diff inspection | confirmed absent | Evidence Log N-2 |

## Evidence Log

| ID | Command/Check | Result | Date | Notes |
|----|---------------|--------|------|-------|
| V-1..V-7 | `NEURIPLO_TASKS_SRC=/tmp/opencode/neuriplo-v0.8.2 bash scripts/check_dependency_pins.sh` | pass | 2026-09-13 | `RESULT: PASS (55 ok, 0 skipped)`, exit 0 |
| V-6 | API conformance TU (`g++ -std=c++17 -fsyntax-only`) against `v0.8.2` | pass | 2026-09-13 | ran as part of scoreboard; `neuriplo-v0.8.2` = `git archive v0.8.2` |
| M-1 | `./scripts/docker/build_docker.sh x86-ds9.1 --print-config` | pass | 2026-09-13 | image `...9.1-triton-multiarch`, dir `.../deepstream-9.1` |
| M-2 | `./scripts/docker/build_docker.sh jetson-ds7.1 --print-config` | pass | 2026-09-13 | image `...7.1-samples-multiarch`, dir `.../deepstream-7.1` |
| M-3 | `BASE_IMAGE=custom ... x86-ds9.1 --print-config` | pass | 2026-09-13 | `Base image : custom`, override wins |
| N-1 | `git status --porcelain -- src include` and scope diff | pass | 2026-09-13 | no `src/`/`include/` changes; no container build performed (Phase 2) |
| N-2 | scope diff inspection | pass | 2026-09-13 | no polygon/UINT8/decodeImage/depth adoption |

## Deviations

- The first implementation of `scripts/docker/common.sh` duplicated per-profile
  `DEEPSTREAM_VERSION` literals, violating R-3. The reviewer rejected the
  change; the specifier strengthened `scripts/check_dependency_pins.sh` to
  detect it and a fresh worker derived the version from the profile name. The
  re-review accepted the fix.
- V-6 requires a checkout at the pinned tag. On a host without one, the checker
  reports `SKIP`; CI runs it after Configure so `build/_deps/neuriplo-tasks-src`
  satisfies the rung. The recorded run supplied `NEURIPLO_TASKS_SRC`.
- Pre-existing, unchanged, and out of scope: `CUDA_MIN_VERSION=13.1` in
  `versions.env` vs `CI_CUDA_IMAGE=nvidia/cuda:12.6.2...` (CI builds the stub
  pipeline, so a lower CUDA is intentional but undocumented), and the README
  phrasing that CMake honors environment overrides (CMake parses the file; only
  the shell consumers honor env overrides).

## Definition of Done (integration)

- [x] Every automated and manual check executed, with evidence recorded
- [x] Evidence traced to [R-n]; deviations documented
- [x] Spec, code, roadmap, and changelog agree — merge as one coherent change
- [x] Durable discoveries propagated to `tech-stack.md` / `mission.md`
