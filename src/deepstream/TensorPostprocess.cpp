#include "TensorPostprocess.hpp"
#include "Logger.hpp"

#if HAVE_DEEPSTREAM

#include <neuriplo/tasks/core/tensor_utils.hpp>
#include <neuriplo/tasks/core/result_types.hpp>
#include <neuriplo/tasks/pose_estimation/yolo_pose_postprocessor.hpp>
#include <neuriplo/tasks/instance_segmentation/rfdetr_segmentation_postprocessor.hpp>

#include <cstring>
#include <cmath>
#include <algorithm>

namespace {
using neuriplo_tasks::Tensor;
using neuriplo_tasks::TensorElement;

// Correct IEEE 754 half (FP16) -> float conversion for host code.
float HalfToFloat(uint16_t h) {
    union { float f; uint32_t u; } out;
    const uint32_t t = (uint32_t)(h & 0x8000u) << 16;  // sign bit
    const uint32_t e = (h >> 10) & 0x1Fu;
    const uint32_t m = h & 0x3FFu;
    switch (e) {
        case 0u: {
            if (m == 0u) {
                out.u = t;
            } else {  // subnormal
                int e2 = 0;
                uint32_t m2 = m;
                do { m2 <<= 1; --e2; } while (!(m2 & 0x400u));
                m2 &= 0x3FFu;
                e2 += 127 - 15 + 1;
                out.u = t | ((uint32_t)e2 << 23) | (m2 << 13);
            }
            break;
        }
        case 31u:  // inf / nan
            out.u = t | 0x7F800000u | (m << 13);
            break;
        default:
            out.u = t | ((e + 112u) << 23) | (m << 13);
            break;
    }
    return out.f;
}

// Byte size of one element for a given NvDsInferDataType.
std::size_t ElementSize(NvDsInferDataType dt) {
    switch (dt) {
        case FLOAT: return 4;   // FP32
        case HALF:  return 2;   // FP16
        case INT8:  return 1;
        case INT32: return 4;
        case INT64: return 8;
        case UINT8: return 1;
        default:    return 0;
    }
}

// Build a neuriplo_tasks::Tensor from one nvinfer output layer's host buffer.
// The NvDsInferLayerInfo buffer pointers are invalid inside output_layers_info,
// so the caller passes the matching host buffer from out_buf_ptrs_host[].
bool BuildTensor(const NvDsInferLayerInfo& info, const void* host_ptr, Tensor& out) {
    const NvDsInferDims& dims = info.inferDims;
    const std::size_t count = static_cast<std::size_t>(dims.numElements);
    if (count == 0 || host_ptr == nullptr) {
        return false;
    }

    const std::size_t elem = ElementSize(info.dataType);
    if (elem == 0) {
        return false;
    }

    out.shape.clear();
    out.shape.reserve(static_cast<std::size_t>(dims.numDims) + 1);
    // nvinfer's NvDsInferDims excludes the batch dimension; re-add it
    // (batch-size=1) so downstream postprocessors see the full rank they
    // expect (e.g. [batch, channels, anchors] for YOLO outputs).
    out.shape.push_back(static_cast<int64_t>(1));
    for (unsigned int i = 0; i < dims.numDims; ++i) {
        out.shape.push_back(static_cast<int64_t>(dims.d[i]));
    }

    out.data.clear();
    out.data.reserve(count);

    switch (info.dataType) {
        case FLOAT: {  // FP32
            const auto* src = static_cast<const float*>(host_ptr);
            for (std::size_t i = 0; i < count; ++i) {
                out.data.emplace_back(src[i]);
            }
            break;
        }
        case HALF: {  // FP16 -> float
            const auto* src = static_cast<const uint16_t*>(host_ptr);
            for (std::size_t i = 0; i < count; ++i) {
                out.data.emplace_back(HalfToFloat(src[i]));
            }
            break;
        }
        case INT32: {
            const auto* src = static_cast<const int32_t*>(host_ptr);
            for (std::size_t i = 0; i < count; ++i) {
                out.data.emplace_back(src[i]);
            }
            break;
        }
        case INT64: {
            const auto* src = static_cast<const int64_t*>(host_ptr);
            for (std::size_t i = 0; i < count; ++i) {
                out.data.emplace_back(src[i]);
            }
            break;
        }
        default: {  // INT8 / UINT8
            const auto* src = static_cast<const uint8_t*>(host_ptr);
            for (std::size_t i = 0; i < count; ++i) {
                out.data.emplace_back(src[i]);
            }
            break;
        }
    }
    return true;
}

bool IsPoseModel(const std::string& model_type) {
    std::string t;
    t.reserve(model_type.size());
    for (char c : model_type) {
        t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return t.find("pose") != std::string::npos && t.find("yolo") != std::string::npos;
}

bool IsRfDetrSegmentation(const std::string& model_type) {
    std::string t;
    t.reserve(model_type.size());
    for (char c : model_type) {
        t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return t.find("rfdetr") != std::string::npos && t.find("seg") != std::string::npos;
}

// Decode the modern Ultralytics decoded "top-N" pose output.
// Shape [batch, N, C] with C == 56 (no class slot) or 57 (with class index),
// where each row is [cx, cy, w, h, conf, (class), kx, ky, kc, ...] in the
// letterboxed network-input pixel space. N is small (e.g. 300 = max_det) and
// the detections are already post-NMS, so only confidence filtering is applied.
std::vector<neuriplo_tasks::PoseEstimation> DecodeYoloPoseTopN(
    const neuriplo_tasks::Tensor& t,
    const neuriplo_tasks::vision::Size& input_size,
    const neuriplo_tasks::vision::Size& frame_size,
    float confidence_threshold) {
    std::vector<neuriplo_tasks::PoseEstimation> poses;
    if (t.shape.size() != 3) {
        return poses;
    }
    const int batch = static_cast<int>(t.shape[0]);
    const int n = static_cast<int>(t.shape[1]);
    const int c = static_cast<int>(t.shape[2]);
    if (n <= 0 || (c != 56 && c != 57) ||
        t.data.size() < static_cast<std::size_t>(batch) * n * c) {
        return poses;
    }

    const int kpts_start = (c >= 57) ? 6 : 5;
    const int num_kpts = (c - kpts_start) / 3;

    // Letterbox (maintain-aspect-ratio, symmetric padding) inverse map from the
    // network input space back to the original frame dimensions.
    const float fw = static_cast<float>(frame_size.width);
    const float fh = static_cast<float>(frame_size.height);
    const float scale = std::min(static_cast<float>(input_size.width) / fw,
                                 static_cast<float>(input_size.height) / fh);
    if (scale <= 0.0f) {
        return poses;
    }
    const float pad_x = (static_cast<float>(input_size.width) - fw * scale) * 0.5f;
    const float pad_y = (static_cast<float>(input_size.height) - fh * scale) * 0.5f;
    const auto x_orig = [&](float x) { return (x - pad_x) / scale; };
    const auto y_orig = [&](float y) { return (y - pad_y) / scale; };

    for (int b = 0; b < batch; ++b) {
        const std::size_t base = static_cast<std::size_t>(b) * n * c;
        for (int i = 0; i < n; ++i) {
            const std::size_t r = base + static_cast<std::size_t>(i) * c;
            const float conf = neuriplo_tasks::tensorElementToFloat(t.data[r + 4]);
            if (conf < confidence_threshold) {
                continue;
            }
            const float cx = neuriplo_tasks::tensorElementToFloat(t.data[r + 0]);
            const float cy = neuriplo_tasks::tensorElementToFloat(t.data[r + 1]);
            const float w = neuriplo_tasks::tensorElementToFloat(t.data[r + 2]) / scale;
            const float h = neuriplo_tasks::tensorElementToFloat(t.data[r + 3]) / scale;

            neuriplo_tasks::PoseEstimation pose;
            pose.score = conf;
            pose.bbox = neuriplo_tasks::vision::Rect(
                static_cast<int>(x_orig(cx) - w * 0.5f),
                static_cast<int>(y_orig(cy) - h * 0.5f),
                static_cast<int>(w),
                static_cast<int>(h));

            pose.keypoints.reserve(static_cast<std::size_t>(num_kpts));
            for (int k = 0; k < num_kpts; ++k) {
                const float kx = neuriplo_tasks::tensorElementToFloat(t.data[r + kpts_start + k * 3 + 0]);
                const float ky = neuriplo_tasks::tensorElementToFloat(t.data[r + kpts_start + k * 3 + 1]);
                const float kc = neuriplo_tasks::tensorElementToFloat(t.data[r + kpts_start + k * 3 + 2]);
                neuriplo_tasks::Keypoint kp;
                kp.x = x_orig(kx);
                kp.y = y_orig(ky);
                kp.confidence = kc;
                pose.keypoints.push_back(std::move(kp));
            }
            poses.push_back(std::move(pose));
        }
    }
    return poses;
}

}  // namespace

namespace deepinfer {

std::vector<neuriplo_tasks::Result> PostprocessTensorMeta(
    NvDsInferTensorMeta* tensor_meta,
    const std::string& model_type,
    const FrameSize& input_size,
    const FrameSize& frame_size,
    float confidence_threshold,
    float nms_threshold) {
    std::vector<neuriplo_tasks::Result> results;
    if (tensor_meta == nullptr) {
        return results;
    }

    auto& logger = Logger::GetInstance();

    // Collect every output layer as a neuriplo_tasks::Tensor.
    // NvDsInferTensorMeta exposes layers as an array (output_layers_info[])
    // with parallel host buffers in out_buf_ptrs_host[]. The buffer pointers
    // inside output_layers_info[] are NOT valid, so we always read host memory.
    std::vector<Tensor> tensors;
    std::vector<std::string> output_names;
    const guint num_layers = tensor_meta->num_output_layers;
    for (guint i = 0; i < num_layers; ++i) {
        const NvDsInferLayerInfo& info = tensor_meta->output_layers_info[i];
        if (info.isInput) {
            continue;
        }
        Tensor t;
        if (BuildTensor(info, tensor_meta->out_buf_ptrs_host[i], t)) {
            tensors.push_back(std::move(t));
            if (info.layerName != nullptr) {
                output_names.emplace_back(info.layerName);
            }
        } else {
            logger.Warnf("TensorPostprocess: skipped output layer %u (empty/invalid)", i);
        }
    }

    if (tensors.empty()) {
        return results;
    }

    // Diagnostic: log tensor shapes and the value range of the first tensor.
    {
        std::string shapes;
        for (std::size_t i = 0; i < tensors.size(); ++i) {
            shapes += "(";
            for (std::size_t d = 0; d < tensors[i].shape.size(); ++d) {
                shapes += std::to_string(tensors[i].shape[d]);
                if (d + 1 < tensors[i].shape.size()) shapes += "x";
            }
            shapes += ") ";
        }
        float lo = 1e9f, hi = -1e9f;
        for (const auto& e : tensors[0].data) {
            const float v = neuriplo_tasks::tensorElementToFloat(e);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
        logger.Debugf("TensorPostprocess: %zu tensor(s) shapes=%s t0 range=[%.4f, %.4f]",
                      tensors.size(), shapes.c_str(), lo, hi);
    }

    neuriplo_tasks::vision::Size in_sz(input_size.width, input_size.height);
    neuriplo_tasks::vision::Size frame_sz(frame_size.width, frame_size.height);

    if (IsPoseModel(model_type)) {
        const auto& t0 = tensors[0];
        const bool decoded_topn =
            (t0.shape.size() == 3) && (t0.shape[1] > 0 && t0.shape[1] <= 1000) &&
            (t0.shape[2] == 56 || t0.shape[2] == 57);
        std::vector<neuriplo_tasks::PoseEstimation> poses;
        if (decoded_topn) {
            poses = DecodeYoloPoseTopN(t0, in_sz, frame_sz, confidence_threshold);
        } else {
            neuriplo_tasks::YoloPosePostprocessor pp(in_sz, confidence_threshold, nms_threshold);
            poses = pp.postprocess(tensors, frame_sz, in_sz);
        }
        results.reserve(poses.size());
        for (auto& p : poses) {
            results.emplace_back(std::move(p));
        }
        logger.Debugf("TensorPostprocess: yolo_pose -> %zu poses (%s)",
                      poses.size(), decoded_topn ? "top-N decoded" : "anchor");
    } else if (IsRfDetrSegmentation(model_type)) {
        neuriplo_tasks::RfDetrSegmentationPostprocessor pp(in_sz, confidence_threshold,
                                                            /*mask_threshold=*/0.5f, output_names);
        auto segs = pp.postprocess(tensors, frame_sz);
        results.reserve(segs.size());
        for (auto& s : segs) {
            results.emplace_back(std::move(s));
        }
        logger.Debugf("TensorPostprocess: rfdetr_segmentation -> %zu instances", segs.size());
    }

    return results;
}

}  // namespace deepinfer

#else  // !HAVE_DEEPSTREAM

namespace deepinfer {

std::vector<neuriplo_tasks::Result> PostprocessTensorMeta(
    NvDsInferTensorMeta* /*tensor_meta*/,
    const std::string& /*model_type*/,
    const FrameSize& /*input_size*/,
    const FrameSize& /*frame_size*/,
    float /*confidence_threshold*/,
    float /*nms_threshold*/) {
    return {};
}

}  // namespace deepinfer

#endif  // HAVE_DEEPSTREAM
