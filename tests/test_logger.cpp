#include "Logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

namespace {

// RAII replacement for std::cout's streambuf. Redirects stream output into an
// in-memory buffer and restores the original streambuf on destruction.
class CoutCapture {
 public:
    CoutCapture() : old_buf_(std::cout.rdbuf(buffer_.rdbuf())) {}

    ~CoutCapture() {
        std::cout.rdbuf(old_buf_);
    }

    CoutCapture(const CoutCapture&) = delete;
    CoutCapture& operator=(const CoutCapture&) = delete;

    std::string str() const {
        return buffer_.str();
    }

 private:
    std::stringstream buffer_;
    std::streambuf* old_buf_;
};

class LoggerTest : public ::testing::Test {
 protected:
    void TearDown() override {
        // Ensure no sink/clock/level leakage between tests.
        Logger::GetInstance().SetOutputStream(std::cout);
        Logger::GetInstance().SetClock([]{ return std::chrono::system_clock::now(); });
        Logger::GetInstance().SetLogLevel(LogLevel::INFO);
    }
};

TEST_F(LoggerTest, SingletonReturnsSameInstance) {
    Logger& first = Logger::GetInstance();
    Logger& second = Logger::GetInstance();
    EXPECT_EQ(&first, &second);
}

TEST_F(LoggerTest, InfoLevelSuppressesDebug) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    CoutCapture capture;
    logger.Debug("x");
    EXPECT_TRUE(capture.str().empty());
}

TEST_F(LoggerTest, InfoLevelEmitsInfoWithTagAndMessage) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    CoutCapture capture;
    logger.Info("x");

    const std::string output = capture.str();
    EXPECT_NE(output.find("[INFO]"), std::string::npos);
    EXPECT_NE(output.find("x"), std::string::npos);
}

TEST_F(LoggerTest, DebugLevelEmitsDebug) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::DEBUG);

    CoutCapture capture;
    logger.Debug("y");

    EXPECT_NE(capture.str().find("[DEBUG]"), std::string::npos);
}

TEST_F(LoggerTest, InfofFormatsArguments) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    CoutCapture capture;
    logger.Infof("x=%d", 7);

    EXPECT_NE(capture.str().find("x=7"), std::string::npos);
}

TEST_F(LoggerTest, WarnEmitsWarnTag) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    CoutCapture capture;
    logger.Warn("careful");

    const std::string output = capture.str();
    EXPECT_NE(output.find("[WARN]"), std::string::npos);
    EXPECT_NE(output.find("careful"), std::string::npos);
}

TEST_F(LoggerTest, ErrorEmitsErrorTag) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    CoutCapture capture;
    logger.Error("boom");

    const std::string output = capture.str();
    EXPECT_NE(output.find("[ERROR]"), std::string::npos);
    EXPECT_NE(output.find("boom"), std::string::npos);
}

TEST_F(LoggerTest, ExactFormatWithInjectedSinkAndClock) {
    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(LogLevel::INFO);

    std::ostringstream sink;
    logger.SetOutputStream(sink);

    const std::time_t fixed = 0;
    logger.SetClock([fixed] { return std::chrono::system_clock::from_time_t(fixed); });

    logger.Info("hello");

    std::time_t t = fixed;
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    const std::string expected = "[" + ts.str() + "] [INFO] hello\n";
    EXPECT_EQ(sink.str(), expected);
}

}  // namespace
