#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::init(const std::string& log_file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    log_file_.open(log_file_path, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Warning: Could not open log file: " << log_file_path
                  << ". Logging to stderr only." << std::endl;
    }
    initialized_ = true;
}

void Logger::log(LogLevel level, const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);

    std::ostringstream oss;
    oss << "[" << timestamp << "] [" << level_str << "] [" << component << "] " << message;
    std::string log_line = oss.str();

    // Always print to stderr (journald captures this for systemd services)
    std::cerr << log_line << std::endl;

    // Also write to log file if available
    if (log_file_.is_open()) {
        log_file_ << log_line << std::endl;
        log_file_.flush();
    }
}

void Logger::info(const std::string& component, const std::string& message) {
    log(LogLevel::INFO, component, message);
}

void Logger::warning(const std::string& component, const std::string& message) {
    log(LogLevel::WARNING, component, message);
}

void Logger::error(const std::string& component, const std::string& message) {
    log(LogLevel::ERROR, component, message);
}

void Logger::critical(const std::string& component, const std::string& message) {
    log(LogLevel::CRITICAL, component, message);
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf{};
    localtime_r(&time_t_now, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
