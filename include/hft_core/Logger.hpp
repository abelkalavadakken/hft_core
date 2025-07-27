#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace hft::core {

enum class LogLevel : int {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

struct LogMessage {
    LogLevel level;
    std::string message;
    std::string file;
    int line;
    std::thread::id thread_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    LogMessage(LogLevel lvl, std::string msg, std::string f, int l)
        : level(lvl), message(std::move(msg)), file(std::move(f)), line(l),
          thread_id(std::this_thread::get_id()),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) noexcept {
        min_level_.store(level, std::memory_order_relaxed);
    }

    void set_output_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        output_file_ = filename;
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        file_stream_.open(filename, std::ios::app);
    }

    void log(LogLevel level, const std::string& message, 
             const std::string& file, int line) {
        if (level < min_level_.load(std::memory_order_relaxed)) {
            return;
        }

        auto log_msg = std::make_unique<LogMessage>(level, message, file, line);
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            log_queue_.push(std::move(log_msg));
        }
        queue_cv_.notify_one();
    }

    void start_background_thread() {
        background_thread_ = std::thread(&Logger::background_worker, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_requested_ = true;
        }
        queue_cv_.notify_one();
        
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
    }

    ~Logger() {
        stop();
    }

private:
    Logger() : min_level_(LogLevel::INFO), stop_requested_(false) {
        start_background_thread();
    }

    void background_worker() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { 
                return !log_queue_.empty() || stop_requested_; 
            });

            if (stop_requested_ && log_queue_.empty()) {
                break;
            }

            while (!log_queue_.empty()) {
                auto msg = std::move(log_queue_.front());
                log_queue_.pop();
                lock.unlock();
                
                write_message(*msg);
                
                lock.lock();
            }
        }
    }

    void write_message(const LogMessage& msg) {
        std::ostringstream oss;
        
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        auto tm = *std::localtime(&time_t);
        
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ";
        oss << "[" << level_to_string(msg.level) << "] ";
        oss << "[" << msg.thread_id << "] ";
        oss << msg.message;
        oss << " (" << msg.file << ":" << msg.line << ")\n";

        std::string formatted = oss.str();
        
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            if (file_stream_.is_open()) {
                file_stream_ << formatted;
                file_stream_.flush();
            } else {
                std::cout << formatted;
            }
        }
    }

    const char* level_to_string(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    std::atomic<LogLevel> min_level_;
    std::queue<std::unique_ptr<LogMessage>> log_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread background_thread_;
    std::atomic<bool> stop_requested_;
    
    std::string output_file_;
    std::ofstream file_stream_;
    std::mutex config_mutex_;
};

#define LOG_TRACE(msg) hft::core::Logger::instance().log(hft::core::LogLevel::TRACE, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) hft::core::Logger::instance().log(hft::core::LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  hft::core::Logger::instance().log(hft::core::LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  hft::core::Logger::instance().log(hft::core::LogLevel::WARN, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) hft::core::Logger::instance().log(hft::core::LogLevel::ERROR, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) hft::core::Logger::instance().log(hft::core::LogLevel::FATAL, msg, __FILE__, __LINE__)

} // namespace hft::core