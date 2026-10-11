#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

Config ConfigManager::ParseCommandLine(int argc, const char* argv[]) {
  ParseResult result = ParseCommandLine(argc, argv, std::cout, Logger::GetInstance());
  if (result.help_requested) {
    exit(0);
  }
  return result.config;
}

ParseResult ConfigManager::ParseCommandLine(int argc, const char* argv[], std::ostream& help_out,
                                            Logger& logger) {
  ParseResult result;
  Config& config = result.config;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // Support both "--opt value" and "--opt=value" forms.
    std::string inline_value;
    bool has_inline_value = false;
    if (arg.rfind("--", 0) == 0) {
      const auto eq = arg.find('=');
      if (eq != std::string::npos) {
        inline_value = arg.substr(eq + 1);
        arg = arg.substr(0, eq);
        has_inline_value = true;
      }
    }

    // Returns the value for an option: the inline value if present
    // ("--opt=value"), otherwise the following argument ("--opt value").
    const auto next_value = [&](std::string& target) {
      if (has_inline_value) {
        target = inline_value;
      } else if (i + 1 < argc) {
        target = argv[++i];
      }
    };

    if (arg == "--help" || arg == "-h") {
      PrintHelp(help_out);
      result.help_requested = true;
      return result;
    } else if (arg == "--source" || arg == "-s") {
      next_value(config.source);
    } else if (arg == "--config" || arg == "-c") {
      next_value(config.config_file);
    } else if (arg == "--model_type" || arg == "-mt") {
      next_value(config.model_type);
    } else if (arg == "--labels" || arg == "-l") {
      next_value(config.labels_file);
    } else if (arg == "--output" || arg == "-o") {
      next_value(config.output_path);
    } else if (arg == "--confidence" || arg == "-conf") {
      std::string v;
      next_value(v);
      if (!v.empty()) {
        try {
          config.confidence_threshold = std::stof(v);
        } catch (const std::exception&) {
          throw ConfigError("--confidence requires a numeric value, got '" + v + "'");
        }
      }
    } else if (arg == "--nms" || arg == "-nms") {
      std::string v;
      next_value(v);
      if (!v.empty()) {
        try {
          config.nms_threshold = std::stof(v);
        } catch (const std::exception&) {
          throw ConfigError("--nms requires a numeric value, got '" + v + "'");
        }
      }
    } else if (arg == "--gpu" || arg == "-g") {
      std::string v;
      next_value(v);
      if (!v.empty()) {
        try {
          config.gpu_id = std::stoi(v);
        } catch (const std::exception&) {
          throw ConfigError("--gpu requires a numeric value, got '" + v + "'");
        }
      }
    } else if (arg == "--tracker") {
      config.use_tracker = true;
      if (has_inline_value) {
        config.tracker_config = inline_value;
      } else if (i + 1 < argc && argv[i + 1][0] != '-') {
        config.tracker_config = argv[++i];
      }
    } else if (arg == "--analytics") {
      config.use_analytics = true;
    } else if (arg == "--verbose" || arg == "-v") {
      config.verbose = true;
      logger.SetLogLevel(LogLevel::DEBUG);
    } else if (arg == "--show") {
      config.show_frame = true;
    } else if (arg == "--no-write") {
      config.write_frame = false;
    }
  }

  return result;
}

void ConfigManager::PrintHelp(std::ostream& out) {
  out << "Deep Infer - Computer Vision with NVIDIA DeepStream\n\n";
  out << "Usage: deep-infer [options]\n\n";
  out << "Required Options:\n";
  out << "  -s, --source <path>        Input source (video file, image, or RTSP stream)\n";
  out << "  -c, --config <path>        DeepStream config file path\n";
  out << "  -mt, --model_type <type>   Model type (yolov8, rfdetr_segmentation, yolo_pose)\n\n";
  out << "Optional:\n";
  out << "  -l, --labels <path>        Path to labels file\n";
  out << "  -o, --output <path>        Output path for results\n";
  out << "  -conf, --confidence <val>  Confidence threshold (default: 0.5)\n";
  out << "  -nms, --nms <val>          NMS threshold (default: 0.4)\n";
  out << "  -g, --gpu <id>             GPU device ID (default: 0)\n";
  out << "  --tracker [config]         Enable tracker (optional config file)\n";
  out << "  --analytics                Enable DeepStream analytics\n";
  out << "  --show                     Display output frames\n";
  out << "  --no-write                 Disable writing output to disk\n";
  out << "  -v, --verbose              Enable verbose logging\n";
  out << "  -h, --help                 Show this help message\n\n";
  out << "Examples:\n";
  out << "  deep-infer -s video.mp4 -c config.txt -mt yolov8 -l coco.names\n";
  out << "  deep-infer -s rtsp://camera -c config.txt -mt yolov5 --tracker\n";
}
