# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-10-11

First tagged release.

### Added
- DeepStream inference application (C++17, CMake) with a GStreamer pipeline for
  file, camera, and RTSP inputs, plus optional tracking, analytics, display, and
  frame output.
- Object detection through `nvinfer`, with results exposed as
  `neuriplo_tasks::Detection`.
- RF-DETR instance segmentation and YOLO26 pose estimation, decoded from raw
  output tensors by the neuriplo-tasks postprocessors.
- Stub pipeline fallback, so the project builds without DeepStream and reports
  the limitation at runtime.
- Docker build and run scripts with Jetson and x86_64 profiles for DeepStream
  9.0 and 9.1, and auto-detection that prefers 9.1.
- DeepStream 9.1 agent skills under `skills/`.
- `versions.env` as the single source of truth for dependency versions and
  base images, shared by CMake (`cmake/LoadDependencyVersions.cmake`) and the
  Docker scripts, with `-D` and environment overrides.
- `scripts/check_dependency_pins.sh` to verify that every consumer agrees with
  `versions.env`; it runs in CI.
- Toolchain minimum checks (CUDA, OpenCV, GStreamer), switchable with
  `-DDEEPINFER_ENFORCE_TOOLCHAIN=OFF` for stub-only builds.
- GitHub Actions CI that builds the stub pipeline and smoke-tests `--help`.
- Getting-started and Jetson setup guides.
- `VERSION` file read by CMake as the project version.

### Changed
- DeepStream 9.1 is the baseline; 9.0 remains a supported fallback.
- neuriplo-tasks (formerly vision-core) is pinned to `v0.8.2` in
  `versions.env` and fetched shallowly, instead of tracking `master`.

### Fixed
- The debug counter reports every result type (segmentation, pose, detection).

[Unreleased]: https://github.com/olibartfast/deep-infer/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/olibartfast/deep-infer/releases/tag/v0.1.0
