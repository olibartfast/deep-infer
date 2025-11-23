#include "Config.hpp"
#include "Logger.hpp"
#include "DeepStreamPipeline.hpp"
#include "utils.hpp"
#include <iostream>
#include <memory>

// Forward declaration
class ConfigManager;
#include "../utils/ConfigManager.cpp"

int main(int argc, const char* argv[]) {
    Logger& logger = Logger::GetInstance();
    
    try {
        logger.Info("=== DeepStream Inference Lab ===");
        logger.Info("Starting application...");
        
        // Parse command line arguments
        Config config = ConfigManager::ParseCommandLine(argc, argv);
        
        // Validate configuration
        if (!config.IsValid()) {
            logger.Error("Invalid configuration: " + config.GetValidationError());
            logger.Error("Use --help for usage information");
            return 1;
        }
        
        logger.Infof("Source: %s", config.source.c_str());
        logger.Infof("Model Type: %s", config.model_type.c_str());
        logger.Infof("Config File: %s", config.config_file.c_str());
        logger.Infof("GPU ID: %d", config.gpu_id);
        
        // Create DeepStream pipeline
        DeepStreamPipeline pipeline(config);
        
        // Initialize pipeline
        if (!pipeline.Initialize()) {
            logger.Error("Failed to initialize DeepStream pipeline");
            return 1;
        }
        
        // Set frame callback for processing results
        pipeline.SetFrameCallback([&](const Result& result) {
            // Process results here
            logger.Debugf("Processed frame with %zu detections", result.boxes.size());
            
            #ifdef WRITE_FRAME
            if (config.write_frame && !result.frame.empty()) {
                // Write frame to output
                static int frame_count = 0;
                std::string output_file = config.output_path + "/frame_" + 
                                         std::to_string(frame_count++) + ".jpg";
                cv::imwrite(output_file, result.frame);
            }
            #endif
            
            #ifdef SHOW_FRAME
            if (config.show_frame && !result.frame.empty()) {
                cv::imshow("DeepStream Inference", result.frame);
                cv::waitKey(1);
            }
            #endif
        });
        
        // Run pipeline
        logger.Info("Running DeepStream pipeline...");
        if (!pipeline.Run()) {
            logger.Error("Pipeline execution failed");
            return 1;
        }
        
        logger.Info("Pipeline completed successfully");
        return 0;
        
    } catch (const std::exception& e) {
        logger.Errorf("Exception: %s", e.what());
        return 1;
    } catch (...) {
        logger.Error("Unknown exception occurred");
        return 1;
    }
}
