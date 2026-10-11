#include "Config.hpp"

#include <gtest/gtest.h>

TEST(ConfigTest, HasSensibleDefaults) {
    Config config;

    EXPECT_TRUE(config.source.empty());
    EXPECT_TRUE(config.model_type.empty());
    EXPECT_TRUE(config.config_file.empty());
    EXPECT_TRUE(config.model_engine.empty());
    EXPECT_TRUE(config.onnx_file.empty());

    EXPECT_EQ(config.batch_size, 1);
    EXPECT_EQ(config.input_width, 640);
    EXPECT_EQ(config.input_height, 640);
    EXPECT_FLOAT_EQ(config.confidence_threshold, 0.5f);
    EXPECT_FLOAT_EQ(config.nms_threshold, 0.4f);
    EXPECT_EQ(config.gpu_id, 0);

    EXPECT_FALSE(config.verbose);
    EXPECT_FALSE(config.show_frame);
    EXPECT_TRUE(config.write_frame);
    EXPECT_FALSE(config.use_tracker);
    EXPECT_FALSE(config.use_analytics);
}

TEST(ConfigValidationTest, DefaultConfigIsInvalid) {
    EXPECT_FALSE(Config{}.IsValid());
}

TEST(ConfigValidationTest, RequiresSource) {
    Config config;
    config.model_type = "yolov8";
    config.config_file = "config.txt";
    EXPECT_FALSE(config.IsValid());
    EXPECT_EQ(config.GetValidationError(), "Source is required");
}

TEST(ConfigValidationTest, RequiresModelType) {
    Config config;
    config.source = "video.mp4";
    config.config_file = "config.txt";
    EXPECT_FALSE(config.IsValid());
    EXPECT_EQ(config.GetValidationError(), "Model type is required");
}

TEST(ConfigValidationTest, RequiresOneModelArtifact) {
    Config config;
    config.source = "video.mp4";
    config.model_type = "yolov8";
    EXPECT_FALSE(config.IsValid());
    EXPECT_EQ(config.GetValidationError(),
              "Either config_file, model_engine, or onnx_file is required");
}

TEST(ConfigValidationTest, ConfigFileSatisfiesModelArtifact) {
    Config config;
    config.source = "video.mp4";
    config.model_type = "yolov8";
    config.config_file = "config.txt";
    EXPECT_TRUE(config.IsValid());
    EXPECT_EQ(config.GetValidationError(), "");
}

TEST(ConfigValidationTest, EngineFileSatisfiesModelArtifact) {
    Config config;
    config.source = "video.mp4";
    config.model_type = "yolov8";
    config.model_engine = "model.engine";
    EXPECT_TRUE(config.IsValid());
}

TEST(ConfigValidationTest, OnnxFileSatisfiesModelArtifact) {
    Config config;
    config.source = "rtsp://camera/stream";
    config.model_type = "yolo_pose";
    config.onnx_file = "model.onnx";
    EXPECT_TRUE(config.IsValid());
}
