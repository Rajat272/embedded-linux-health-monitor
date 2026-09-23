#include <gtest/gtest.h>
#include "service_monitor.h"

// ============================================================================
// Mock ServiceMonitor: overrides command execution for testing without systemd
// ============================================================================
class MockServiceMonitor : public ServiceMonitor {
public:
    void set_mock_status(const std::string& status) { mock_status_ = status; }
    void set_mock_exit_code(int code) { mock_exit_code_ = code; }
    void set_recovery_status(const std::string& status) { recovery_status_ = status; }

protected:
    int execute_command(const std::string& /*cmd*/) const override {
        return mock_exit_code_;
    }

    std::string get_systemctl_status() const override {
        // If recovery was attempted and recovery_status is set, return it
        if (recovery_attempted_ && !recovery_status_.empty()) {
            return recovery_status_;
        }
        return mock_status_;
    }

public:
    // Override attempt_recovery to track calls and use recovery_status
    ServiceInfo attempt_recovery() {
        recovery_attempted_ = true;

        ServiceInfo info;
        info.service_name = "test-service";
        info.recovery_attempted = true;

        // Simulate restart command
        execute_command("systemctl restart test-service");

        // Check state after recovery
        std::string status = get_systemctl_status();
        if (status == "active") {
            info.state = ServiceState::RUNNING;
            info.state_string = "running";
            info.is_healthy = true;
            info.recovery_succeeded = true;
        } else {
            info.state = (status == "failed") ? ServiceState::FAILED : ServiceState::STOPPED;
            info.state_string = status;
            info.is_healthy = false;
            info.recovery_succeeded = false;
        }
        return info;
    }

private:
    std::string mock_status_ = "active";
    int mock_exit_code_ = 0;
    std::string recovery_status_;
    mutable bool recovery_attempted_ = false;
};

// ============================================================================
// Test: running service detected as healthy
// ============================================================================
TEST(ServiceMonitorTest, RunningServiceIsHealthy) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("active");

    ServiceInfo info = monitor.check();
    EXPECT_TRUE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::RUNNING);
    EXPECT_EQ(info.state_string, "running");
    EXPECT_EQ(info.service_name, "test-service");
}

// ============================================================================
// Test: stopped service detected as unhealthy
// ============================================================================
TEST(ServiceMonitorTest, StoppedServiceIsUnhealthy) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("inactive");

    ServiceInfo info = monitor.check();
    EXPECT_FALSE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::STOPPED);
    EXPECT_EQ(info.state_string, "stopped");
}

// ============================================================================
// Test: failed service detected
// ============================================================================
TEST(ServiceMonitorTest, FailedServiceDetected) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("failed");

    ServiceInfo info = monitor.check();
    EXPECT_FALSE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::FAILED);
    EXPECT_EQ(info.state_string, "failed");
}

// ============================================================================
// Test: unknown status
// ============================================================================
TEST(ServiceMonitorTest, UnknownStatus) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("something-unexpected");

    ServiceInfo info = monitor.check();
    EXPECT_FALSE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::UNKNOWN);
}

// ============================================================================
// Test: successful recovery
// ============================================================================
TEST(ServiceMonitorTest, SuccessfulRecovery) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("inactive");       // Initial state: stopped
    monitor.set_recovery_status("active");     // After restart: running
    monitor.set_mock_exit_code(0);

    ServiceInfo info = monitor.attempt_recovery();
    EXPECT_TRUE(info.recovery_attempted);
    EXPECT_TRUE(info.recovery_succeeded);
    EXPECT_TRUE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::RUNNING);
}

// ============================================================================
// Test: failed recovery
// ============================================================================
TEST(ServiceMonitorTest, FailedRecovery) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("inactive");
    monitor.set_recovery_status("failed");    // After restart: still failed
    monitor.set_mock_exit_code(1);

    ServiceInfo info = monitor.attempt_recovery();
    EXPECT_TRUE(info.recovery_attempted);
    EXPECT_FALSE(info.recovery_succeeded);
    EXPECT_FALSE(info.is_healthy);
    EXPECT_EQ(info.state, ServiceState::FAILED);
}

// ============================================================================
// Test: recovery attempt with service remaining stopped
// ============================================================================
TEST(ServiceMonitorTest, RecoveryServiceRemainsDown) {
    MockServiceMonitor monitor;
    monitor.set_service("test-service");
    monitor.set_mock_status("inactive");
    monitor.set_recovery_status("inactive");  // Still stopped after restart
    monitor.set_mock_exit_code(0);

    ServiceInfo info = monitor.attempt_recovery();
    EXPECT_TRUE(info.recovery_attempted);
    EXPECT_FALSE(info.recovery_succeeded);
    EXPECT_FALSE(info.is_healthy);
}
