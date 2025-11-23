#ifndef DEEPSTREAM_INFER_LAB_INCLUDE_LOGGER_HPP_
#define DEEPSTREAM_INFER_LAB_INCLUDE_LOGGER_HPP_

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
 public:
  static Logger& GetInstance() {
    static Logger instance;
    return instance;
  }
  
  void SetLogLevel(LogLevel level) {
    current_level_ = level;
  }
  
  void SetLogFile(const std::string& filename) {
    if (log_file_.is_open()) {
      log_file_.close();
    }
    log_file_.open(filename, std::ios::app);
  }
  
  void Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
  }
  
  void Info(const std::string& message) {
    Log(LogLevel::INFO, message);
  }
  
  void Warn(const std::string& message) {
    Log(LogLevel::WARN, message);
  }
  
  void Error(const std::string& message) {
    Log(LogLevel::ERROR, message);
  }
  
  template<typename... Args>
  void Debugf(const std::string& format, Args... args) {
    Log(LogLevel::DEBUG, FormatString(format, args...));
  }
  
  template<typename... Args>
  void Infof(const std::string& format, Args... args) {
    Log(LogLevel::INFO, FormatString(format, args...));
  }
  
  template<typename... Args>
  void Warnf(const std::string& format, Args... args) {
    Log(LogLevel::WARN, FormatString(format, args...));
  }
  
  template<typename... Args>
  void Errorf(const std::string& format, Args... args) {
    Log(LogLevel::ERROR, FormatString(format, args...));
  }
    
 private:
  Logger() : current_level_(LogLevel::INFO) {}
  
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  
  void Log(LogLevel level, const std::string& message) {
    if (level < current_level_) {
      return;
    }
    
    std::string formatted = FormatMessage(level, message);
    std::cout << formatted << std::endl;
    
    if (log_file_.is_open()) {
      log_file_ << formatted << std::endl;
      log_file_.flush();
    }
  }
  
  std::string FormatMessage(LogLevel level, const std::string& message) {
    std::stringstream ss;
    ss << "[" << GetCurrentTime() << "] ";
    ss << "[" << LevelToString(level) << "] ";
    ss << message;
    return ss.str();
  }
  
  std::string LevelToString(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG: return "DEBUG";
      case LogLevel::INFO:  return "INFO";
      case LogLevel::WARN:  return "WARN";
      case LogLevel::ERROR: return "ERROR";
      default: return "UNKNOWN";
    }
  }
  
  std::string GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
  }
  
  template<typename... Args>
  std::string FormatString(const std::string& format, Args... args) {
    size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
  }
  
  LogLevel current_level_;
  std::ofstream log_file_;
};

#endif  // DEEPSTREAM_INFER_LAB_INCLUDE_LOGGER_HPP_
