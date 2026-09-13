# Validation — Dependency Contract Hardening

> Written before implementation. Results recorded after the single scoreboard
> run.

## Scoreboard — Dependency Contract Hardening

- **Command (repo-relative)**: `bash scripts/check_dependency_pins.sh`
- **Adjudicates**:
  - [R-1] parser precedence (`-D` > env > file) and diagnostics.
  - [R-2] malformed/missing-key lines fail at configure.
  - [R-3] `CMAKE_MIN_VERSION`, `CUDA_MIN_VERSION`, `OPENCV_MIN_VERSION`,
    `GSTREAMER_VERSION` are enforced behind `DEEPINFER_ENFORCE_TOOLCHAIN`
    (default ON; CI opts out).
  - [R-4] `GIT_SHALLOW` present on the `neuriplo-tasks` fetch.
  - [R-5] README documents precedence + offline + the option; versions.env and
    ci.yml clarify the CI image and the opt-out.
  - [R-6] the checker exercises the real parser.
- **Run per attempt**: exactly once, as the final action.

## Automated Checks

Run from the repository root:

```text
NEURIPLO_TASKS_SRC=/tmp/opencode/neuriplo-v0.8.2 bash scripts/check_dependency_pins.sh
```

- [x] [V-1] Parser module exists and `CMakeLists.txt` includes it.
- [x] [V-2] `cmake -P` valid-file probe sets `NEURIPLO_TASKS_VERSION` to the pin.
- [x] [V-3] `cmake -P` malformed-line probe exits non-zero.
- [x] [V-4] `cmake -P` cache-precedence probe: `-DNEURIPLO_TASKS_VERSION` wins
      over the file.
- [x] [V-5] `cmake -P` env-precedence probe: `NEURIPLO_TASKS_VERSION` from the
      environment wins over the file.
- [x] [V-6] `CMakeLists.txt` declares `DEEPINFER_ENFORCE_TOOLCHAIN` (default ON),
      enforces the four toolchain minimums under that gate, and uses
      `GIT_SHALLOW`.
- [x] [V-7] README documents precedence, the offline path, and
      `DEEPINFER_ENFORCE_TOOLCHAIN`; `ci.yml` passes
      `-DDEEPINFER_ENFORCE_TOOLCHAIN=OFF` to the stub configure.
- [x] [V-8] Phase 1 checks (pin, wiring, container/CI, docs, API conformance)
      still pass.
- [x] [V-9] The malformed-line probe fixture contains all required keys plus a
      malformed line, so removing the malformed check would make the checker
      fail (no false pass).

## Manual Checks

- [x] [M-1] `cmake -P` probes cover precedence directly (V-4/V-5); no separate
      manual run required.
- [x] [M-2] `bash -n` on every script (covered by the checker).

## Requirements-to-Evidence Matrix

| ID | Requirement / boundary | Evidence type | Exact command or check | Result | Where recorded |
|----|------------------------|---------------|------------------------|--------|----------------|
| R-1 | precedence + diagnostics | probe | checker V-4/V-5 | pass | Evidence Log |
| R-2 | malformed/missing fails | probe | checker V-3/V-9 | pass | Evidence Log |
| R-3 | toolchain minimums enforced (gated) | static | checker V-6 | pass | Evidence Log |
| R-4 | shallow fetch | static | checker V-6 | pass | Evidence Log |
| R-5 | docs + CI note | static | checker V-7 | pass | Evidence Log |
| R-6 | checker uses real parser | static | checker V-1/V-2 | pass | Evidence Log |
| N-1 | out of scope: version bump / app code | scope diff | diff inspection | confirmed absent | Evidence Log |
| N-2 | out of scope: CI image bump | scope diff | diff inspection | confirmed absent | Evidence Log |

## Evidence Log

| ID | Command/Check | Result | Date | Notes |
|----|---------------|--------|------|-------|
| V-1..V-9 | `NEURIPLO_TASKS_SRC=/tmp/opencode/neuriplo-v0.8.2 bash scripts/check_dependency_pins.sh` | pass | 2026-09-13 | `RESULT: PASS (69 ok, 0 skipped)`, exit 0 |
| N-1 | `git status --porcelain -- src include` | pass | 2026-09-13 | no application-code changes; `versions.env:14` still `v0.8.2` |
| N-2 | diff inspection of `.github/workflows/ci.yml` | pass | 2026-09-13 | image unchanged; only the opt-out flag + comment added |

## Deviations

- Attempt 1 was rejected: unconditional toolchain enforcement would have failed
  the documented stub CI (CUDA 12.6.2 / OpenCV 4.5.4 / GStreamer 1.20.3).
  Requirement R-3 was revised to gate CUDA/OpenCV/GStreamer behind
  `DEEPINFER_ENFORCE_TOOLCHAIN` (default ON; CI sets OFF) and decision D-5 was
  recorded. Re-review accepted.
- Attempt 2's scoreboard failed on a checker defect, not the implementation:
  `check_contains` used `grep -qF "-D"`, which parsed `-D` as a grep flag
  (`grep: unknown devices method`). Fixed to `grep -qF -- "${needle}"`; the
  re-run passed. This was a specifier-owned fix.
- Non-blocking observations from review: `cmake_minimum_required(VERSION 3.19)`
  still duplicates `CMAKE_MIN_VERSION` (guarded by the unconditional check);
  `pkg_check_modules(GSTREAMER ...)` overwrites `GSTREAMER_VERSION` with the
  discovered value after using it as a constraint (not reused); the checker's
  option grep does not assert the default is ON.

## Definition of Done (integration)

- [x] Every automated and manual check executed, with evidence recorded
- [x] Evidence traced to [R-n]; deviations documented
- [x] Spec, code, roadmap, and changelog agree — merge as one coherent change
- [x] Durable discoveries propagated to `tech-stack.md` / `mission.md`
