#include "disk_monitor.h"
#include <sys/statvfs.h>

void DiskMonitor::set_threshold(int threshold) {
    threshold_ = threshold;
}

void DiskMonitor::set_path(const std::string& path) {
    path_ = path;
}

DiskInfo DiskMonitor::check() {
    DiskInfo info;

    struct statvfs stat{};
    if (statvfs(path_.c_str(), &stat) != 0) {
        info.valid = false;
        return info;
    }

    // Total space = total blocks * block size
    info.total_bytes = static_cast<uint64_t>(stat.f_blocks) * stat.f_frsize;
    // Available space = available blocks * block size (for unprivileged users)
    info.available_bytes = static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    // Used space
    uint64_t used_blocks = stat.f_blocks - stat.f_bfree;
    info.used_bytes = used_blocks * stat.f_frsize;

    // Usage percentage based on space available to non-root
    // used / (used + available) matches what df reports
    uint64_t usable_total = info.used_bytes + info.available_bytes;
    info.usage_percent = (usable_total > 0) ?
        (static_cast<double>(info.used_bytes) / static_cast<double>(usable_total)) * 100.0 : 0.0;

    info.threshold_exceeded = (info.usage_percent >= static_cast<double>(threshold_));
    info.valid = true;

    return info;
}
