#pragma once

#include <string>
#include <unordered_map>

class ConfigManager {
public:
    ConfigManager();

    bool load(const std::string& config_path);

    int check_interval_seconds() const;
    int cpu_threshold() const;
    int memory_threshold() const;
    int disk_threshold() const;
    int temperature_threshold() const;
    std::string disk_path() const;
    std::string network_interface() const;
    std::string critical_service() const;
    std::string log_file() const;
    int recovery_wait_seconds() const;

private:
    void set_defaults();

    int check_interval_seconds_ = 10;
    int cpu_threshold_ = 90;
    int memory_threshold_ = 90;
    int disk_threshold_ = 90;
    int temperature_threshold_ = 80;
    std::string disk_path_ = "/";
    std::string network_interface_ = "eth0";
    std::string critical_service_ = "ssh";
    std::string log_file_ = "/var/log/device-health-monitor.log";
    int recovery_wait_seconds_ = 5;
};
