# Plan — Dependency Version Environment

## Group 0 — Contract (specifier; already authored before code)

- [T-0] Create `specs/` constitution and this packet, with `validation.md`
  written before implementation.
- [T-1] Create `versions.env` (plain `KEY=VALUE`) pinning neuriplo-tasks `v0.8.2`
  plus toolchain/platform/DeepStream/NGC/CI versions. Deliverable: the file.
- [T-2] Create `scripts/check_dependency_pins.sh` — the immutable acceptance
  checker (specifier-owned). Deliverable: a runnable static + API-conformance
  check.
  - Checks: `bash scripts/check_dependency_pins.sh`

## Group 1 — Wire the build (implementer packet A)

- [T-3] `CMakeLists.txt`: parse `versions.env` into CMake variables, guard the
  missing-key case, and use `NEURIPLO_TASKS_REPO`/`NEURIPLO_TASKS_VERSION` in
  `FetchContent_Declare`; remove `GIT_TAG master`. Derive DeepStream search
  paths from `DEEPSTREAM_INSTALL_ROOT` + baseline/fallback. — Deliverable:
  configure resolves the pinned ref.
- [T-4] `README.md`: document `versions.env` as the single source of truth and
  the pinned `neuriplo-tasks` version. — Deliverable: docs agree with code.
  - Checks: `scripts/check_dependency_pins.sh` (static section), `cmake -P`
    parse probe.

## Group 2 — Wire shell, containers, and CI (implementer packet B)

- [T-5] `scripts/docker/common.sh`: source `versions.env`; map each profile to
  its pinned `NGC_IMAGE_*`; derive `DEEPSTREAM_VERSION` from the profile and
  `DEEPSTREAM_DIR` from `DEEPSTREAM_INSTALL_ROOT`; preserve `${VAR:-default}`
  overrides. — Deliverable: `--print-config` still prints a valid summary.
- [T-6] `Dockerfile`, `.devcontainer/Dockerfile`, `.devcontainer/devcontainer.json`,
  `.github/workflows/ci.yml`: replace stale DeepStream `7.1`/`8.0` literals with
  the pinned baseline, and make the CI image match `CI_CUDA_IMAGE`; add the
  checker as a CI step. — Deliverable: checked literals match `versions.env`.
  - Checks: `bash -n` on scripts; `scripts/check_dependency_pins.sh` (consumer
    section).

## Group 3 — Verification (orchestrator, once)

- [T-7] Run the single acceptance scoreboard once as the final action; record
  the result and evidence in `validation.md`.
- [T-8] Update `specs/roadmap.md` Phase 1 status and `README.md` if behavior
  changed.

## Notes

- Two implementer packets touch disjoint paths and may run concurrently; both
  read `versions.env` (read-only) authored in Group 0.
- The worker must not edit `versions.env`, `scripts/check_dependency_pins.sh`,
  or anything under `specs/`; those are specifier-owned.
- No application code under `src/` or `include/` is in scope.
