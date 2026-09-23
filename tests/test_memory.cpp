#include <gtest/gtest.h>
#include "memory_monitor.h"
#include <cmath>

// ============================================================================
// Mock MemoryMonitor: overrides read_proc_meminfo() to inject test data
// ============================================================================
class MockMemoryMonitor : public MemoryMonitor {
public:
    void set_mock_data(const std::string& data) { mock_data_ = data; }

protected:
    std::string read_proc_meminfo() const override { return mock_data_; }

private:
    std::string mock_data_;
};

// ============================================================================
// Test: parse_meminfo with full data (MemAvailable present)
// ============================================================================
TEST(MemoryMonitorTest, ParseFullMeminfo) {
    std::string content =
        "MemTotal:       16384000 kB\n"
        "MemFree:         2000000 kB\n"
        "MemAvailable:    8000000 kB\n"
        "Buffers:          500000 kB\n"
        "Cached:          4000000 kB\n";

    uint64_t total = 0, available = 0;
    ASSERT_TRUE(MemoryMonitor::parse_meminfo(content, total, available));
    EXPECT_EQ(total, 16384000UL);
    EXPECT_EQ(available, 8000000UL); // Prefers MemAvailable
}

// ============================================================================
// Test: parse_meminfo falls back to MemFree when MemAvailable is missing
// ============================================================================
TEST(MemoryMonitorTest, ParseMeminfoFallbackToFree) {
    std::string content =
        "MemTotal:       16384000 kB\n"
        "MemFree:         2000000 kB\n"
        "Buffers:          500000 kB\n";

    uint64_t total = 0, available = 0;
    ASSERT_TRUE(MemoryMonitor::parse_meminfo(content, total, available));
    EXPECT_EQ(total, 16384000UL);
    EXPECT_EQ(available, 2000000UL); // Falls back to MemFree
}

// ============================================================================
// Test: parse_meminfo with empty content
// ============================================================================
TEST(MemoryMonitorTest, ParseEmptyMeminfo) {
    uint64_t total = 0, available = 0;
    EXPECT_FALSE(MemoryMonitor::parse_meminfo("", total, available));
}

// ============================================================================
// Test: parse_meminfo missing MemTotal
// ============================================================================
TEST(MemoryMonitorTest, ParseMissingTotal) {
    std::string content =
        "MemFree:         2000000 kB\n"
        "MemAvailable:    8000000 kB\n";

    uint64_t total = 0, available = 0;
    EXPECT_FALSE(MemoryMonitor::parse_meminfo(content, total, available));
}

// ============================================================================
// Test: memory utilization calculation
// ============================================================================
TEST(MemoryMonitorTest, UtilizationCalculation) {
    MockMemoryMonitor monitor;
    monitor.set_threshold(90);
    monitor.set_mock_data(
        "MemTotal:       10000000 kB\n"
        "MemFree:         1000000 kB\n"
        "MemAvailable:    5000000 kB\n"
    );

    MemoryInfo info = monitor.check();
    ASSERT_TRUE(info.valid);
    EXPECT_EQ(info.total_kb, 10000000UL);
    EXPECT_EQ(info.available_kb, 5000000UL);
    EXPECT_EQ(info.used_kb, 5000000UL);
    // 5000000/10000000 * 100 = 50%
    EXPECT_NEAR(info.usage_percent, 50.0, 0.1);
    EXPECT_FALSE(info.threshold_exceeded);
}

// ============================================================================
// Test: threshold exceeded detection
// ============================================================================
TEST(MemoryMonitorTest, ThresholdExceeded) {
    MockMemoryMonitor monitor;
    monitor.set_threshold(80);
    monitor.set_mock_data(
        "MemTotal:       10000000 kB\n"
        "MemFree:          500000 kB\n"
        "MemAvailable:     500000 kB\n"
    );

    MemoryInfo info = monitor.check();
    ASSERT_TRUE(info.valid);
    // 9500000/10000000 * 100 = 95%
    EXPECT_NEAR(info.usage_percent, 95.0, 0.1);
    EXPECT_TRUE(info.threshold_exceeded);
}

// ============================================================================
// Test: threshold not exceeded
// ============================================================================
TEST(MemoryMonitorTest, ThresholdNotExceeded) {
    MockMemoryMonitor monitor;
    monitor.set_threshold(90);
    monitor.set_mock_data(
        "MemTotal:       10000000 kB\n"
        "MemFree:         5000000 kB\n"
        "MemAvailable:    7000000 kB\n"
    );

    MemoryInfo info = monitor.check();
    ASSERT_TRUE(info.valid);
    // 3000000/10000000 * 100 = 30%
    EXPECT_NEAR(info.usage_percent, 30.0, 0.1);
    EXPECT_FALSE(info.threshold_exceeded);
}

// ============================================================================
// Test: invalid data returns valid=false
// ============================================================================
TEST(MemoryMonitorTest, InvalidDataReturnsInvalid) {
    MockMemoryMonitor monitor;
    monitor.set_mock_data("garbage data here");

    MemoryInfo info = monitor.check();
    EXPECT_FALSE(info.valid);
}
