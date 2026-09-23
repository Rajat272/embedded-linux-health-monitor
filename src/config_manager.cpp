#include "config_manager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

// ============================================================================
// Minimal JSON parser - avoids external dependency for embedded use
// Parses a flat JSON object with string and integer values only.
// For production, consider nlohmann/json fetched via CMake FetchContent.
// ============================================================================

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Remove quotes from a JSON string value
std::string unquote(const std::string& s) {
    std::string t = trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

// Very simple flat JSON object parser: {"key": value, ...}
// Supports string and integer values only.
bool parse_json(const std::string& json_str,
                std::unordered_map<std::string, std::string>& out) {
    // Find the outermost braces
    size_t start = json_str.find('{');
    size_t end = json_str.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }
    std::string body = json_str.substr(start + 1, end - start - 1);

    // Split by commas (simple: doesn't handle commas inside strings)
    // This is sufficient for our flat config with no nested objects
    std::vector<std::string> pairs;
    bool in_string = false;
    size_t pair_start = 0;
    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '"') in_string = !in_string;
        if (!in_string && body[i] == ',') {
            pairs.push_back(body.substr(pair_start, i - pair_start));
            pair_start = i + 1;
        }
    }
    pairs.push_back(body.substr(pair_start));

    for (const auto& pair : pairs) {
        size_t colon = std::string::npos;
        bool in_str = false;
        for (size_t i = 0; i < pair.size(); ++i) {
            if (pair[i] == '"') in_str = !in_str;
            if (!in_str && pair[i] == ':') {
                colon = i;
                break;
            }
        }
        if (colon == std::string::npos) continue;

        std::string key = unquote(pair.substr(0, colon));
        std::string value = trim(pair.substr(colon + 1));
        // Remove trailing comma if any
        if (!value.empty() && value.back() == ',') value.pop_back();
        value = trim(value);

        // Unquote string values, leave numbers as-is
        if (!value.empty() && value.front() == '"') {
            value = unquote(value);
        }
        out[key] = value;
    }
    return true;
}

} // anonymous namespace

ConfigManager::ConfigManager() {
    set_defaults();
}

void ConfigManager::set_defaults() {
    check_interval_seconds_ = 10;
    cpu_threshold_ = 90;
    memory_threshold_ = 90;
    disk_threshold_ = 90;
    temperature_threshold_ = 80;
    disk_path_ = "/";
    network_interface_ = "eth0";
    critical_service_ = "ssh";
    log_file_ = "/var/log/device-health-monitor.log";
    recovery_wait_seconds_ = 5;
}

bool ConfigManager::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << config_path
                  << ". Using defaults." << std::endl;
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();
    file.close();

    std::unordered_map<std::string, std::string> values;
    if (!parse_json(content, values)) {
        std::cerr << "Warning: Failed to parse config JSON. Using defaults." << std::endl;
        return false;
    }

    // Parse integer values with validation
    auto parse_int = [&](const std::string& key, int& target, int min_val, int max_val) {
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                int v = std::stoi(it->second);
                if (v >= min_val && v <= max_val) {
                    target = v;
                } else {
                    std::cerr << "Warning: " << key << " value " << v
                              << " out of range [" << min_val << "," << max_val
                              << "]. Using default." << std::endl;
                }
            } catch (...) {
                std::cerr << "Warning: Invalid value for " << key
                          << ". Using default." << std::endl;
            }
        }
    };

    auto parse_string = [&](const std::string& key, std::string& target) {
        auto it = values.find(key);
        if (it != values.end() && !it->second.empty()) {
            target = it->second;
        }
    };

    parse_int("check_interval_seconds", check_interval_seconds_, 1, 3600);
    parse_int("cpu_threshold", cpu_threshold_, 1, 100);
    parse_int("memory_threshold", memory_threshold_, 1, 100);
    parse_int("disk_threshold", disk_threshold_, 1, 100);
    parse_int("temperature_threshold", temperature_threshold_, 1, 150);
    parse_int("recovery_wait_seconds", recovery_wait_seconds_, 1, 300);
    parse_string("disk_path", disk_path_);
    parse_string("network_interface", network_interface_);
    parse_string("critical_service", critical_service_);
    parse_string("log_file", log_file_);

    return true;
}

int ConfigManager::check_interval_seconds() const { return check_interval_seconds_; }
int ConfigManager::cpu_threshold() const { return cpu_threshold_; }
int ConfigManager::memory_threshold() const { return memory_threshold_; }
int ConfigManager::disk_threshold() const { return disk_threshold_; }
int ConfigManager::temperature_threshold() const { return temperature_threshold_; }
std::string ConfigManager::disk_path() const { return disk_path_; }
std::string ConfigManager::network_interface() const { return network_interface_; }
std::string ConfigManager::critical_service() const { return critical_service_; }
std::string ConfigManager::log_file() const { return log_file_; }
int ConfigManager::recovery_wait_seconds() const { return recovery_wait_seconds_; }
