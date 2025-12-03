#ifndef DEEPSTREAM_INFER_LAB_INCLUDE_COMMON_HPP_
#define DEEPSTREAM_INFER_LAB_INCLUDE_COMMON_HPP_

#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <vector>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <stdexcept>

// DeepStream headers
#include <gst/gst.h>
#include <glib.h>
#include "gstnvdsmeta.h"
#include "nvds_analytics_meta.h"
#include "nvbufsurface.h"
#include "nvbufsurftransform.h"

// Vision Core
#include <vision-core/core/result_types.hpp>

// Common structures
struct AppResult {
    cv::Mat frame;
    std::vector<vision_core::Result> results;
};

enum class TaskType {
    ObjectDetection,
    Classification,
    InstanceSegmentation,
    OpticalFlow
};

// Custom exceptions
class DeepStreamError : public std::runtime_error {
public:
    explicit DeepStreamError(const std::string& message) 
        : std::runtime_error("DeepStream Error: " + message) {}
};

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message) 
        : std::runtime_error("Config Error: " + message) {}
};

class InputDimensionError : public std::runtime_error {
public:
    explicit InputDimensionError(const std::string& message) 
        : std::runtime_error("Input Dimension Error: " + message) {}
};

#endif  // DEEPSTREAM_INFER_LAB_INCLUDE_COMMON_HPP_
