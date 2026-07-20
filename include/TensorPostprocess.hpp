#ifndef DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_
#define DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_

#include "common.hpp"

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

}  // namespace deepinfer

#endif  // DEEP_INFER_INCLUDE_TENSOR_POSTPROCESS_HPP_
