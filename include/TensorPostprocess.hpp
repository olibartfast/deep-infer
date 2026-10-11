#ifndef DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_
#define DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_

#include "common.hpp"

#include <neuriplo/tasks/core/tensor_utils.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace deepinfer {

struct FrameSize {
    int width{0};
    int height{0};
};

// Run the neuriplo-tasks postprocessor that matches `model_type` against the
// raw nvinfer output tensors attached to a frame.
//
// Supported model_type values:
//   - "yolo_pose" / "yolo26_pose" / "yolov8_pose"  -> PoseEstimation results
//   - "rfdetr_segmentation"                        -> InstanceSegmentation results
//   - everything else                              -> empty (DS object-meta path is used)
//
// Returns a vector of neuriplo_tasks::Result variants ready for the AppResult.
std::vector<neuriplo_tasks::Result> PostprocessTensorMeta(
    NvDsInferTensorMeta* tensor_meta,
    const std::string& model_type,
    const FrameSize& input_size,
    const FrameSize& frame_size,
    float confidence_threshold,
    float nms_threshold);

bool IsPoseModel(const std::string& model_type);
bool IsRfDetrSegmentation(const std::string& model_type);
float HalfToFloat(uint16_t h);
std::vector<neuriplo_tasks::PoseEstimation> DecodeYoloPoseTopN(
    const neuriplo_tasks::Tensor& t,
    const neuriplo_tasks::vision::Size& input_size,
    const neuriplo_tasks::vision::Size& frame_size,
    float confidence_threshold);
std::vector<neuriplo_tasks::Result> PostprocessTensors(
    const std::vector<neuriplo_tasks::Tensor>& tensors,
    const std::vector<std::string>& output_names,
    const std::string& model_type,
    const FrameSize& input_size,
    const FrameSize& frame_size,
    float confidence_threshold,
    float nms_threshold);

}  // namespace deepinfer

#endif  // DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_
