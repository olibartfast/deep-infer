#include "Config.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include <fstream>
#include <sstream>

class ConfigManager {
 public:
  static Config ParseCommandLine(int argc, const char* argv[]) {
    Config config;
    Logger& logger = Logger::GetInstance();
    
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      
      if (arg == "--help" || arg == "-h") {
        PrintHelp();
        exit(0);
      } else if (arg == "--source" || arg == "-s") {
        if (i + 1 < argc) {
          config.source = argv[++i];
        }
      } else if (arg == "--config" || arg == "-c") {
        if (i + 1 < argc) {
          config.config_file = argv[++i];
        }
      } else if (arg == "--model_type" || arg == "-mt") {
        if (i + 1 < argc) {
          config.model_type = argv[++i];
        }
      } else if (arg == "--labels" || arg == "-l") {
        if (i + 1 < argc) {
          config.labels_file = argv[++i];
        }
      } else if (arg == "--output" || arg == "-o") {
        if (i + 1 < argc) {
          config.output_path = argv[++i];
        }
      } else if (arg == "--confidence" || arg == "-conf") {
        if (i + 1 < argc) {
          config.confidence_threshold = std::stof(argv[++i]);
        }
      } else if (arg == "--nms" || arg == "-nms") {
        if (i + 1 < argc) {
          config.nms_threshold = std::stof(argv[++i]);
        }
      } else if (arg == "--gpu" || arg == "-g") {
        if (i + 1 < argc) {
          config.gpu_id = std::stoi(argv[++i]);
        }
      } else if (arg == "--tracker") {
        config.use_tracker = true;
        if (i + 1 < argc && argv[i + 1][0] != '-') {
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
    
    return config;
  }
    
 private:
  static void PrintHelp() {
    std::cout << "DeepStream Inference Lab - Computer Vision with NVIDIA DeepStream\n\n";
    std::cout << "Usage: deepstream-infer-lab [options]\n\n";
    std::cout << "Required Options:\n";
    std::cout << "  -s, --source <path>        Input source (video file, image, or RTSP stream)\n";
    std::cout << "  -c, --config <path>        DeepStream config file path\n";
    std::cout << "  -mt, --model_type <type>   Model type (yolov5, yolov8, etc.)\n\n";
    std::cout << "Optional:\n";
    std::cout << "  -l, --labels <path>        Path to labels file\n";
    std::cout << "  -o, --output <path>        Output path for results\n";
    std::cout << "  -conf, --confidence <val>  Confidence threshold (default: 0.5)\n";
    std::cout << "  -nms, --nms <val>          NMS threshold (default: 0.4)\n";
    std::cout << "  -g, --gpu <id>             GPU device ID (default: 0)\n";
    std::cout << "  --tracker [config]         Enable tracker (optional config file)\n";
    std::cout << "  --analytics                Enable DeepStream analytics\n";
    std::cout << "  --show                     Display output frames\n";
    std::cout << "  --no-write                 Disable writing output to disk\n";
    std::cout << "  -v, --verbose              Enable verbose logging\n";
    std::cout << "  -h, --help                 Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  deepstream-infer-lab -s video.mp4 -c config.txt -mt yolov8 -l coco.names\n";
    std::cout << "  deepstream-infer-lab -s rtsp://camera -c config.txt -mt yolov5 --tracker\n";
  }
};
