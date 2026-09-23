#pragma once

#include <string>
#include <cstdint>

struct MemoryInfo {
    uint64_t total_kb = 0;
    uint64_t available_kb = 0;
    uint64_t used_kb = 0;
    double usage_percent = 0.0;
    bool threshold_exceeded = false;
    bool valid = false;
};

class MemoryMonitor {
public:
    MemoryMonitor() = default;

    void set_threshold(int threshold);
    MemoryInfo check();

    /// Parse /proc/meminfo content (public for testing)
    static bool parse_meminfo(const std::string& content, uint64_t& total, uint64_t& available);

protected:
    virtual std::string read_proc_meminfo() const;

private:
    int threshold_ = 90;
};
