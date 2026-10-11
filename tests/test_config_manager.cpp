#include "ConfigManager.hpp"
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

Config Parse(const std::vector<std::string>& args) {
    std::vector<const char*> argv;
    const std::string program = "deep-infer";
    argv.push_back(program.c_str());
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    return ConfigManager::ParseCommandLine(static_cast<int>(argv.size()), argv.data());
}

ParseResult ParseWithHelp(const std::vector<std::string>& args, std::ostringstream& help_out) {
    std::vector<const char*> argv;
    const std::string program = "deep-infer";
    argv.push_back(program.c_str());
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    return ConfigManager::ParseCommandLine(static_cast<int>(argv.size()), argv.data(), help_out,
                                           Logger::GetInstance());
}

}  // namespace

TEST(ConfigManagerTest, NoArgumentsYieldsDefaults) {
    const Config config = Parse({});
    EXPECT_TRUE(config.source.empty());
    EXPECT_TRUE(config.model_type.empty());
    EXPECT_EQ(config.gpu_id, 0);
    EXPECT_TRUE(config.write_frame);
}

TEST(ConfigManagerTest, ParsesShortOptionsWithSeparateValues) {
    const Config config = Parse({
        "-s", "video.mp4",
        "-c", "config.txt",
        "-mt", "yolov8",
        "-l", "coco.names",
        "-o", "output",
    });

    EXPECT_EQ(config.source, "video.mp4");
    EXPECT_EQ(config.config_file, "config.txt");
    EXPECT_EQ(config.model_type, "yolov8");
    EXPECT_EQ(config.labels_file, "coco.names");
    EXPECT_EQ(config.output_path, "output");
}

TEST(ConfigManagerTest, ParsesLongOptionsWithSeparateValues) {
    const Config config = Parse({
        "--source", "clip.mkv",
        "--config", "cfg.txt",
        "--model_type", "yolo_pose",
    });

    EXPECT_EQ(config.source, "clip.mkv");
    EXPECT_EQ(config.config_file, "cfg.txt");
    EXPECT_EQ(config.model_type, "yolo_pose");
}

TEST(ConfigManagerTest, ParsesInlineEqualsForm) {
    const Config config = Parse({
        "--source=video.mp4",
        "--config=config.txt",
        "--model_type=yolov8",
        "--confidence=0.75",
        "--nms=0.3",
        "--gpu=2",
    });

    EXPECT_EQ(config.source, "video.mp4");
    EXPECT_EQ(config.config_file, "config.txt");
    EXPECT_EQ(config.model_type, "yolov8");
    EXPECT_FLOAT_EQ(config.confidence_threshold, 0.75f);
    EXPECT_FLOAT_EQ(config.nms_threshold, 0.3f);
    EXPECT_EQ(config.gpu_id, 2);
}

TEST(ConfigManagerTest, ParsesNumericThresholds) {
    const Config config = Parse({"-conf", "0.25", "-nms", "0.6", "-g", "3"});

    EXPECT_FLOAT_EQ(config.confidence_threshold, 0.25f);
    EXPECT_FLOAT_EQ(config.nms_threshold, 0.6f);
    EXPECT_EQ(config.gpu_id, 3);
}

TEST(ConfigManagerTest, BooleanFlags) {
    const Config config = Parse({"--show", "--no-write", "--analytics"});

    EXPECT_TRUE(config.show_frame);
    EXPECT_FALSE(config.write_frame);
    EXPECT_TRUE(config.use_analytics);
}

TEST(ConfigManagerTest, VerboseFlagEnablesDebugLogging) {
    const Config config = Parse({"--verbose"});
    EXPECT_TRUE(config.verbose);
    Logger::GetInstance().SetLogLevel(LogLevel::INFO);
}

TEST(ConfigManagerTest, TrackerWithoutConfigEnablesTracking) {
    const Config config = Parse({"--tracker"});

    EXPECT_TRUE(config.use_tracker);
    EXPECT_TRUE(config.tracker_config.empty());
}

TEST(ConfigManagerTest, TrackerConsumesFollowingConfigPath) {
    const Config config = Parse({"--tracker", "tracker.yml"});

    EXPECT_TRUE(config.use_tracker);
    EXPECT_EQ(config.tracker_config, "tracker.yml");
}

TEST(ConfigManagerTest, TrackerDoesNotConsumeAnotherOption) {
    const Config config = Parse({"--tracker", "--show"});

    EXPECT_TRUE(config.use_tracker);
    EXPECT_TRUE(config.tracker_config.empty());
    EXPECT_TRUE(config.show_frame);
}

TEST(ConfigManagerTest, TrackerInlineConfigPath) {
    const Config config = Parse({"--tracker=deepstream_tracker.yml"});

    EXPECT_TRUE(config.use_tracker);
    EXPECT_EQ(config.tracker_config, "deepstream_tracker.yml");
}

TEST(ConfigManagerTest, MissingValueLeavesFieldEmpty) {
    const Config config = Parse({"-s"});
    EXPECT_TRUE(config.source.empty());
}

TEST(ConfigManagerTest, UnknownOptionIsIgnored) {
    const Config config = Parse({"--does-not-exist", "-s", "video.mp4"});
    EXPECT_EQ(config.source, "video.mp4");
}

TEST(ConfigManagerTest, HelpSetsHelpRequested) {
    std::ostringstream help_out;
    const ParseResult result = ParseWithHelp({"--help"}, help_out);
    EXPECT_TRUE(result.help_requested);
    EXPECT_NE(help_out.str().find("Usage: deep-infer"), std::string::npos);
}

TEST(ConfigManagerTest, ShortHelpSetsHelpRequested) {
    std::ostringstream help_out;
    const ParseResult result = ParseWithHelp({"-h"}, help_out);
    EXPECT_TRUE(result.help_requested);
    EXPECT_NE(help_out.str().find("Usage: deep-infer"), std::string::npos);
}

TEST(ConfigManagerTest, NormalArgsDoNotRequestHelp) {
    std::ostringstream help_out;
    const ParseResult result =
        ParseWithHelp({"-s", "video.mp4", "-c", "config.txt", "-mt", "yolov8"}, help_out);
    EXPECT_FALSE(result.help_requested);
    EXPECT_EQ(result.config.source, "video.mp4");
    EXPECT_EQ(result.config.config_file, "config.txt");
    EXPECT_EQ(result.config.model_type, "yolov8");
}

TEST(ConfigManagerTest, InvalidConfidenceThrowsConfigError) {
    std::ostringstream help_out;
    std::vector<const char*> argv = {"deep-infer", "--confidence=abc"};
    EXPECT_THROW(
        (void)ConfigManager::ParseCommandLine(2, argv.data(), help_out, Logger::GetInstance()),
        ConfigError);
}

TEST(ConfigManagerTest, InvalidNmsThrowsConfigError) {
    std::ostringstream help_out;
    std::vector<const char*> argv = {"deep-infer", "--nms=xyz"};
    EXPECT_THROW(
        (void)ConfigManager::ParseCommandLine(2, argv.data(), help_out, Logger::GetInstance()),
        ConfigError);
}

TEST(ConfigManagerTest, InvalidGpuThrowsConfigError) {
    std::ostringstream help_out;
    std::vector<const char*> argv = {"deep-infer", "--gpu=abc"};
    EXPECT_THROW(
        (void)ConfigManager::ParseCommandLine(2, argv.data(), help_out, Logger::GetInstance()),
        ConfigError);
}
