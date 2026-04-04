#!/bin/bash
# Build Docker image for DeepStream Inference Lab
# Usage: ./build_docker.sh [deepstream_version]
# Defaults:
#   - x86_64: 8.0-gc-triton-devel base
#   - aarch64 (Jetson): deepstream-l4t:<version>-triton base

set -euo pipefail

ARCH=$(uname -m)
DEFAULT_X86_VERSION=8.0
DEFAULT_JETSON_VERSION=6.4

# If version not provided, pick a sensible default per architecture
if [ -n "${1-}" ]; then
  DEEPSTREAM_VERSION="$1"
else
  if [ "$ARCH" = "aarch64" ]; then
    DEEPSTREAM_VERSION="$DEFAULT_JETSON_VERSION"
  else
    DEEPSTREAM_VERSION="$DEFAULT_X86_VERSION"
  fi
fi

# Allow overriding via BASE_IMAGE env/arg; otherwise choose per arch
if [ -z "${BASE_IMAGE-}" ]; then
  if [ "$ARCH" = "aarch64" ]; then
    BASE_IMAGE="nvcr.io/nvidia/deepstream-l4t:${DEEPSTREAM_VERSION}-triton"
  else
    BASE_IMAGE="nvcr.io/nvidia/deepstream:${DEEPSTREAM_VERSION}-gc-triton-devel"
  fi
fi

echo "Building DeepStream Inference Lab Docker image..."
echo "Architecture    : ${ARCH}"
echo "DeepStream Ver. : ${DEEPSTREAM_VERSION}"
echo "Base image      : ${BASE_IMAGE}"
echo ""
echo "To override the base image explicitly, set BASE_IMAGE=<image> or pass a version."

docker build \
  --build-arg DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION}" \
  --build-arg BASE_IMAGE="${BASE_IMAGE}" \
  --rm \
  -t deepstream-infer-lab:latest \
  -t deepstream-infer-lab:${DEEPSTREAM_VERSION} \
  -f Dockerfile \
  .

echo "Build complete!"
echo "Image tags:"
echo "  - deepstream-infer-lab:latest"
echo "  - deepstream-infer-lab:${DEEPSTREAM_VERSION}"
