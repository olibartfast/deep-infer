#include "utils.hpp"
#include "common.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>

std::vector<std::string> Split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(s);
    while (std::getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string ToLower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

bool IsImageFile(const std::string& file_name) {
    std::string lower_name = ToLower(file_name);
    return (lower_name.find(".jpg") != std::string::npos ||
            lower_name.find(".jpeg") != std::string::npos ||
            lower_name.find(".png") != std::string::npos ||
            lower_name.find(".bmp") != std::string::npos);
}

bool IsVideoFile(const std::string& file_name) {
    std::string lower_name = ToLower(file_name);
    return (lower_name.find(".mp4") != std::string::npos ||
            lower_name.find(".avi") != std::string::npos ||
            lower_name.find(".mkv") != std::string::npos ||
            lower_name.find(".mov") != std::string::npos ||
            lower_name.find("rtsp://") != std::string::npos ||
            lower_name.find("rtmp://") != std::string::npos);
}

std::vector<cv::Scalar> GenerateRandomColors(size_t size) {
    std::vector<cv::Scalar> colors;
    srand(42);  // Fixed seed for reproducibility
    for (size_t i = 0; i < size; i++) {
        colors.push_back(cv::Scalar(rand() % 256, rand() % 256, rand() % 256));
    }
    return colors;
}

void DrawLabel(cv::Mat& input_image, const std::string& label,
               float confidence, int left, int top) {
    int base_line;
    std::string text = label + ": " + std::to_string(confidence).substr(0, 4);
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &base_line);
    
    top = std::max(top, label_size.height);
    cv::rectangle(input_image, 
                  cv::Point(left, top - label_size.height),
                  cv::Point(left + label_size.width, top + base_line),
                  cv::Scalar(255, 255, 255), cv::FILLED);
    cv::putText(input_image, text, cv::Point(left, top),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
}

cv::Mat Nv12ToBgr(void* data, int width, int height) {
    cv::Mat yuv(height * 3 / 2, width, CV_8UC1, data);
    cv::Mat bgr;
    cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_NV12);
    return bgr;
}
