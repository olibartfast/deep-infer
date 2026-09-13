#!/usr/bin/env bash
# Verify the dependency-version contract for deep-infer.
#
# `versions.env` is the single source of truth for third-party versions and base
# images. This checker asserts that the build, shell, container, and CI
# consumers agree with it, and (when a matching checkout is available) that the
# code's neuriplo-tasks API usage still compiles against the pinned tag.
#
# Runs without a GPU, CUDA, or DeepStream. Usage:
#   bash scripts/check_dependency_pins.sh
#   NEURIPLO_TASKS_SRC=/path/to/neuriplo-tasks bash scripts/check_dependency_pins.sh
#
# Exit status is 0 only when every non-skipped check passes.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
VERSIONS_ENV="${ROOT_DIR}/versions.env"

PASS=0
FAIL=0
SKIP=0

pass() { PASS=$((PASS + 1)); printf '  ok   %s\n' "$1"; }
fail() { FAIL=$((FAIL + 1)); printf '  FAIL %s\n' "$1"; }
skip() { SKIP=$((SKIP + 1)); printf '  skip %s\n' "$1"; }
section() { printf '\n== %s ==\n' "$1"; }

# ---------------------------------------------------------------------------
# versions.env: format and required keys
# ---------------------------------------------------------------------------
section "versions.env"

if [[ ! -f "${VERSIONS_ENV}" ]]; then
    fail "versions.env exists at repository root"
    printf '\nRESULT: FAIL (%d failed)\n' "${FAIL}"
    exit 1
fi
pass "versions.env exists"

# Parse into a map without evaluating the file. Malformed content is rejected
# before any value is consumed.
declare -A PINS
FORMAT_OK=1
while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ -z "${line}" || "${line}" == \#* ]] && continue
    if [[ ! "${line}" =~ ^([A-Za-z_][A-Za-z0-9_]*)=([^[:space:]]+)$ ]]; then
        fail "line is not plain KEY=VALUE: ${line}"
        FORMAT_OK=0
        continue
    fi
    key="${BASH_REMATCH[1]}"
    value="${BASH_REMATCH[2]}"
    case "${value}" in
        *'$'* | *'`'* | *'"'* | *"'"*)
            fail "value uses shell syntax/quoting: ${line}"
            FORMAT_OK=0
            continue
            ;;
    esac
    PINS["${key}"]="${value}"
done <"${VERSIONS_ENV}"

if [[ "${FORMAT_OK}" -ne 1 ]]; then
    printf '\nRESULT: FAIL (%d failed; refusing to consume malformed versions.env)\n' "${FAIL}"
    exit 1
fi
pass "plain KEY=VALUE, no shell evaluation"

# Expose validated values as shell variables (non-evaluating; every key and
# value was checked above).
for key in "${!PINS[@]}"; do
    printf -v "${key}" '%s' "${PINS[${key}]}"
done

REQUIRED_KEYS=(
    NEURIPLO_TASKS_REPO NEURIPLO_TASKS_VERSION
    CMAKE_MIN_VERSION CUDA_MIN_VERSION TENSORRT_MIN_VERSION
    GSTREAMER_VERSION OPENCV_MIN_VERSION
    UBUNTU_VERSION JETPACK_VERSION L4T_VERSION
    DEEPSTREAM_INSTALL_ROOT DEEPSTREAM_BASELINE_VERSION DEEPSTREAM_FALLBACK_VERSION
    NGC_IMAGE_X86_DS9_1 NGC_IMAGE_X86_DS9_0 NGC_IMAGE_X86_DS8_0
    NGC_IMAGE_JETSON_DS9_1 NGC_IMAGE_JETSON_DS9_0
    NGC_IMAGE_JETSON_DS7_1 NGC_IMAGE_JETSON_DS7_0
    CI_CUDA_IMAGE
)

for key in "${REQUIRED_KEYS[@]}"; do
    if [[ -n "${PINS[${key}]:-}" ]]; then
        pass "pins ${key}"
    else
        fail "missing required key ${key}"
    fi
done

if [[ "${NEURIPLO_TASKS_VERSION:-}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    pass "neuriplo-tasks pinned to release tag ${NEURIPLO_TASKS_VERSION}"
else
    fail "neuriplo-tasks version '${NEURIPLO_TASKS_VERSION:-unset}' is not a release tag (vX.Y.Z)"
fi

# ---------------------------------------------------------------------------
# CMake wiring
# ---------------------------------------------------------------------------
section "CMake"

cmake_file="${ROOT_DIR}/CMakeLists.txt"
module_file="${ROOT_DIR}/cmake/LoadDependencyVersions.cmake"

if [[ -f "${module_file}" ]]; then
    pass "cmake/LoadDependencyVersions.cmake exists"
else
    fail "cmake/LoadDependencyVersions.cmake is missing"
fi
if grep -q 'LoadDependencyVersions' "${cmake_file}"; then
    pass "CMakeLists.txt includes the versions parser module"
else
    fail "CMakeLists.txt does not include cmake/LoadDependencyVersions.cmake"
fi
if grep -q 'NEURIPLO_TASKS_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt uses NEURIPLO_TASKS_VERSION"
else
    fail "CMakeLists.txt does not use NEURIPLO_TASKS_VERSION"
fi
if grep -q 'NEURIPLO_TASKS_REPO' "${cmake_file}"; then
    pass "CMakeLists.txt uses NEURIPLO_TASKS_REPO"
else
    fail "CMakeLists.txt does not use NEURIPLO_TASKS_REPO"
fi
if grep -qE 'GIT_TAG[[:space:]]+master' "${cmake_file}"; then
    fail "CMakeLists.txt still pins GIT_TAG master"
else
    pass "CMakeLists.txt has no floating GIT_TAG master"
fi
if grep -E 'GIT_TAG' "${cmake_file}" | grep -qv 'NEURIPLO_TASKS_VERSION'; then
    fail "CMakeLists.txt has a GIT_TAG that is not NEURIPLO_TASKS_VERSION"
else
    pass "every GIT_TAG resolves from NEURIPLO_TASKS_VERSION"
fi
if grep -q 'GIT_SHALLOW' "${cmake_file}"; then
    pass "CMakeLists.txt fetches the pinned ref shallowly"
else
    fail "CMakeLists.txt does not set GIT_SHALLOW"
fi
if grep -q 'DEEPSTREAM_INSTALL_ROOT' "${cmake_file}"; then
    pass "CMakeLists.txt derives DeepStream paths from DEEPSTREAM_INSTALL_ROOT"
else
    fail "CMakeLists.txt does not use DEEPSTREAM_INSTALL_ROOT"
fi
if grep -q 'CMAKE_VERSION VERSION_LESS CMAKE_MIN_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt enforces CMAKE_MIN_VERSION"
else
    fail "CMakeLists.txt does not enforce CMAKE_MIN_VERSION"
fi
if grep -q 'CUDA_VERSION VERSION_LESS CUDA_MIN_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt enforces CUDA_MIN_VERSION"
else
    fail "CMakeLists.txt does not enforce CUDA_MIN_VERSION"
fi
if grep -q 'OPENCV_MIN_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt enforces OPENCV_MIN_VERSION"
else
    fail "CMakeLists.txt does not enforce OPENCV_MIN_VERSION"
fi
if grep -q 'GSTREAMER_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt enforces GSTREAMER_VERSION"
else
    fail "CMakeLists.txt does not enforce GSTREAMER_VERSION"
fi
if grep -q 'DEEPINFER_ENFORCE_TOOLCHAIN' "${cmake_file}"; then
    pass "CMakeLists.txt gates toolchain enforcement behind DEEPINFER_ENFORCE_TOOLCHAIN"
else
    fail "CMakeLists.txt does not declare DEEPINFER_ENFORCE_TOOLCHAIN"
fi
if grep -q 'add_definitions' "${cmake_file}"; then
    fail "CMakeLists.txt still uses global add_definitions"
else
    pass "CMakeLists.txt has no global add_definitions"
fi
if grep -q 'target_compile_definitions' "${cmake_file}"; then
    pass "CMakeLists.txt uses target_compile_definitions"
else
    fail "CMakeLists.txt does not use target_compile_definitions"
fi
if grep -q 'CMAKE_MINIMUM_REQUIRED_VERSION' "${cmake_file}"; then
    pass "CMakeLists.txt reconciles CMAKE_MIN_VERSION via CMAKE_MINIMUM_REQUIRED_VERSION"
else
    fail "CMakeLists.txt does not reconcile CMAKE_MIN_VERSION"
fi

if command -v cmake >/dev/null 2>&1; then
    probe="$(mktemp)"
    cat >"${probe}" <<'CMAKE'
include("${DEEP_INFER_VERSIONS_MODULE}")
if(NOT NEURIPLO_TASKS_VERSION STREQUAL "${EXPECTED}")
  message(FATAL_ERROR "parse mismatch: got '${NEURIPLO_TASKS_VERSION}', want '${EXPECTED}'")
endif()
message(STATUS "versions parse ok: ${NEURIPLO_TASKS_VERSION}")
CMAKE

    run_probe() { cmake "$@" -P "${probe}" >/dev/null 2>&1; }

    if run_probe -DDEEP_INFER_VERSIONS_MODULE="${module_file}" \
        -DVERSIONS_ENV_FILE="${VERSIONS_ENV}" -DEXPECTED="${NEURIPLO_TASKS_VERSION}"; then
        pass "parser resolves the pinned version from versions.env"
    else
        fail "parser could not resolve the pinned version"
    fi

    bad_env="$(mktemp)"
    { cat "${VERSIONS_ENV}"; printf 'this line is malformed\n'; } >"${bad_env}"
    if run_probe -DDEEP_INFER_VERSIONS_MODULE="${module_file}" \
        -DVERSIONS_ENV_FILE="${bad_env}" -DEXPECTED="${NEURIPLO_TASKS_VERSION}"; then
        fail "parser accepted a malformed versions.env line"
    else
        pass "parser rejects a malformed versions.env line"
    fi
    rm -f "${bad_env}"

    if run_probe -DDEEP_INFER_VERSIONS_MODULE="${module_file}" \
        -DVERSIONS_ENV_FILE="${VERSIONS_ENV}" \
        -DNEURIPLO_TASKS_VERSION=v9.9.9 -DEXPECTED=v9.9.9; then
        pass "cache (-D) value overrides versions.env"
    else
        fail "cache (-D) value did not override versions.env"
    fi

    if NEURIPLO_TASKS_VERSION=v8.8.8 run_probe \
        -DDEEP_INFER_VERSIONS_MODULE="${module_file}" \
        -DVERSIONS_ENV_FILE="${VERSIONS_ENV}" -DEXPECTED=v8.8.8; then
        pass "environment value overrides versions.env"
    else
        fail "environment value did not override versions.env"
    fi

    rm -f "${probe}"
else
    skip "CMake parser probes (cmake not found)"
fi

# ---------------------------------------------------------------------------
# Shell consumers
# ---------------------------------------------------------------------------
section "shell"

common_sh="${ROOT_DIR}/scripts/docker/common.sh"
if grep -q 'versions.env' "${common_sh}"; then
    pass "common.sh reads versions.env"
else
    fail "common.sh does not read versions.env"
fi
if grep -qE '^[[:space:]]*(source|\.)[[:space:]]+.*versions\.env' "${common_sh}"; then
    fail "common.sh sources versions.env (must parse it non-evaluatingly)"
else
    pass "common.sh does not source versions.env"
fi
if grep -q 'printf -v' "${common_sh}"; then
    pass "common.sh uses a non-evaluating loader"
else
    fail "common.sh does not use a non-evaluating loader"
fi
if grep -q 'nvcr.io/nvidia/deepstream:' "${common_sh}"; then
    fail "common.sh still hardcodes an NGC DeepStream image tag"
else
    pass "common.sh has no hardcoded NGC DeepStream image tag"
fi
if grep -qE 'DEEPSTREAM_VERSION="\$\{DEEPSTREAM_VERSION:-[0-9]' "${common_sh}"; then
    fail "common.sh duplicates a DeepStream version literal instead of deriving it from the profile"
else
    pass "common.sh derives DEEPSTREAM_VERSION without per-profile literals"
fi

self_file="${ROOT_DIR}/scripts/check_dependency_pins.sh"
if grep -qE '^[[:space:]]*(source|\.)[[:space:]]+.*versions\.env' "${self_file}"; then
    fail "checker sources versions.env (must parse it non-evaluatingly)"
else
    pass "checker does not source versions.env"
fi

while IFS= read -r -d '' script; do
    rel="${script#"${ROOT_DIR}/"}"
    if bash -n "${script}" 2>/dev/null; then
        pass "bash -n ${rel}"
    else
        fail "bash -n ${rel}"
    fi
done < <(find "${ROOT_DIR}/scripts" -name '*.sh' -print0)

# ---------------------------------------------------------------------------
# Container / CI consumers
# ---------------------------------------------------------------------------
section "containers and CI"

baseline_dir="${DEEPSTREAM_INSTALL_ROOT}/deepstream-${DEEPSTREAM_BASELINE_VERSION}"

check_contains() {
    local file="$1" needle="$2" label="$3"
    if [[ -f "${file}" ]] && grep -qF -- "${needle}" "${file}"; then
        pass "${label}"
    else
        fail "${label} (missing '${needle}' in ${file#"${ROOT_DIR}/"})"
    fi
}

check_contains "${ROOT_DIR}/.devcontainer/Dockerfile" \
    "ARG DEEPSTREAM_VERSION=${DEEPSTREAM_BASELINE_VERSION}" \
    "devcontainer Dockerfile uses baseline ${DEEPSTREAM_BASELINE_VERSION}"
check_contains "${ROOT_DIR}/.devcontainer/devcontainer.json" \
    "\"DEEPSTREAM_VERSION\": \"${DEEPSTREAM_BASELINE_VERSION}\"" \
    "devcontainer.json uses baseline ${DEEPSTREAM_BASELINE_VERSION}"
check_contains "${ROOT_DIR}/.devcontainer/devcontainer.json" \
    "${baseline_dir}" \
    "devcontainer.json points at ${baseline_dir}"
check_contains "${ROOT_DIR}/.github/workflows/ci.yml" \
    "${CI_CUDA_IMAGE}" \
    "ci.yml uses CI_CUDA_IMAGE"
check_contains "${ROOT_DIR}/.github/workflows/ci.yml" \
    "DEEPINFER_ENFORCE_TOOLCHAIN=OFF" \
    "ci.yml opts the stub build out of toolchain enforcement"
check_contains "${ROOT_DIR}/Dockerfile" \
    "ARG DEEPSTREAM_VERSION=${DEEPSTREAM_BASELINE_VERSION}" \
    "Dockerfile defaults to baseline ${DEEPSTREAM_BASELINE_VERSION}"
check_contains "${ROOT_DIR}/Dockerfile" \
    "ARG DEEPSTREAM_DIR=${baseline_dir}" \
    "Dockerfile defaults to ${baseline_dir}"

# ---------------------------------------------------------------------------
# Documentation
# ---------------------------------------------------------------------------
section "documentation"

readme="${ROOT_DIR}/README.md"
check_contains "${readme}" "versions.env" "README documents versions.env"
check_contains "${readme}" "${NEURIPLO_TASKS_VERSION}" "README states pinned ${NEURIPLO_TASKS_VERSION}"
check_contains "${readme}" "FETCHCONTENT_SOURCE_DIR_NEURIPLO-TASKS" \
    "README documents the offline FetchContent source path"
check_contains "${readme}" "-D" "README documents the -D override precedence"
check_contains "${readme}" "DEEPINFER_ENFORCE_TOOLCHAIN" \
    "README documents DEEPINFER_ENFORCE_TOOLCHAIN"

# ---------------------------------------------------------------------------
# API conformance compile
# ---------------------------------------------------------------------------
section "neuriplo-tasks API conformance"

checkout_version() {
    local dir="$1"
    [[ -f "${dir}/VERSION" ]] || return 1
    printf 'v%s' "$(tr -d '[:space:]' <"${dir}/VERSION")"
}

NEURIPLO_SRC=""
if [[ -n "${NEURIPLO_TASKS_SRC:-}" ]]; then
    supplied_version="$(checkout_version "${NEURIPLO_TASKS_SRC}" || true)"
    if [[ ! -f "${NEURIPLO_TASKS_SRC}/include/neuriplo/tasks/core/result_types.hpp" ]]; then
        fail "NEURIPLO_TASKS_SRC has no neuriplo-tasks headers: ${NEURIPLO_TASKS_SRC}"
    elif [[ "${supplied_version}" != "${NEURIPLO_TASKS_VERSION}" ]]; then
        fail "NEURIPLO_TASKS_SRC version '${supplied_version:-unknown}' != pinned ${NEURIPLO_TASKS_VERSION}"
    else
        NEURIPLO_SRC="${NEURIPLO_TASKS_SRC}"
    fi
else
    fetch_src="${ROOT_DIR}/build/_deps/neuriplo-tasks-src"
    fetch_version="$(checkout_version "${fetch_src}" || true)"
    if [[ "${fetch_version}" == "${NEURIPLO_TASKS_VERSION}" ]]; then
        NEURIPLO_SRC="${fetch_src}"
    elif [[ -n "${fetch_version}" ]]; then
        skip "build/_deps checkout is ${fetch_version}, not ${NEURIPLO_TASKS_VERSION}"
    fi
fi

if ! command -v g++ >/dev/null 2>&1; then
    skip "API conformance compile (g++ not found)"
elif [[ -z "${NEURIPLO_SRC}" ]]; then
    skip "API conformance compile (set NEURIPLO_TASKS_SRC to a ${NEURIPLO_TASKS_VERSION} checkout)"
else
    tmp_dir="$(mktemp -d)"
    trap 'rm -rf "${tmp_dir}"' EXIT
    cat >"${tmp_dir}/conformance.cpp" <<'CPP'
// Mirrors the neuriplo-tasks API surface used by
// src/deepstream/TensorPostprocess.cpp and include/common.hpp.
#include <neuriplo/tasks/core/result_types.hpp>
#include <neuriplo/tasks/core/tensor_utils.hpp>
#include <neuriplo/tasks/instance_segmentation/rfdetr_segmentation_postprocessor.hpp>
#include <neuriplo/tasks/pose_estimation/yolo_pose_postprocessor.hpp>

#include <string>
#include <vector>

int main() {
    using namespace neuriplo_tasks;

    const vision::Size input_size(640, 640);
    const vision::Size frame_size(1920, 1080);

    std::vector<Tensor> tensors;
    tensors.emplace_back(std::vector<TensorElement>{1.0f}, std::vector<int64_t>{1, 300, 57});
    tensors.emplace_back(std::vector<TensorElement>{0.5f}, std::vector<int64_t>{1, 3, 160, 160});

    YoloPosePostprocessor pose_pp(input_size, 0.5f, 0.4f);
    std::vector<PoseEstimation> poses = pose_pp.postprocess(tensors, frame_size, input_size);
    (void)poses;

    PoseEstimation pose;
    pose.score = 0.5f;
    pose.bbox = vision::Rect(1, 2, 3, 4);
    Keypoint kp;
    kp.x = 1.0f;
    kp.y = 2.0f;
    kp.confidence = 0.9f;
    pose.keypoints.push_back(kp);

    std::vector<std::string> output_names{"dets", "labels", "masks"};
    RfDetrSegmentationPostprocessor seg_pp(input_size, 0.5f, 0.5f, output_names);
    std::vector<InstanceSegmentation> segs = seg_pp.postprocess(tensors, frame_size);
    (void)segs;

    InstanceSegmentation seg;
    seg.mask_height = 1;
    seg.mask_width = 1;
    Detection det;
    det.bbox = vision::Rect(0, 0, 1, 1);
    Result result = det;
    (void)result;

    (void)tensorElementToFloat(tensors[0].data[0]);
    return 0;
}
CPP
    if g++ -std=c++17 -fsyntax-only -I"${NEURIPLO_SRC}/include" \
        "${tmp_dir}/conformance.cpp" 2>"${tmp_dir}/err"; then
        pass "API conformance compiles against ${NEURIPLO_SRC}"
    else
        fail "API conformance compile failed"
        sed -n '1,40p' "${tmp_dir}/err"
    fi
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
printf '\nRESULT: '
if [[ "${FAIL}" -eq 0 ]]; then
    printf 'PASS (%d ok, %d skipped)\n' "${PASS}" "${SKIP}"
    exit 0
fi
printf 'FAIL (%d failed, %d ok, %d skipped)\n' "${FAIL}" "${PASS}" "${SKIP}"
exit 1
