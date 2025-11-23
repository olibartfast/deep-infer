#ifndef DEEPSTREAM_INFER_LAB_INCLUDE_CONFIG_HPP_
#define DEEPSTREAM_INFER_LAB_INCLUDE_CONFIG_HPP_

#include <string>
#include <vector>
#include "common.hpp"

struct Config {
    // DeepStream configuration
    std::string config_file;  // DeepStream config file path
    std::string model_engine;  // TensorRT engine file
    std::string onnx_file;     // ONNX model file
    bool verbose = false;
    
    // Model configuration
    std::string model_name;
    std::string model_type;
    int batch_size = 1;
    
    // Input configuration
    std::string source;  // Video file, RTSP stream, or image
    int input_width = 640;
    int input_height = 640;
    
    // Output configuration
    std::string output_path;
    std::string labels_file;
    bool show_frame = false;
    bool write_frame = true;
    
    // Processing configuration
    float confidence_threshold = 0.5f;
    float nms_threshold = 0.4f;
    int gpu_id = 0;
    
    // DeepStream pipeline configuration
    bool use_tracker = false;
    std::string tracker_config;
    bool use_analytics = false;
    
    // Performance configuration
    int num_decode_surfaces = 16;
    int num_extra_surfaces = 1;
    bool enable_perf_measurement = false;
    
    // Logging configuration
    std::string log_level = "info";
    std::string log_file = "";
    
    // Validation
    bool IsValid() const {
        if (source.empty()) {
            return false;
        }
        if (model_type.empty()) {
            return false;
        }
        if (config_file.empty() && model_engine.empty() && onnx_file.empty()) {
            return false;
        }
        return true;
    }
    
    std::string GetValidationError() const {
        if (source.empty()) return "Source is required";
        if (model_type.empty()) return "Model type is required";
        if (config_file.empty() && model_engine.empty() && onnx_file.empty()) {
            return "Either config_file, model_engine, or onnx_file is required";
        }
        return "";
    }
};

#endif  // DEEPSTREAM_INFER_LAB_INCLUDE_CONFIG_HPP_
