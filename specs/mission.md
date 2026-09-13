# Mission — Deep Infer

## Problem

Building a correct NVIDIA DeepStream inference pipeline by hand is repetitive:
every project re-wires file/camera/RTSP sources, `nvinfer`, tracking, OSD, and
result decoding, and every project drifts on which SDK and model-postprocessing
versions it targets.

## Audience

Developers and integrators running DeepStream inference on NVIDIA desktop,
server, and Jetson targets. Explicitly not for users who want a Python training
or model-authoring framework.

## Product Promise

Deep Infer is a small C++17 DeepStream application that builds a GStreamer
pipeline from a source, an `nvinfer` config, and a model/task type, then decodes
results (detection, instance segmentation, pose) through the `neuriplo-tasks`
postprocessors. It must build against a documented SDK baseline and degrade
cleanly when DeepStream is absent (stub executable).

## Success Criteria

- A documented, single source of truth pins every third-party dependency and
  base image the build and containers consume.
- Changing a pinned dependency version is a one-line edit in that file, not a
  search across CMake, scripts, containers, and CI.
- The project builds on the supported DeepStream baseline and falls back to the
  documented stub build when the SDK is unavailable.

## Product Principles

- Reproducibility over convenience: floating dependency refs are defects.
- Surgical, reviewable changes; the existing build and runtime behavior is a
  contract unless a change explicitly records otherwise.

## Open Questions

- [Q-1] Should CI/devcontainer (formats that cannot source `versions.env`) be
  generated from it, or kept as checked literals? Owner: maintainer.

_Revision: 2026-09-13 — initial constitution reconstructed from README.md, CMakeLists.txt, scripts/docker/common.sh, .github/workflows/ci.yml._
