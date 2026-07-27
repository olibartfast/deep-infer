#!/usr/bin/env bash
# Run DeepStream with RTSP stream using the upstream NGC runtime image.
# Usage: ./run_rtsp_stream.sh <rtsp_url> [auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|x86-ds9.1|jetson-ds9.0|jetson-ds9.1]
# Requires: NVIDIA driver >= 570.x, nvidia-container-toolkit

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./common.sh
source "${SCRIPT_DIR}/common.sh"

ROOT_DIR="$(repo_root)"
TARGET_PROFILE_ARG="${2:-auto}"
require_docker_access
resolve_target_profile "${TARGET_PROFILE_ARG}"
require_local_binary "${ROOT_DIR}"

RTSP_URL=${1:-"rtsp://localhost:8554/stream"}
IMAGE_NAME="${IMAGE_NAME:-${BASE_IMAGE}}"
mapfile -t DOCKER_GPU_ARGS < <(docker_gpu_args)
RUNTIME_SETUP="$(container_runtime_setup)"

docker run --rm \
  "${DOCKER_GPU_ARGS[@]}" \
  --privileged \
  --network host \
  --ipc=host \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -e DISPLAY=${DISPLAY:-:0} \
  -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
  -v "${ROOT_DIR}:/workspace/deep-infer" \
  -w /workspace/deep-infer \
  "${IMAGE_NAME}" \
  /bin/bash -lc "
    ${RUNTIME_SETUP} &&
    exec /workspace/deep-infer/build/deep-infer \
      --source='${RTSP_URL}' \
      --config=/workspace/deep-infer/configs/yolov8_config.txt \
      --model_type=yolov8 \
      --labels=/workspace/deep-infer/data/labels/coco.names \
      --tracker \
      --show \
      --verbose
  "
