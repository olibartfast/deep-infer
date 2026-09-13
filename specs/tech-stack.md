# Tech Stack — Deep Infer

## Fixed (must not be changed per feature)

- Language(s): C++17 (`CMAKE_CXX_STANDARD 17`, required).
- Build: CMake >= 3.19 + Ninja; `FetchContent` for `neuriplo-tasks`.
- Runtime SDK: NVIDIA DeepStream 9.1 baseline, 9.0 fallback; CUDA required.
- Result/model types: `neuriplo-tasks` (`neuriplo_tasks` namespace,
  `neuriplo/tasks/...` include root).
- Media: GStreamer 1.24.2, GLib; OpenCV for host-side frame handling.
- Containers: NVIDIA NGC DeepStream images; CI uses an `nvidia/cuda` devel image.
- Tests: `BUILD_TESTING` option, `tests/` subdirectory; CI runs `--help` smoke.

## Preferred (follow unless there is a recorded reason not to)

- Target-based CMake usage requirements; no global include/link flags.
- Shell scripts use `set -euo pipefail` and source shared helpers.
- Dependency versions live in `versions.env`; consumers override via
  `${VAR:-<pinned>}` so operators can still pin ad hoc.

## Open (decide per feature, record the decision)

- Whether JSON/YAML consumers (devcontainer, GitHub Actions) are generated from
  `versions.env` or validated against it.

## Explicit Non-Choices

- No second package manager (Conan/vcpkg) — `FetchContent` is the mechanism.
- No new build system — CMake only.
- No hidden floating refs: `GIT_TAG master` is not acceptable for releases.

## Constraints (cross-feature)

- CUDA is required for every build; DeepStream is optional and must fall back to
  `DeepStreamPipelineStub.cpp`.
- `neuriplo-tasks` 0.8.x requires OpenCV only through its optional interop
  targets; the core postprocessors used here are OpenCV-free.
- Supported platform baseline: Ubuntu 24.04; JetPack 7.2 / L4T R39.2 on Jetson.
- README.md must be updated whenever build/CLI/dependency behavior changes
  (AGENTS.md working rule).

## Open Questions

- [Q-1] Minimum CUDA/TensorRT versions for the 9.1 baseline are documented but
  not enforced by CMake. Owner: maintainer.

_Revision: 2026-09-13 — initial stack reconstructed from CMakeLists.txt, README.md, AGENTS.md._
