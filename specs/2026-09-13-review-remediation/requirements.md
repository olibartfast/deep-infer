# Requirements — Review Remediation (Qodo + skill review)

Spec: `specs/2026-09-13-review-remediation/requirements.md` · Branch: `feat/dependency-version-env` · Roadmap: Phase 1c

## Goal

Close the findings from the Qodo deep review on PR #3 and the remaining minor
skill-review items, without changing the dependency pins or application
behavior.

## In Scope

- [R-1] `scripts/docker/common.sh` must not `source` `versions.env` (whose
  unconditional assignments overwrite inherited environment values). Use a
  non-evaluating `KEY=VALUE` loader that assigns a file value only when the
  shell variable is unset/empty, so environment overrides such as
  `NGC_IMAGE_X86_DS9_1`, `DEEPSTREAM_INSTALL_ROOT`, and `BASE_IMAGE` reach
  `resolve_target_profile`. (Qodo #1)
- [R-2] `scripts/check_dependency_pins.sh` must not hardcode a specific
  `neuriplo-tasks` version. Validate that the pin is a release tag
  (`^v[0-9]+\.[0-9]+\.[0-9]+$`) and treat the validated value from
  `versions.env` as authoritative, so a pin update is a one-line edit. (Qodo #2)
- [R-3] The API-conformance check must validate the selected checkout's
  `VERSION` against the pinned version for **both** an explicit
  `NEURIPLO_TASKS_SRC` and the discovered `build/_deps` checkout; on mismatch
  (explicit) fail clearly, and (discovered) skip rather than compile the wrong
  revision. (Qodo #3)
- [R-4] `scripts/check_dependency_pins.sh` must not `source` `versions.env`.
  Parse validated `KEY=VALUE` lines into a map and stop before consuming values
  when format validation fails, so shell-like content is never executed.
  (Qodo #4)
- [R-5] Minor cleanups from the skill review: replace the global
  `add_definitions(-DSHOW_FRAME/-DWRITE_FRAME)` with
  `target_compile_definitions(${PROJECT_NAME} PRIVATE ...)`; and reconcile
  `CMAKE_MIN_VERSION` with `cmake_minimum_required` via CMake's
  `CMAKE_MINIMUM_REQUIRED_VERSION` instead of a duplicated literal.

## Out of Scope

- Changing any pinned version, application code, or CLI behavior.
- Generating JSON/YAML consumers from `versions.env`.
- Bumping the CI image or DeepStream baseline.

## Decisions

- [D-1] The shell and the CMake loader now share the same precedence rule:
  environment (or `-D` cache) over `versions.env`; the file is the default.
- [D-2] The durable checker validates the pin's *shape* and the *correspondence*
  between pin and tested checkout, not a specific version literal; the specific
  value is asserted by the feature spec/PR.
- [D-3] `common.sh` and the checker use `printf -v` / an associative array for
  non-evaluating assignment; no `eval`, no `source` of data files.

## Constraints

- `set -euo pipefail`-safe; `bash -n`-clean; values stay `KEY=VALUE` with no
  spaces, `$`, quotes, or backticks.
- No new dependencies; no `src/`/`include/` changes.

## Dependencies

- Codebase: `scripts/docker/common.sh`, `scripts/check_dependency_pins.sh`,
  `CMakeLists.txt`, `README.md`, `specs/`.

## Context

- Qodo deep review on PR #3: 3 bugs + 1 rule violation (all addressed by R-1..R-4).
- Skill review minors: global `add_definitions`; `CMAKE_MIN_VERSION` duplicate.
- Phase 2 build succeeded in the DS 9.0 container with `neuriplo-tasks` v0.8.2
  (`deep-infer --help` ran). Evidence: PR #3 CI green + local container build.

## Assumptions & Open Questions

- [A-1] `CMAKE_MINIMUM_REQUIRED_VERSION` is set by `cmake_minimum_required` —
  Basis: documented CMake variable.
- [Q-1] Whether to pin a commit SHA instead of the tag — deferred (Qodo
  recommendation: retain the tag unless mutable-tag risk matters).

## Definition of Done (requirements level)

- [ ] Every [R-n] is implemented or explicitly deferred with a tracked location
- [ ] No In Scope behavior silently dropped; no Out of Scope work smuggled in
