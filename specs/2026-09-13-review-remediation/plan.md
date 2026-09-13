# Plan — Review Remediation

## Group 0 — Contract (specifier; before code)

- [T-0] Author this packet with `validation.md` first.
- [T-1] Rework `scripts/check_dependency_pins.sh`:
  - parse `versions.env` without `source` (associative map), abort on format failure (R-4);
  - replace the hardcoded version with release-tag format validation (R-2);
  - validate the conformance checkout `VERSION` for explicit and discovered
    sources (R-3). Deliverable: the updated scoreboard.
  - Checks: `bash scripts/check_dependency_pins.sh` (expected red on R-1/R-5).

## Group 1 — Docker loader (implementer packet A)

- [T-2] `scripts/docker/common.sh`: replace `source versions.env` with a
  non-evaluating `KEY=VALUE` loader that assigns only when the variable is
  unset/empty; keep `${VAR:-...}` defaults and profile resolution. — Deliverable:
  environment overrides win.
  - Checks: `--print-config` with `NGC_IMAGE_X86_DS9_1`/`DEEPSTREAM_INSTALL_ROOT`/`BASE_IMAGE` exported.
- [T-3] `README.md`: ensure the shell precedence wording matches the new loader.

## Group 2 — CMake cleanups (implementer packet B)

- [T-4] `CMakeLists.txt`: replace global `add_definitions` with
  `target_compile_definitions(${PROJECT_NAME} PRIVATE ...)`; reconcile
  `CMAKE_MIN_VERSION` against `CMAKE_MINIMUM_REQUIRED_VERSION`. — Deliverable:
  no global definitions; no duplicated minimum. — Checks: `cmake -P` parse.

## Group 3 — Verification (orchestrator)

- [T-5] Reviewer pass over the remediation diff.
- [T-6] Run the scoreboard once; record evidence and update the roadmap.
- [T-7] Commit, push, and confirm CI on PR #3.

## Notes

- Workers must not edit `versions.env`, the checker, or `specs/`.
- R-1 and R-5 are disjoint from the checker changes in Group 0.
