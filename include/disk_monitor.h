#pragma once

#include <string>
#include <cstdint>

struct DiskInfo {
    uint64_t total_bytes = 0;
    uint64_t available_bytes = 0;
    uint64_t used_bytes = 0;
    double usage_percent = 0.0;
    bool threshold_exceeded = false;
    bool valid = false;
};

class DiskMonitor {
public:
    DiskMonitor() = default;

    void set_threshold(int threshold);
    void set_path(const std::string& path);
    DiskInfo check();

private:
    int threshold_ = 90;
    std::string path_ = "/";
};
