cmake_minimum_required(VERSION 3.20)

project(deepstream-infer-lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(WITH_SHOW_FRAME "Enable frame display" OFF)
option(WITH_WRITE_FRAME "Enable frame writing to disk" ON)

if(WITH_SHOW_FRAME)
    add_definitions(-DSHOW_FRAME)
    message(STATUS "Show frame enabled")
endif()

if(WITH_WRITE_FRAME)
    add_definitions(-DWRITE_FRAME)
    message(STATUS "Write frame enabled")
endif()

# Find required packages
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)
find_package(PkgConfig REQUIRED)

# Find GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Fetch vision-core
include(FetchContent)
FetchContent_Declare(
    vision-core
    GIT_REPOSITORY https://github.com/olibartfast/vision-core.git
    GIT_TAG main
)
FetchContent_MakeAvailable(vision-core)

# DeepStream paths (adjust based on your installation)
set(DEEPSTREAM_DIR "/opt/nvidia/deepstream/deepstream" CACHE PATH "DeepStream SDK directory")
set(DEEPSTREAM_LIBS_DIR "${DEEPSTREAM_DIR}/lib")
set(DEEPSTREAM_INCLUDE_DIR "${DEEPSTREAM_DIR}/sources/includes")

# Include directories
include_directories(
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
    ${DEEPSTREAM_INCLUDE_DIR}
)

# Source files
set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/main/main.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/utils.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/ConfigManager.cpp
    ${PROJECT_SOURCE_DIR}/src/deepstream/DeepStreamPipeline.cpp
)

# Detector sources
set(DETECTOR_SOURCES
    ${PROJECT_SOURCE_DIR}/src/tasks/object_detection/src/YOLO.cpp
)

# Classifier sources
set(CLASSIFIER_SOURCES
    ${PROJECT_SOURCE_DIR}/src/tasks/classification/src/Classifier.cpp
)

# Instance segmentation sources
set(INSTANCE_SEGMENTATION_SOURCES
    ${PROJECT_SOURCE_DIR}/src/tasks/instance_segmentation/src/YOLOSeg.cpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${DETECTOR_SOURCES}
    ${CLASSIFIER_SOURCES}
    ${INSTANCE_SEGMENTATION_SOURCES}
)

# Link directories
link_directories(
    ${DEEPSTREAM_LIBS_DIR}
    ${CUDA_LIBRARY_DIRS}
)

# Link libraries
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

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Testing
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
