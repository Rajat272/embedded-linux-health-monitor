#include <gtest/gtest.h>
#include "cpu_monitor.h"
#include <cmath>

// ============================================================================
// Mock CpuMonitor: overrides read_proc_stat() to inject test data
// ============================================================================
class MockCpuMonitor : public CpuMonitor {
public:
    void set_mock_data(const std::string& data) { mock_data_ = data; }

protected:
    std::string read_proc_stat() const override { return mock_data_; }

private:
    std::string mock_data_;
};

// ============================================================================
// Test: parse_proc_stat with valid data
// ============================================================================
TEST(CpuMonitorTest, ParseValidProcStat) {
    std::string content =
        "cpu  10132153 290696 3084719 46828483 16683 0 25195 0 0 0\n"
        "cpu0 1393280  32966  572056  13343292  6130 0 17875 0 0 0\n";

    CpuStats stats;
    ASSERT_TRUE(CpuMonitor::parse_proc_stat(content, stats));
    EXPECT_EQ(stats.user,    10132153UL);
    EXPECT_EQ(stats.nice,    290696UL);
    EXPECT_EQ(stats.system,  3084719UL);
    EXPECT_EQ(stats.idle,    46828483UL);
    EXPECT_EQ(stats.iowait,  16683UL);
    EXPECT_EQ(stats.irq,     0UL);
    EXPECT_EQ(stats.softirq, 25195UL);
    EXPECT_EQ(stats.steal,   0UL);
}

// ============================================================================
// Test: parse_proc_stat with invalid/empty data
// ============================================================================
TEST(CpuMonitorTest, ParseEmptyProcStat) {
    CpuStats stats;
    EXPECT_FALSE(CpuMonitor::parse_proc_stat("", stats));
}

TEST(CpuMonitorTest, ParseInvalidProcStat) {
    CpuStats stats;
    EXPECT_FALSE(CpuMonitor::parse_proc_stat("not a valid cpu line", stats));
}

// ============================================================================
// Test: calculate_usage produces correct percentage
// ============================================================================
TEST(CpuMonitorTest, CalculateUsage) {
    CpuStats prev;
    prev.user = 100; prev.nice = 0; prev.system = 50;
    prev.idle = 800; prev.iowait = 50;
    prev.irq = 0; prev.softirq = 0; prev.steal = 0;

    CpuStats curr;
    curr.user = 200; curr.nice = 0; curr.system = 100;
    curr.idle = 850; curr.iowait = 50;
    curr.irq = 0; curr.softirq = 0; curr.steal = 0;

    // Total diff = (200+100+850+50) - (100+50+800+50) = 1200 - 1000 = 200
    // Idle diff = (850+50) - (800+50) = 900 - 850 = 50
    // Usage = (200-50)/200 * 100 = 75%
    double usage = CpuMonitor::calculate_usage(prev, curr);
    EXPECT_NEAR(usage, 75.0, 0.1);
}

TEST(CpuMonitorTest, CalculateUsageZeroDiff) {
    CpuStats stats;
    stats.user = 100; stats.nice = 0; stats.system = 50;
    stats.idle = 800; stats.iowait = 0;
    stats.irq = 0; stats.softirq = 0; stats.steal = 0;

    // Same stats -> 0% usage
    double usage = CpuMonitor::calculate_usage(stats, stats);
    EXPECT_NEAR(usage, 0.0, 0.01);
}

// ============================================================================
// Test: first measurement returns valid=false (avoid false 100%)
// ============================================================================
TEST(CpuMonitorTest, FirstMeasurementIsInvalid) {
    MockCpuMonitor monitor;
    monitor.set_threshold(90);
    monitor.set_mock_data(
        "cpu  10132153 290696 3084719 46828483 16683 0 25195 0 0 0\n"
    );

    CpuInfo info = monitor.check();
    EXPECT_FALSE(info.valid);
}

// ============================================================================
// Test: second measurement returns valid data
// ============================================================================
TEST(CpuMonitorTest, SecondMeasurementIsValid) {
    MockCpuMonitor monitor;
    monitor.set_threshold(90);

    // First call
    monitor.set_mock_data(
        "cpu  1000 0 500 8000 100 0 0 0 0 0\n"
    );
    monitor.check();

    // Second call with slightly changed values
    monitor.set_mock_data(
        "cpu  1100 0 600 8100 100 0 0 0 0 0\n"
    );
    CpuInfo info = monitor.check();
    EXPECT_TRUE(info.valid);
    EXPECT_GE(info.usage_percent, 0.0);
    EXPECT_LE(info.usage_percent, 100.0);
}

// ============================================================================
// Test: high CPU detection with threshold
// ============================================================================
TEST(CpuMonitorTest, HighCpuDetection) {
    MockCpuMonitor monitor;
    monitor.set_threshold(50); // 50% threshold

    // First call - baseline
    monitor.set_mock_data(
        "cpu  1000 0 0 9000 0 0 0 0 0 0\n"
    );
    monitor.check();

    // Second call - 90% CPU (900 active out of 1000 new ticks)
    monitor.set_mock_data(
        "cpu  1900 0 0 9100 0 0 0 0 0 0\n"
    );
    CpuInfo info = monitor.check();
    EXPECT_TRUE(info.valid);
    EXPECT_TRUE(info.threshold_exceeded);
    EXPECT_GT(info.usage_percent, 50.0);
}

// ============================================================================
// Test: normal CPU below threshold
// ============================================================================
TEST(CpuMonitorTest, NormalCpuBelowThreshold) {
    MockCpuMonitor monitor;
    monitor.set_threshold(90);

    // First call
    monitor.set_mock_data("cpu  1000 0 0 9000 0 0 0 0 0 0\n");
    monitor.check();

    // Second call - 10% CPU
    monitor.set_mock_data("cpu  1100 0 0 9900 0 0 0 0 0 0\n");
    CpuInfo info = monitor.check();
    EXPECT_TRUE(info.valid);
    EXPECT_FALSE(info.threshold_exceeded);
    EXPECT_NEAR(info.usage_percent, 10.0, 0.5);
}
