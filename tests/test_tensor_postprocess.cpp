#include "TensorPostprocess.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

neuriplo_tasks::Tensor MakeTopNTensor(float conf_row0, float conf_row1) {
    constexpr int kC = 57;
    neuriplo_tasks::Tensor t;
    t.shape = {1, 2, kC};
    t.data.reserve(2 * kC);
    for (int row = 0; row < 2; ++row) {
        const float conf = (row == 0) ? conf_row0 : conf_row1;
        std::vector<float> values(static_cast<std::size_t>(kC), 0.0f);
        values[0] = 320.0f;  // cx
        values[1] = 320.0f;  // cy
        values[2] = 100.0f;  // w
        values[3] = 100.0f;  // h
        values[4] = conf;
        values[5] = 0.0f;  // class index
        for (int k = 0; k < 17; ++k) {
            values[6 + k * 3 + 0] = 320.0f;
            values[6 + k * 3 + 1] = 320.0f;
            values[6 + k * 3 + 2] = 0.9f;
        }
        for (float v : values) {
            t.data.emplace_back(v);
        }
    }
    return t;
}

}  // namespace

TEST(IsPoseModelTest, RecognizesPoseVariants) {
    EXPECT_TRUE(deepinfer::IsPoseModel("yolo_pose"));
    EXPECT_TRUE(deepinfer::IsPoseModel("YOLO26_POSE"));
}

TEST(IsPoseModelTest, RejectsNonPose) {
    EXPECT_FALSE(deepinfer::IsPoseModel("yolov8"));
    EXPECT_FALSE(deepinfer::IsPoseModel("rfdetr_segmentation"));
    EXPECT_FALSE(deepinfer::IsPoseModel(""));
}

TEST(IsRfDetrSegmentationTest, RecognizesVariants) {
    EXPECT_TRUE(deepinfer::IsRfDetrSegmentation("rfdetr_segmentation"));
    EXPECT_TRUE(deepinfer::IsRfDetrSegmentation("RFDETR_SEG"));
}

TEST(IsRfDetrSegmentationTest, RejectsOthers) {
    EXPECT_FALSE(deepinfer::IsRfDetrSegmentation("yolov8"));
    EXPECT_FALSE(deepinfer::IsRfDetrSegmentation("yolo_pose"));
}

TEST(HalfToFloatTest, ConvertsKnownValues) {
    EXPECT_FLOAT_EQ(deepinfer::HalfToFloat(0x3C00), 1.0f);
    EXPECT_FLOAT_EQ(deepinfer::HalfToFloat(0xC000), -2.0f);
    EXPECT_FLOAT_EQ(deepinfer::HalfToFloat(0x0000), 0.0f);
    EXPECT_TRUE(std::isinf(deepinfer::HalfToFloat(0x7C00)));
    EXPECT_GT(deepinfer::HalfToFloat(0x7C00), 0.0f);
}

TEST(DecodeYoloPoseTopNTest, RejectsNonRank3) {
    neuriplo_tasks::Tensor t;
    t.shape = {2, 57};
    t.data.assign(2 * 57, neuriplo_tasks::TensorElement{0.0f});
    const neuriplo_tasks::vision::Size sz(640, 640);
    EXPECT_TRUE(deepinfer::DecodeYoloPoseTopN(t, sz, sz, 0.5f).empty());
}

TEST(DecodeYoloPoseTopNTest, RejectsBadLastDim) {
    neuriplo_tasks::Tensor t;
    t.shape = {1, 2, 10};
    t.data.assign(1 * 2 * 10, neuriplo_tasks::TensorElement{0.0f});
    const neuriplo_tasks::vision::Size sz(640, 640);
    EXPECT_TRUE(deepinfer::DecodeYoloPoseTopN(t, sz, sz, 0.5f).empty());
}

TEST(DecodeYoloPoseTopNTest, FiltersByConfidence) {
    const neuriplo_tasks::Tensor t = MakeTopNTensor(0.9f, 0.1f);
    const neuriplo_tasks::vision::Size sz(640, 640);
    const auto poses = deepinfer::DecodeYoloPoseTopN(t, sz, sz, 0.5f);
    ASSERT_EQ(poses.size(), 1u);
    EXPECT_EQ(poses[0].keypoints.size(), 17u);
}

TEST(PostprocessTensorsTest, UnknownModelYieldsEmpty) {
    const neuriplo_tasks::Tensor t = MakeTopNTensor(0.9f, 0.1f);
    const deepinfer::FrameSize sz{640, 640};
    const auto results = deepinfer::PostprocessTensors(
        {t}, {"out"}, "yolov8", sz, sz, 0.5f, 0.5f);
    EXPECT_TRUE(results.empty());
}

TEST(PostprocessTensorsTest, EmptyTensorsYieldEmpty) {
    const deepinfer::FrameSize sz{640, 640};
    const auto results = deepinfer::PostprocessTensors(
        {}, {}, "yolo_pose", sz, sz, 0.5f, 0.5f);
    EXPECT_TRUE(results.empty());
}

TEST(PostprocessTensorsTest, PoseTopNDecodesPoses) {
    const neuriplo_tasks::Tensor t = MakeTopNTensor(0.9f, 0.1f);
    const deepinfer::FrameSize sz{640, 640};
    const auto results = deepinfer::PostprocessTensors(
        {t}, {"out"}, "yolo_pose", sz, sz, 0.5f, 0.5f);
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(std::holds_alternative<neuriplo_tasks::PoseEstimation>(results[0]));
}
