#include "cpu_monitor.h"
#include <fstream>
#include <sstream>
#include <cstdio>

std::string CpuMonitor::read_proc_stat() const {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool CpuMonitor::parse_proc_stat(const std::string& content, CpuStats& stats) {
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        // Look for the aggregate "cpu " line (with a space, not cpu0, cpu1, etc.)
        if (line.substr(0, 4) == "cpu ") {
            // Format: cpu  user nice system idle iowait irq softirq steal guest guest_nice
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
            // Skip "cpu" prefix
            const char* ptr = line.c_str() + 3;
            int parsed = sscanf(ptr, " %lu %lu %lu %lu %lu %lu %lu %lu",
                                &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            if (parsed >= 4) {
                stats.user = user;
                stats.nice = nice;
                stats.system = system;
                stats.idle = idle;
                stats.iowait = (parsed >= 5) ? iowait : 0;
                stats.irq = (parsed >= 6) ? irq : 0;
                stats.softirq = (parsed >= 7) ? softirq : 0;
                stats.steal = (parsed >= 8) ? steal : 0;
                return true;
            }
            return false;
        }
    }
    return false;
}

double CpuMonitor::calculate_usage(const CpuStats& prev, const CpuStats& curr) {
    uint64_t prev_total = prev.total();
    uint64_t curr_total = curr.total();
    uint64_t prev_idle = prev.total_idle();
    uint64_t curr_idle = curr.total_idle();

    uint64_t total_diff = curr_total - prev_total;
    uint64_t idle_diff = curr_idle - prev_idle;

    if (total_diff == 0) return 0.0;

    return (static_cast<double>(total_diff - idle_diff) / static_cast<double>(total_diff)) * 100.0;
}

void CpuMonitor::set_threshold(int threshold) {
    threshold_ = threshold;
}

CpuInfo CpuMonitor::check() {
    CpuInfo info;
    std::string content = read_proc_stat();
    if (content.empty()) {
        info.valid = false;
        return info;
    }

    CpuStats current;
    if (!parse_proc_stat(content, current)) {
        info.valid = false;
        return info;
    }

    if (!has_previous_) {
        // First measurement: store stats but don't report usage
        // to avoid false 100% reading
        prev_stats_ = current;
        has_previous_ = true;
        info.valid = false;
        info.usage_percent = 0.0;
        return info;
    }

    info.usage_percent = calculate_usage(prev_stats_, current);
    info.threshold_exceeded = (info.usage_percent >= static_cast<double>(threshold_));
    info.valid = true;

    prev_stats_ = current;
    return info;
}
