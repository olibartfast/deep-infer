#ifndef DEEP_INFER_INCLUDE_UTILS_HPP_
#define DEEP_INFER_INCLUDE_UTILS_HPP_

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

std::vector<std::string> Split(const std::string& s, char delimiter);
std::string ToLower(const std::string& str);
bool IsImageFile(const std::string& file_name);
bool IsVideoFile(const std::string& file_name);
std::vector<cv::Scalar> GenerateRandomColors(size_t size);
void DrawLabel(cv::Mat& input_image, const std::string& label, 
               float confidence, int left, int top);
cv::Mat Nv12ToBgr(void* data, int width, int height);

#endif  // DEEP_INFER_INCLUDE_UTILS_HPP_
