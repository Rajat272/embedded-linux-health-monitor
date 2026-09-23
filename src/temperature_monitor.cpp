#include "temperature_monitor.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <cstring>

void TemperatureMonitor::set_threshold(int threshold) {
    threshold_ = threshold;
}

std::string TemperatureMonitor::discover_thermal_zone() const {
    // Enumerate /sys/class/thermal/ and find the first thermal_zone* directory
    // that has a readable "temp" file
    const std::string base_path = "/sys/class/thermal/";
    DIR* dir = opendir(base_path.c_str());
    if (!dir) return "";

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        // Look for directories named thermal_zone*
        if (name.find("thermal_zone") == 0) {
            std::string temp_path = base_path + name + "/temp";
            std::ifstream test_file(temp_path);
            if (test_file.is_open()) {
                test_file.close();
                closedir(dir);
                return base_path + name;
            }
        }
    }
    closedir(dir);
    return "";
}

TemperatureInfo TemperatureMonitor::check() {
    TemperatureInfo info;

    // Use cached zone path, or discover one
    if (cached_zone_path_.empty()) {
        cached_zone_path_ = discover_thermal_zone();
    }

    if (cached_zone_path_.empty()) {
        info.valid = false;
        info.sensor_available = false;
        return info;
    }

    info.sensor_available = true;

    // Read zone type/name
    {
        std::ifstream type_file(cached_zone_path_ + "/type");
        if (type_file.is_open()) {
            std::getline(type_file, info.zone_name);
        } else {
            // Extract name from path
            size_t last_slash = cached_zone_path_.rfind('/');
            if (last_slash != std::string::npos) {
                info.zone_name = cached_zone_path_.substr(last_slash + 1);
            }
        }
    }

    // Read temperature (value is in millidegrees Celsius)
    {
        std::ifstream temp_file(cached_zone_path_ + "/temp");
        if (!temp_file.is_open()) {
            // Sensor disappeared - invalidate cache and report unavailable
            cached_zone_path_.clear();
            info.valid = false;
            info.sensor_available = false;
            return info;
        }

        long millidegrees = 0;
        temp_file >> millidegrees;
        if (temp_file.fail()) {
            info.valid = false;
            return info;
        }

        info.temperature_celsius = static_cast<double>(millidegrees) / 1000.0;
    }

    info.threshold_exceeded = (info.temperature_celsius >= static_cast<double>(threshold_));
    info.valid = true;

    return info;
}
