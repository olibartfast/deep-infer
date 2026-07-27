#!/usr/bin/env bash
# Pull the upstream DeepStream runtime image for this host/profile.
# Usage: ./build_docker.sh [auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|x86-ds9.1|jetson-ds9.0|jetson-ds9.1] [--print-config]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./common.sh
source "${SCRIPT_DIR}/common.sh"

TARGET_PROFILE_ARG="auto"
PRINT_CONFIG=0

while (($#)); do
    case "$1" in
        auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|x86-ds9.1|jetson-ds9.0|jetson-ds9.1)
            TARGET_PROFILE_ARG="$1"
            ;;
        --print-config)
            PRINT_CONFIG=1
            ;;
        *)
            echo "Unknown argument: $1" >&2
            echo "Usage: ./build_docker.sh [auto|jetson-ds7.0|jetson-ds7.1|x86-ds8.0|x86-ds9.0|x86-ds9.1|jetson-ds9.0|jetson-ds9.1] [--print-config]" >&2
            exit 1
            ;;
    esac
    shift
done

resolve_target_profile "${TARGET_PROFILE_ARG}"

echo "Preparing DeepStream runtime image..."
print_target_summary

if [[ "${PRINT_CONFIG}" -eq 1 ]]; then
    exit 0
fi

require_docker_access

docker pull "${BASE_IMAGE}"

echo "Runtime image ready:"
echo "  - ${BASE_IMAGE}"
echo
echo "Build the executable on the host, then run it inside the mounted NGC container with:"
echo "  ./scripts/docker/run_yolo_detection.sh"
