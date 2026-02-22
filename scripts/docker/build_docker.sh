#!/bin/bash
# Build Docker image for DeepStream Inference Lab
# Usage: ./build_docker.sh [deepstream_version]
# Default uses the 8.0-gc-triton-devel container (Graph Composer + Triton, development variant)

DEEPSTREAM_VERSION=${1:-8.0}

echo "Building DeepStream Inference Lab Docker image..."
echo "DeepStream Version: ${DEEPSTREAM_VERSION}"
echo "Base image: nvcr.io/nvidia/deepstream:${DEEPSTREAM_VERSION}-gc-triton-devel"

docker build \
  --build-arg DEEPSTREAM_VERSION=${DEEPSTREAM_VERSION} \
  --rm \
  -t deepstream-infer-lab:latest \
  -t deepstream-infer-lab:${DEEPSTREAM_VERSION} \
  -f Dockerfile \
  .

echo "Build complete!"
echo "Image tags:"
echo "  - deepstream-infer-lab:latest"
echo "  - deepstream-infer-lab:${DEEPSTREAM_VERSION}"