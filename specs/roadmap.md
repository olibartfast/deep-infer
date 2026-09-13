# Roadmap

Status values: `idea` · `planned` · `in progress` · `done` · `blocked` · `deferred`

## Phase 1 — Dependency version environment

- Status: done — see `specs/2026-09-13-dependency-version-env/validation.md`
  (`RESULT: PASS (55 ok, 0 skipped)`)
- Outcome: `versions.env` is the single source of truth for third-party
  versions and base images; CMake, docker scripts, containers, and CI consume or
  are validated against it; `neuriplo-tasks` is pinned to `v0.8.2`.
- Proves: dependency changes are one-line edits, and the pinned `neuriplo-tasks`
  revision still satisfies the code's API usage.
- Spec: `specs/2026-09-13-dependency-version-env/`
- Branch: `develop`

## Phase 1b — Dependency contract hardening

- Status: done — see `specs/2026-09-13-dependency-contract-hardening/validation.md`
  (`RESULT: PASS (69 ok, 0 skipped)`)
- Outcome: the `versions.env` contract is enforced (gated by
  `DEEPINFER_ENFORCE_TOOLCHAIN`), overridable with explicit precedence,
  typo-safe, and honest about the CI image.
- Proves: a declared version that is not enforced or not overridable is a
  defect, not documentation.
- Spec: `specs/2026-09-13-dependency-contract-hardening/`
- Branch: `develop`

## Phase 2 — Sync consumers and verify the 0.8.x integration

- Status: planned
- Outcome: full end-to-end build/run against `neuriplo-tasks` 0.8.x verified in
  the DeepStream 9.1 container; any compile or behavior drift fixed.
- Proves: the pinned dependency actually builds and runs, not just parses.
- Spec: `specs/2026-09-13-dependency-version-env/` (extends Phase 1)

## Deferred / Revisit

- Generate devcontainer/CI from `versions.env` instead of checked literals
  (blocked on tooling; see `specs/tech-stack.md` Q-1).
- Adopt `neuriplo-tasks` 0.8 polygon segmentation output and UINT8 image-input
  handling where it improves results (no current requirement).

_Revision: 2026-09-13 — initial roadmap created with the dependency-version-env feature._
