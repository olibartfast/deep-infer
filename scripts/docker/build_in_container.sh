#!/usr/bin/env bash
# Build deep-infer inside the upstream NGC container with the repo mounted from the host.
# Usage: ./build_in_container.sh [auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|x86-ds9.1|jetson-ds9.0|jetson-ds9.1]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./common.sh
source "${SCRIPT_DIR}/common.sh"

ROOT_DIR="$(repo_root)"
resolve_target_profile "${1:-auto}"
require_docker_access

IMAGE_NAME="${IMAGE_NAME:-${BASE_IMAGE}}"
mapfile -t DOCKER_GPU_ARGS < <(docker_gpu_args)

docker run --rm -it \
  "${DOCKER_GPU_ARGS[@]}" \
  --network host \
  --ipc=host \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
  -v "${ROOT_DIR}:/workspace/deep-infer" \
  -w /workspace/deep-infer \
  "${IMAGE_NAME}" \
  /bin/bash -lc "
    apt-get update &&
    if [ -x /opt/nvidia/deepstream/deepstream/user_additional_install.sh ]; then
      /opt/nvidia/deepstream/deepstream/user_additional_install.sh;
    fi &&
    apt-get install -y build-essential cmake ninja-build pkg-config libopencv-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libglib2.0-dev libde265-0 libx265-199 libjbig-dev &&
    # Base image ships libjbig0 as "installed" but without libjbig.so.0 on disk.
    apt-get install --reinstall -y libjbig0 &&
    apt-get install --reinstall -y libflac8 libmp3lame0 libxvidcore4 ffmpeg &&
    rm -rf build &&
    cmake -S . -B build -GNinja -DDEEPSTREAM_DIR=${DEEPSTREAM_DIR} -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda &&
    cmake --build build
  "
