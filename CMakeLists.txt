cmake_minimum_required(VERSION 3.19)

project(deepstream-infer-lab)

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
find_package(OpenCV REQUIRED)
find_package(CUDA   REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER       REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GLIB            REQUIRED glib-2.0)

# ---------------------------------------------------------------------------
# vision-core (fetched automatically)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    vision-core
    GIT_REPOSITORY https://github.com/olibartfast/vision-core.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(vision-core)

# ---------------------------------------------------------------------------
# DeepStream paths
# DeepStream 8.0 installs to a versioned directory:
#   /opt/nvidia/deepstream/deepstream-8.0
# Earlier versions used the unversioned path:
#   /opt/nvidia/deepstream/deepstream
#
# Detection order (first match wins):
#   1. -DDEEPSTREAM_DIR=<path>  passed at configure time
#   2. Versioned path  (DS 8.0+)
#   3. Unversioned path (DS 7.x and earlier)
# ---------------------------------------------------------------------------
if(NOT DEFINED DEEPSTREAM_DIR OR DEEPSTREAM_DIR STREQUAL "")
    if(EXISTS "/opt/nvidia/deepstream/deepstream-8.0")
        set(DEEPSTREAM_DIR "/opt/nvidia/deepstream/deepstream-8.0")
    elseif(EXISTS "/opt/nvidia/deepstream/deepstream")
        set(DEEPSTREAM_DIR "/opt/nvidia/deepstream/deepstream")
    else()
        message(FATAL_ERROR
            "DeepStream SDK not found. Install it or pass "
            "-DDEEPSTREAM_DIR=<path> to cmake.")
    endif()
endif()

set(DEEPSTREAM_DIR      "${DEEPSTREAM_DIR}" CACHE PATH "DeepStream SDK root directory")
set(DEEPSTREAM_LIBS_DIR "${DEEPSTREAM_DIR}/lib")
set(DEEPSTREAM_INC_DIR  "${DEEPSTREAM_DIR}/sources/includes")

if(NOT EXISTS "${DEEPSTREAM_INC_DIR}")
    message(FATAL_ERROR
        "DeepStream include directory not found: ${DEEPSTREAM_INC_DIR}\n"
        "Check that DEEPSTREAM_DIR (${DEEPSTREAM_DIR}) points to a valid SDK installation.")
endif()

message(STATUS "DeepStream SDK : ${DEEPSTREAM_DIR}")
message(STATUS "DeepStream libs: ${DEEPSTREAM_LIBS_DIR}")

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/main/main.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/utils.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/ConfigManager.cpp
    ${PROJECT_SOURCE_DIR}/src/deepstream/DeepStreamPipeline.cpp
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
        ${DEEPSTREAM_INC_DIR}
)

target_link_directories(${PROJECT_NAME}
    PRIVATE
        ${DEEPSTREAM_LIBS_DIR}
        /usr/local/cuda/targets/x86_64-linux/lib
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        vision-core::vision-core
        ${OpenCV_LIBS}
        ${CUDA_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${GLIB_LIBRARIES}
        nvdsgst_meta
        nvds_meta
        nvdsgst_helper
        nvdsgst_smartrecord
        nvds_utils
        nvds_msgbroker
        nvbufsurface
        nvbufsurftransform
        cuda
        cudart
        pthread
        dl
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
)

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