# Plan — Dependency Contract Hardening

## Group 0 — Contract (specifier; before code)

- [T-0] Author this packet with `validation.md` before implementation.
- [T-1] Clarify `versions.env`: comment that `CI_CUDA_IMAGE` is a build-only
  stub toolchain below the DeepStream baseline. Deliverable: contract comment.
- [T-2] Extend `scripts/check_dependency_pins.sh` to assert R-1..R-4 and to
  exercise the real parser via `cmake -P` (valid, malformed, cache-precedence,
  env-precedence). Deliverable: the scoreboard.
  - Checks: `bash scripts/check_dependency_pins.sh` (expected red before code)

## Group 1 — Parser module and CMake enforcement (implementer packet A)

- [T-3] Create `cmake/LoadDependencyVersions.cmake`: parse, validate, apply
  precedence (`DEFINED CACHE{}` > `ENV{}` > file), never cache file values, and
  print resolved pins. — Deliverable: one reusable parser.
- [T-4] `CMakeLists.txt`: include the module after `project()` and before the
  package lookups; remove the inline parser; enforce `CMAKE_MIN_VERSION`,
  `CUDA_MIN_VERSION`, `OPENCV_MIN_VERSION`, `GSTREAMER_VERSION`; add
  `GIT_SHALLOW TRUE`. — Deliverable: early, diagnosable configure.
  - Checks: `cmake -P` parser probes; `grep` for constraints.

## Group 2 — Documentation (implementer packet B)

- [T-5] `README.md`: document override precedence (`-D` > env > `versions.env`)
  and the offline path (`FETCHCONTENT_SOURCE_DIR_NEURIPLO-TASKS`). — Deliverable:
  docs agree with behavior.
- [T-6] `.github/workflows/ci.yml`: comment that the CI image is build-only and
  intentionally below `CUDA_MIN_VERSION`. — Deliverable: reconciled CI note.
  - Checks: `bash -n` (scripts untouched); checker docs section.

## Group 3 — Verification (orchestrator, once)

- [T-7] Run the scoreboard once as the final action; record evidence in
  `validation.md` and update `specs/roadmap.md`.

## Notes

- Packets A and B touch disjoint paths; A owns `CMakeLists.txt` and
  `cmake/`, B owns `README.md` and `ci.yml`.
- Workers must not edit `versions.env`, `scripts/check_dependency_pins.sh`, or
  `specs/`; those are specifier-owned.
- The Phase 1 acceptance command is unchanged in name; it now covers the
  hardening requirements too.
