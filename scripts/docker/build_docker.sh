#!/bin/bash
# Build Docker image for DeepStream Inference Lab
# Usage: ./build_docker.sh [deepstream_version]

DEEPSTREAM_VERSION=${1:-7.1}

echo "Building DeepStream Inference Lab Docker image..."
echo "DeepStream Version: ${DEEPSTREAM_VERSION}"

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
