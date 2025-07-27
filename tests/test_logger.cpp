#include <gtest/gtest.h>
#include "hft_core/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace hft::core;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_file_ = "test_log.txt";
    }

    void TearDown() override {
        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }
    }

    std::string test_log_file_;
};

TEST_F(LoggerTest, BasicLogging) {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::DEBUG);
    logger.set_output_file(test_log_file_);
    
    logger.log(LogLevel::INFO, "Test message", __FILE__, __LINE__);
    
    // Give the background thread time to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::ifstream file(test_log_file_);
    ASSERT_TRUE(file.is_open());
    
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find("Test message") != std::string::npos);
    EXPECT_TRUE(line.find("[INFO]") != std::string::npos);
}

TEST_F(LoggerTest, LogLevelFiltering) {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::ERROR);
    logger.set_output_file(test_log_file_);
    
    logger.log(LogLevel::DEBUG, "Debug message", __FILE__, __LINE__);
    logger.log(LogLevel::INFO, "Info message", __FILE__, __LINE__);
    logger.log(LogLevel::ERROR, "Error message", __FILE__, __LINE__);
    
    // Give the background thread time to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::ifstream file(test_log_file_);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Debug message") == std::string::npos);
    EXPECT_TRUE(content.find("Info message") == std::string::npos);
    EXPECT_TRUE(content.find("Error message") != std::string::npos);
}

TEST_F(LoggerTest, MacroUsage) {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::TRACE);
    logger.set_output_file(test_log_file_);
    
    LOG_INFO("Macro test message");
    LOG_ERROR("Macro error message");
    
    // Give the background thread time to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::ifstream file(test_log_file_);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Macro test message") != std::string::npos);
    EXPECT_TRUE(content.find("Macro error message") != std::string::npos);
    EXPECT_TRUE(content.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(content.find("[ERROR]") != std::string::npos);
}