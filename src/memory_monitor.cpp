#include "memory_monitor.h"
#include <fstream>
#include <sstream>
#include <string>

std::string MemoryMonitor::read_proc_meminfo() const {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool MemoryMonitor::parse_meminfo(const std::string& content,
                                   uint64_t& total, uint64_t& available) {
    total = 0;
    available = 0;
    uint64_t mem_free = 0;
    bool has_total = false;
    bool has_available = false;
    bool has_free = false;

    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        uint64_t value = 0;
        // Each line format: "FieldName:     12345 kB"
        if (line.compare(0, 9, "MemTotal:") == 0) {
            if (sscanf(line.c_str() + 9, " %lu", &value) == 1) {
                total = value;
                has_total = true;
            }
        } else if (line.compare(0, 13, "MemAvailable:") == 0) {
            if (sscanf(line.c_str() + 13, " %lu", &value) == 1) {
                available = value;
                has_available = true;
            }
        } else if (line.compare(0, 8, "MemFree:") == 0) {
            if (sscanf(line.c_str() + 8, " %lu", &value) == 1) {
                mem_free = value;
                has_free = true;
            }
        }
    }

    if (!has_total) return false;

    // Prefer MemAvailable; fall back to MemFree
    if (!has_available) {
        if (has_free) {
            available = mem_free;
        } else {
            return false;
        }
    }

    return true;
}

void MemoryMonitor::set_threshold(int threshold) {
    threshold_ = threshold;
}

MemoryInfo MemoryMonitor::check() {
    MemoryInfo info;
    std::string content = read_proc_meminfo();
    if (content.empty()) {
        info.valid = false;
        return info;
    }

    uint64_t total = 0, available = 0;
    if (!parse_meminfo(content, total, available)) {
        info.valid = false;
        return info;
    }

    info.total_kb = total;
    info.available_kb = available;
    info.used_kb = (total > available) ? (total - available) : 0;
    info.usage_percent = (total > 0) ?
        (static_cast<double>(info.used_kb) / static_cast<double>(total)) * 100.0 : 0.0;
    info.threshold_exceeded = (info.usage_percent >= static_cast<double>(threshold_));
    info.valid = true;

    return info;
}
