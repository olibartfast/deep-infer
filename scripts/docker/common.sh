#!/usr/bin/env bash

_COMMON_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_DEEP_INFER_ROOT="$(cd "${_COMMON_SCRIPT_DIR}/../.." && pwd)"

_DEEP_INFER_VERSIONS_ENV="${_DEEP_INFER_ROOT}/versions.env"

# Load pinned KEY=VALUE defaults without evaluating the file. A file value is
# applied only when the same-named variable is unset or empty, so environment
# overrides take precedence.
load_dependency_pins() {
    local file="$1" line key value
    if [[ ! -f "${file}" ]]; then
        echo "Missing ${file}: pinned versions file is required." >&2
        return 1
    fi
    while IFS= read -r line || [[ -n "${line}" ]]; do
        [[ -z "${line}" || "${line}" == \#* ]] && continue
        if [[ ! "${line}" =~ ^([A-Za-z_][A-Za-z0-9_]*)=([^[:space:]]+)$ ]]; then
            echo "Malformed line in ${file}: ${line}" >&2
            return 1
        fi
        key="${BASH_REMATCH[1]}"
        value="${BASH_REMATCH[2]}"
        case "${value}" in
            *'$'* | *'`'* | *'"'* | *"'"*)
                echo "Unsafe value in ${file}: ${line}" >&2
                return 1
                ;;
        esac
        if [[ -z "${!key:-}" ]]; then
            printf -v "${key}" '%s' "${value}"
        fi
    done < "${file}"
}

load_dependency_pins "${_DEEP_INFER_VERSIONS_ENV}" || { return 1 2>/dev/null || exit 1; }

repo_root() {
    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "${script_dir}/../.." && pwd
}

detect_target_profile() {
    local arch
    arch="$(uname -m)"

    if [[ "${arch}" == "aarch64" && -r /etc/nv_tegra_release ]]; then
        # L4T R39.x → JetPack 7.x → DeepStream 9.1
        if grep -q 'R39' /etc/nv_tegra_release 2>/dev/null; then
            echo "jetson-ds9.1"
            return 0
        fi
        # Older L4T → JetPack 6.x → DeepStream 7.1
        echo "jetson-ds7.1"
        return 0
    fi

    if [[ "${arch}" == "x86_64" ]]; then
        echo "x86-ds9.1"
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
                echo "Set one of: jetson-ds7.0, jetson-ds7.1, x86-ds8.0, x86-ds9.0, jetson-ds9.0, x86-ds9.1, jetson-ds9.1" >&2
                return 1
            fi
            requested_profile="${detected_profile}"
            ;;
    esac

    local derived_ds_version="${requested_profile##*-ds}"

    case "${requested_profile}" in
        jetson-ds7.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_JETSON_DS7_0}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds7.0}"
            ;;
        jetson-ds7.1)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_JETSON_DS7_1}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds7.1}"
            ;;
        x86-ds8.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_X86_DS8_0}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-x86-ds8.0}"
            ;;
        x86-ds9.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_X86_DS9_0}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-x86-ds9.0}"
            ;;
        x86-ds9.1)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_X86_DS9_1}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-x86-ds9.1}"
            ;;
        jetson-ds9.0)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_JETSON_DS9_0}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds9.0}"
            ;;
        jetson-ds9.1)
            DEEPSTREAM_VERSION="${DEEPSTREAM_VERSION:-${derived_ds_version}}"
            BASE_IMAGE="${BASE_IMAGE:-${NGC_IMAGE_JETSON_DS9_1}}"
            DEEPSTREAM_DIR="${DEEPSTREAM_DIR:-${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_VERSION}}"
            IMAGE_TAG="${IMAGE_TAG:-jetson-ds9.1}"
            ;;
        *)
            echo "Unsupported target profile: ${requested_profile}" >&2
            echo "Supported profiles: auto, jetson-ds7.0, jetson-ds7.1, x86-ds8.0, x86-ds9.0, x86-ds9.1, jetson-ds9.0, jetson-ds9.1" >&2
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
# The base image records libjbig0 as installed but ships no libjbig.so.0 on
# disk; force a reinstall so libtiff's runtime DT_NEEDED resolves.
apt-get install --reinstall -y libjbig0 libflac8 libmp3lame0 libxvidcore4 ffmpeg
EOF
}
