#!/usr/bin/env bash

repo_root() {
    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "${script_dir}/../.." && pwd
}

detect_target_profile() {
    local arch
    arch="$(uname -m)"

    if [[ "${arch}" == "aarch64" && -r /etc/nv_tegra_release ]]; then
        # L4T R39.x → JetPack 7.x → DeepStream 9.0
        if grep -q 'R39' /etc/nv_tegra_release 2>/dev/null; then
            echo "jetson-ds9.0"
            return 0
        fi
        # Older L4T → JetPack 6.x → DeepStream 7.1
        echo "jetson-ds7.1"
        return 0
    fi

    if [[ "${arch}" == "x86_64" ]]; then
        echo "x86-ds8.0"
        return 0
    fi

    return 1
}

resolve_target_profile() {
    local requested_profile="${1:-auto}"
    local detected_profile

    case "${requested_profile}" in
        auto)
            if ! detected_profile="$(detect_target_profile)"; then
                echo "Unable to detect a supported DeepStream target for this host." >&2
                echo "Set one of: jetson-ds7.0, jetson-ds7.1, x86-ds8.0, x86-ds9.0, jetson-ds9.0" >&2
                return 1
            fi
            requested_profile="${detected_profile}"
            ;;
    esac

    case "${requested_profile}" in
        jetson-ds7.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-7.0}"
            BASE_IMAGE="${BASE_IMAGE:-nvcr.io/nvidia/deepstream:7.0-triton-multiarch}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-/opt/nvidia/deepstream/deepstream-7.0}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds7.0}"
            ;;
        jetson-ds7.1)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-7.1}"
            BASE_IMAGE="${BASE_IMAGE:-nvcr.io/nvidia/deepstream:7.1-samples-multiarch}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-/opt/nvidia/deepstream/deepstream-7.1}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds7.1}"
            ;;
        x86-ds8.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-8.0}"
            BASE_IMAGE="${BASE_IMAGE:-nvcr.io/nvidia/deepstream:8.0-gc-triton-devel}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-/opt/nvidia/deepstream/deepstream-8.0}"
            IMAGE_TAG="${IMAGE_TAG:-x86-ds8.0}"
            ;;
        x86-ds9.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-9.0}"
            BASE_IMAGE="${BASE_IMAGE:-nvcr.io/nvidia/deepstream:9.0-triton-multiarch}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-/opt/nvidia/deepstream/deepstream-9.0}"
            IMAGE_TAG="${IMAGE_TAG:-x86-ds9.0}"
            ;;
        jetson-ds9.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-9.0}"
            BASE_IMAGE="${BASE_IMAGE:-nvcr.io/nvidia/deepstream:9.0-triton-multiarch}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-/opt/nvidia/deepstream/deepstream-9.0}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds9.0}"
            ;;
        *)
            echo "Unsupported target profile: ${requested_profile}" >&2
            echo "Supported profiles: auto, jetson-ds7.0, jetson-ds7.1, x86-ds8.0, x86-ds9.0, jetson-ds9.0" >&2
            return 1
            ;;
    esac

    TARGET_PROFILE="${requested_profile}"
    export TARGET_PROFILE BASE_IMAGE DEEPSTREAM_VERSION DEEPSTREAM_DIR IMAGE_TAG
}

require_docker_access() {
    if ! docker info >/dev/null 2>&1; then
        echo "Docker daemon is not accessible for the current user." >&2
        echo "Ensure Docker is running and your user is in the docker group." >&2
        return 1
    fi
}

print_target_summary() {
    echo "Target profile : ${TARGET_PROFILE}"
    echo "Base image     : ${BASE_IMAGE}"
    echo "DeepStream dir : ${DEEPSTREAM_DIR}"
    echo "Image tag      : ${IMAGE_TAG}"
}

require_local_binary() {
    local root_dir="${1}"
    local binary_path="${root_dir}/build/deep-infer"

    if [[ ! -x "${binary_path}" ]]; then
        echo "Local executable not found: ${binary_path}" >&2
        echo "Build it on the host first with CMake before running the NGC container." >&2
        return 1
    fi
}

docker_gpu_args() {
    if [[ "$(uname -m)" == "aarch64" && -r /etc/nv_tegra_release ]]; then
        printf '%s\n' "--runtime=nvidia"
    else
        printf '%s\n' "--gpus" "all"
    fi
}

container_runtime_setup() {
    cat <<'EOF'
apt-get update &&
if [ -x /opt/nvidia/deepstream/deepstream/user_additional_install.sh ]; then
  /opt/nvidia/deepstream/deepstream/user_additional_install.sh;
fi &&
apt-get install -y libopencv-dev libde265-0 libx265-199 &&
apt-get install --reinstall -y libflac8 libmp3lame0 libxvidcore4 ffmpeg
EOF
}
