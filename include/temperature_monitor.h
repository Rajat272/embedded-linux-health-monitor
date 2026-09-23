#pragma once

#include <string>

struct TemperatureInfo {
    double temperature_celsius = 0.0;
    std::string zone_name;
    bool threshold_exceeded = false;
    bool valid = false;
    bool sensor_available = false;
};

class TemperatureMonitor {
public:
    TemperatureMonitor() = default;

    void set_threshold(int threshold);
    TemperatureInfo check();

private:
    /// Discover the first available thermal zone path
    std::string discover_thermal_zone() const;

    int threshold_ = 80;
    std::string cached_zone_path_;
};
