#pragma once

#include <string>
#include <cstdint>

struct CpuStats {
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;

    uint64_t total_idle() const { return idle + iowait; }
    uint64_t total_active() const { return user + nice + system + irq + softirq + steal; }
    uint64_t total() const { return total_idle() + total_active(); }
};

struct CpuInfo {
    double usage_percent = 0.0;
    bool threshold_exceeded = false;
    bool valid = false;
};

class CpuMonitor {
public:
    CpuMonitor() = default;

    /// Set the threshold percentage (0-100)
    void set_threshold(int threshold);

    /// Read current CPU stats and compute usage since last call.
    /// First call returns valid=false to avoid false 100% reading.
    CpuInfo check();

    /// Parse /proc/stat content (public for testing)
    static bool parse_proc_stat(const std::string& content, CpuStats& stats);

    /// Calculate CPU usage percentage from two snapshots
    static double calculate_usage(const CpuStats& prev, const CpuStats& curr);

protected:
    /// Virtual so tests can override
    virtual std::string read_proc_stat() const;

private:
    int threshold_ = 90;
    CpuStats prev_stats_{};
    bool has_previous_ = false;
};
