# LoadDependencyVersions.cmake
#
# Parses versions.env (plain KEY=VALUE, the repository's single source of
# truth for third-party dependency pins) and defines the pin variables in the
# INCLUDING scope. This file must be included at top level (not called as a
# function/macro) so the variables propagate to the caller.
#
# Precedence per key (highest first):
#   1. Existing cache entry (left untouched, never overwritten)
#   2. Environment variable of the same name
#   3. File value (stored with a normal set(); NEVER written to the cache)
#
# Works both from a full CMake configure and from `cmake -P` script mode with
# -DVERSIONS_ENV_FILE=<path>.

if(NOT DEFINED VERSIONS_ENV_FILE OR VERSIONS_ENV_FILE STREQUAL "")
    set(VERSIONS_ENV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/versions.env")
endif()

if(NOT EXISTS "${VERSIONS_ENV_FILE}")
    message(FATAL_ERROR
        "Versions file not found: ${VERSIONS_ENV_FILE}. "
        "It pins the third-party dependency versions and must be present "
        "(supply it via -DVERSIONS_ENV_FILE=<path> or place versions.env in "
        "the source root).")
endif()

file(STRINGS "${VERSIONS_ENV_FILE}" _versions_lines)

foreach(_line IN LISTS _versions_lines)
    # Skip blank lines and comments (first non-space character is '#').
    if("${_line}" MATCHES "^[ \t]*#")
        continue()
    endif()
    if(_line STREQUAL "")
        continue()
    endif()

    if(NOT "${_line}" MATCHES "^[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*=[ \t]*(.*)$")
        message(FATAL_ERROR
            "Malformed line in ${VERSIONS_ENV_FILE}: \"${_line}\" "
            "(expected KEY=VALUE with no spaces in KEY).")
    endif()

    set(_key "${CMAKE_MATCH_1}")

    if(DEFINED CACHE{${_key}})
        # Existing cache value wins; leave it untouched.
        continue()
    elseif(DEFINED ENV{${_key}})
        set(${_key} "$ENV{${_key}}")
    else()
        string(STRIP "${CMAKE_MATCH_2}" _value)
        set(${_key} "${_value}")
    endif()
endforeach()

# --- Required pins -----------------------------------------------------------
set(_required_pins
    NEURIPLO_TASKS_REPO NEURIPLO_TASKS_VERSION
    CMAKE_MIN_VERSION CUDA_MIN_VERSION OPENCV_MIN_VERSION
    GSTREAMER_VERSION
    DEEPSTREAM_INSTALL_ROOT DEEPSTREAM_BASELINE_VERSION DEEPSTREAM_FALLBACK_VERSION)

foreach(_key IN LISTS _required_pins)
    if(NOT DEFINED ${_key} OR "${${_key}}" STREQUAL "")
        message(FATAL_ERROR
            "Required dependency pin '${_key}' is missing or empty in "
            "${VERSIONS_ENV_FILE}.")
    endif()
endforeach()

# --- Diagnostics -------------------------------------------------------------
message(STATUS "Dependency pins resolved from ${VERSIONS_ENV_FILE}:")
message(STATUS "  neuriplo-tasks: ${NEURIPLO_TASKS_REPO} (tag ${NEURIPLO_TASKS_VERSION})")
message(STATUS "  DeepStream: baseline ${DEEPSTREAM_BASELINE_VERSION}, "
    "fallback ${DEEPSTREAM_FALLBACK_VERSION} (root ${DEEPSTREAM_INSTALL_ROOT})")
message(STATUS "  Minimums: CMake>=${CMAKE_MIN_VERSION}, CUDA>=${CUDA_MIN_VERSION}, "
    "OpenCV>=${OPENCV_MIN_VERSION}, GStreamer>=${GSTREAMER_VERSION}")
