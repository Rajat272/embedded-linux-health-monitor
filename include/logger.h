#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& instance();

    void init(const std::string& log_file_path = "/var/log/device-health-monitor.log");
    void log(LogLevel level, const std::string& component, const std::string& message);
    void info(const std::string& component, const std::string& message);
    void warning(const std::string& component, const std::string& message);
    void error(const std::string& component, const std::string& message);
    void critical(const std::string& component, const std::string& message);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;

    std::ofstream log_file_;
    std::mutex mutex_;
    bool initialized_ = false;
};
