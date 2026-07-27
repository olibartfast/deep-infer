cmake_minimum_required(VERSION 3.19)

project(deep-infer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(WITH_SHOW_FRAME  "Enable frame display"           OFF)
option(WITH_WRITE_FRAME "Enable frame writing to disk"   ON)
option(BUILD_TESTING    "Build tests"                    OFF)

if(WITH_SHOW_FRAME)
    add_definitions(-DSHOW_FRAME)
    message(STATUS "Show frame: enabled")
endif()

if(WITH_WRITE_FRAME)
    add_definitions(-DWRITE_FRAME)
    message(STATUS "Write frame: enabled")
endif()

# ---------------------------------------------------------------------------
# Required packages
# ---------------------------------------------------------------------------
if((NOT DEFINED CUDA_TOOLKIT_ROOT_DIR OR CUDA_TOOLKIT_ROOT_DIR STREQUAL "")
   AND EXISTS "/usr/local/cuda")
    set(CUDA_TOOLKIT_ROOT_DIR "/usr/local/cuda" CACHE PATH
        "CUDA toolkit root directory" FORCE)
endif()

find_package(OpenCV REQUIRED)
find_package(CUDA   REQUIRED)
find_package(PkgConfig REQUIRED)

# OpenCV's libtiff depends on JBIG (jbg_* symbols). Some base images do not
# ship the shared libjbig.so.0 runtime, so link the static archive with
# --whole-archive to satisfy libtiff at both link and run time.
find_library(JBIG_STATIC_LIB
    NAMES libjbig.a
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu /usr/lib /usr/lib64)

pkg_check_modules(GSTREAMER       REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GLIB            REQUIRED glib-2.0)

# ---------------------------------------------------------------------------
# vision-core (fetched automatically)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    neuriplo-tasks
    GIT_REPOSITORY https://github.com/olibartfast/neuriplo-tasks.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(neuriplo-tasks)

# ---------------------------------------------------------------------------
# DeepStream paths
# ---------------------------------------------------------------------------
set(DEEPSTREAM_SEARCH_DIRS "")
if(DEFINED DEEPSTREAM_DIR AND NOT DEEPSTREAM_DIR STREQUAL "")
    list(APPEND DEEPSTREAM_SEARCH_DIRS "${DEEPSTREAM_DIR}")
endif()

list(APPEND DEEPSTREAM_SEARCH_DIRS
    "/opt/nvidia/deepstream/deepstream-9.1"
    "/opt/nvidia/deepstream/deepstream-9.0"
    "/opt/nvidia/deepstream/deepstream-8.0"
    "/opt/nvidia/deepstream/deepstream-7.1"
    "/opt/nvidia/deepstream/deepstream-7.0"
    "/opt/nvidia/deepstream/deepstream-6.4"
    "/opt/nvidia/deepstream/deepstream-6.3"
    "/opt/nvidia/deepstream/deepstream"
)
list(REMOVE_DUPLICATES DEEPSTREAM_SEARCH_DIRS)

set(DEEPSTREAM_FOUND OFF)
set(DEEPSTREAM_DIR "")
foreach(candidate_dir IN LISTS DEEPSTREAM_SEARCH_DIRS)
    if(EXISTS "${candidate_dir}/sources/includes/gstnvdsmeta.h")
        set(DEEPSTREAM_FOUND ON)
        set(DEEPSTREAM_DIR "${candidate_dir}")
        break()
    endif()
endforeach()

set(DEEPSTREAM_DIR "${DEEPSTREAM_DIR}" CACHE PATH "DeepStream SDK root directory")

if(DEEPSTREAM_FOUND)
    set(DEEPSTREAM_LIBS_DIR "${DEEPSTREAM_DIR}/lib")
    set(DEEPSTREAM_INC_DIR  "${DEEPSTREAM_DIR}/sources/includes")
    message(STATUS "DeepStream SDK : ${DEEPSTREAM_DIR}")
    message(STATUS "DeepStream libs: ${DEEPSTREAM_LIBS_DIR}")
else()
    message(WARNING
        "DeepStream SDK not found. Building without DeepStream runtime support. "
        "Install it or pass -DDEEPSTREAM_DIR=<path> and reconfigure to enable the full pipeline.")
endif()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/main/main.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/utils.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/ConfigManager.cpp
)

if(DEEPSTREAM_FOUND)
    list(APPEND SOURCES
        ${PROJECT_SOURCE_DIR}/src/deepstream/DeepStreamPipeline.cpp
    )
else()
    list(APPEND SOURCES
        ${PROJECT_SOURCE_DIR}/src/deepstream/DeepStreamPipelineStub.cpp
    )
endif()

list(APPEND SOURCES
    ${PROJECT_SOURCE_DIR}/src/deepstream/TensorPostprocess.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/utils
        ${PROJECT_SOURCE_DIR}/src/tasks
        ${PROJECT_SOURCE_DIR}/src/tasks/object_detection/include
        ${PROJECT_SOURCE_DIR}/src/tasks/classification/include
        ${PROJECT_SOURCE_DIR}/src/tasks/instance_segmentation/include
        ${PROJECT_SOURCE_DIR}/src/deepstream
        ${OpenCV_INCLUDE_DIRS}
        ${CUDA_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

if(DEEPSTREAM_FOUND)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${DEEPSTREAM_INC_DIR}
    )
endif()

set(CUDA_TARGET_LIB_DIRS "")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    # Jetson Orin uses SBSA-compatible ARM; fall back to generic aarch64 if missing
    if(EXISTS "${CUDA_TOOLKIT_ROOT_DIR}/targets/sbsa-linux/lib")
        list(APPEND CUDA_TARGET_LIB_DIRS "${CUDA_TOOLKIT_ROOT_DIR}/targets/sbsa-linux/lib")
    endif()
    if(EXISTS "${CUDA_TOOLKIT_ROOT_DIR}/targets/aarch64-linux/lib")
        list(APPEND CUDA_TARGET_LIB_DIRS "${CUDA_TOOLKIT_ROOT_DIR}/targets/aarch64-linux/lib")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    list(APPEND CUDA_TARGET_LIB_DIRS "${CUDA_TOOLKIT_ROOT_DIR}/targets/x86_64-linux/lib")
endif()
list(FILTER CUDA_TARGET_LIB_DIRS INCLUDE REGEX ".+")

target_link_directories(${PROJECT_NAME}
    PRIVATE
        ${CUDA_TARGET_LIB_DIRS}
)

if(DEEPSTREAM_FOUND)
    target_link_directories(${PROJECT_NAME}
        PRIVATE
            ${DEEPSTREAM_LIBS_DIR}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        neuriplo-tasks
        ${OpenCV_LIBS}
        ${CUDA_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${GLIB_LIBRARIES}
        cuda
        cudart
        pthread
        dl
)

if(JBIG_STATIC_LIB)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            -Wl,--whole-archive
            ${JBIG_STATIC_LIB}
            -Wl,--no-whole-archive
    )
endif()

if(DEEPSTREAM_FOUND)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            nvdsgst_meta
            nvds_meta
            nvdsgst_helper
            nvdsgst_smartrecord
            nvds_utils
            nvds_msgbroker
            nvbufsurface
            nvbufsurftransform
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HAVE_DEEPSTREAM=$<BOOL:${DEEPSTREAM_FOUND}>
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
)

# GNU ld does not always search the multiarch lib dir (e.g.
# /usr/lib/x86_64-linux-gnu) for the indirect DT_NEEDED dependencies of shared
# libraries such as OpenCV's libtiff -> libjbig. Point -rpath-link at the
# multiarch directory so these transitive shared deps resolve at link time.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-rpath-link,/usr/lib/aarch64-linux-gnu)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
