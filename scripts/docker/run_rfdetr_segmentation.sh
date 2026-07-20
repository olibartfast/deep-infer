#!/usr/bin/env bash
# Run DeepStream RF-DETR segmentation inference using the upstream NGC runtime image.
# Requires the binary built by scripts/docker/build_in_container.sh and the
# rfdetr_seg_small model exported by scripts/setup/export_models.sh seg.
#
# Usage: ./run_rfdetr_segmentation.sh [auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|jetson-ds9.0]
# Requires: NVIDIA driver >= 570.x, nvidia-container-toolkit.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./common.sh
source "${SCRIPT_DIR}/common.sh"

ROOT_DIR="$(repo_root)"
resolve_target_profile "${1:-auto}"
require_docker_access
require_local_binary "${ROOT_DIR}"

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
  -e NVIDIA_VISIBLE_DEVICES=all \
  -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
  -e DISPLAY=${DISPLAY:-:0} \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "${ROOT_DIR}:/workspace/deep-infer" \
  -w /workspace/deep-infer \
  "${IMAGE_NAME}" \
  /bin/bash -lc "
    ${RUNTIME_SETUP} &&
    exec /workspace/deep-infer/build/deep-infer \
      --source=/workspace/deep-infer/people-walking.mp4 \
      --config=/workspace/deep-infer/configs/rfdetr_segmentation_config.txt \
      --model_type=rfdetr_segmentation \
      --labels=/workspace/deep-infer/data/labels/coco.names \
      --output=/workspace/deep-infer/output \
      --verbose
  "
