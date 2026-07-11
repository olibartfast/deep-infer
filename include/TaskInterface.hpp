#ifndef DEEPSTREAM_INFER_LAB_INCLUDE_TASK_INTERFACE_HPP_
#define DEEPSTREAM_INFER_LAB_INCLUDE_TASK_INTERFACE_HPP_

#include "common.hpp"
#include <fstream>
#include <memory>

class TaskInterface {
 public:
  virtual ~TaskInterface() = default;
  
  virtual TaskType GetTaskType() = 0;
  
  // Process metadata from DeepStream
  virtual std::vector<neuriplo_tasks::Result> ProcessMetadata(
      NvDsFrameMeta* frame_meta,
      const cv::Mat& frame) = 0;
  
  // Utility functions
  std::vector<std::string> ReadLabelNames(const std::string& file_name) const {
    std::vector<std::string> classes;
    std::ifstream ifs(file_name.c_str());
    std::string line;
    while (getline(ifs, line)) {
      classes.push_back(line);
    }
    return classes;
  }
  
 protected:
  int input_width_ = 0;
  int input_height_ = 0;
  int input_channels_ = 0;
};

#endif  // DEEPSTREAM_INFER_LAB_INCLUDE_TASK_INTERFACE_HPP_
