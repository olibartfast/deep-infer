# Validation — Review Remediation

> Written before implementation.

## Scoreboard — Review Remediation

- **Command (repo-relative)**: `bash scripts/check_dependency_pins.sh`
- **Adjudicates**: R-1 (shell env precedence), R-2 (no hardcoded version),
  R-3 (checkout version match), R-4 (no `source`), R-5 (CMake cleanups).
- **Run per attempt**: exactly once, as the final action.

## Automated Checks

```text
NEURIPLO_TASKS_SRC=/tmp/opencode/neuriplo-v0.8.2 bash scripts/check_dependency_pins.sh
```

- [x] [V-1] Checker does not contain `source "${VERSIONS_ENV}"`; parses into a
      map and exits before consuming values when the format is invalid.
- [x] [V-2] Checker has no hardcoded `vX.Y.Z` pin; it validates the release-tag
      format and uses the file value.
- [x] [V-3] `NEURIPLO_TASKS_SRC` with a mismatched `VERSION` fails the
      conformance check; matching checkout passes.
- [x] [V-4] `common.sh` assigns a file value only when the variable is unset
      (no `source`), and environment overrides are honored.
- [x] [V-5] `CMakeLists.txt` has no `add_definitions`; uses
      `target_compile_definitions(${PROJECT_NAME} PRIVATE ...)`; reconciles
      `CMAKE_MIN_VERSION` via `CMAKE_MINIMUM_REQUIRED_VERSION`.
- [x] [V-6] Phase 1/1b checks still pass.

## Manual Checks

- [x] [M-1] `NGC_IMAGE_X86_DS9_1=custom ./scripts/docker/build_docker.sh x86-ds9.1 --print-config`
      prints `custom`.
- [x] [M-2] `DEEPSTREAM_INSTALL_ROOT=/opt/custom ./scripts/docker/build_docker.sh x86-ds9.1 --print-config`
      prints `/opt/custom/deepstream-9.1`.
- [x] [M-3] `bash -n` clean on `common.sh` and the checker.

## Requirements-to-Evidence Matrix

| ID | Requirement / boundary | Evidence type | Exact command or check | Result | Where recorded |
|----|------------------------|---------------|------------------------|--------|----------------|
| R-1 | shell env precedence | manual + static | M-1/M-2, V-4 | pass | Evidence Log |
| R-2 | no hardcoded version | static | V-2 | pass | Evidence Log |
| R-3 | checkout version match | probe | V-3 | pass | Evidence Log |
| R-4 | no `source` in checker | static | V-1 | pass | Evidence Log |
| R-5 | CMake cleanups | static | V-5 | pass | Evidence Log |
| N-1 | out of scope: pin/app changes | scope diff | diff inspection | confirmed absent | Evidence Log |

## Evidence Log

| ID | Command/Check | Result | Date | Notes |
|----|---------------|--------|------|-------|
| V-1..V-6 | `NEURIPLO_TASKS_SRC=/tmp/opencode/neuriplo-v0.8.2 bash scripts/check_dependency_pins.sh` | pass | 2026-09-13 | `RESULT: PASS (75 ok, 0 skipped)`, exit 0 |
| M-1..M-3 | `NGC_IMAGE_X86_DS9_1=custom ... --print-config`; `DEEPSTREAM_INSTALL_ROOT=/opt/custom ... --print-config`; `bash -n` | pass | 2026-09-13 | `Base image : custom`; `DeepStream dir : /opt/custom/deepstream-9.1` |

## Deviations

- None. Qodo findings #1..#4 and the two skill minors are all resolved and verified.

## Definition of Done (integration)

- [ ] Every automated and manual check executed, with evidence recorded
- [ ] Evidence traced to [R-n]; deviations documented
- [ ] Spec, code, roadmap, and changelog agree — merge as one coherent change
