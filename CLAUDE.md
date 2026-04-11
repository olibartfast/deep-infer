# CLAUDE.md

This file is persistent repository memory for AI coding agents. Read it before making changes.

## Scope

- Applies to the whole repository.
- `AGENTS.md` mirrors this file. If you update one, update the other in the same change.

## Project memory

- This repository is a C++17 NVIDIA DeepStream application built with CMake.
- `vision-core` is fetched automatically at configure time through CMake `FetchContent`.
- DeepStream is auto-detected from common install paths, or it can be supplied with `-DDEEPSTREAM_DIR=...`.
- When DeepStream is available, the build uses `src/deepstream/DeepStreamPipeline.cpp`.
- When DeepStream is not available, the build falls back to `src/deepstream/DeepStreamPipelineStub.cpp` so the project still builds with a clear runtime limitation.

## Working rules

- Make surgical changes and preserve any user edits already present in the worktree.
- Reuse existing CMake, include, and source layout patterns instead of introducing new tooling or parallel build systems.
- Update `README.md` whenever behavior, CLI usage, or build instructions change.
- Prefer repository facts from `README.md`, `CMakeLists.txt`, and nearby code over assumptions.

## Useful commands

- Configure: `cmake -S . -B build -GNinja`
- Configure with an explicit DeepStream install: `cmake -S . -B build -GNinja -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-8.0`
- Build: `cmake --build build`
