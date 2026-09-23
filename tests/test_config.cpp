#include <gtest/gtest.h>
#include "config_manager.h"
#include <fstream>
#include <cstdio>
#include <string>

// Helper: write a temporary config file and return its path
static std::string write_temp_config(const std::string& content) {
    std::string path = "/tmp/test_health_monitor_config.json";
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}

static void remove_temp_config() {
    std::remove("/tmp/test_health_monitor_config.json");
}

// ============================================================================
// Test: valid configuration loads correctly
// ============================================================================
TEST(ConfigManagerTest, ValidConfig) {
    std::string path = write_temp_config(R"({
        "check_interval_seconds": 15,
        "cpu_threshold": 85,
        "memory_threshold": 80,
        "disk_threshold": 75,
        "temperature_threshold": 70,
        "disk_path": "/home",
        "network_interface": "wlan0",
        "critical_service": "nginx",
        "log_file": "/tmp/test.log",
        "recovery_wait_seconds": 10
    })");

    ConfigManager config;
    ASSERT_TRUE(config.load(path));

    EXPECT_EQ(config.check_interval_seconds(), 15);
    EXPECT_EQ(config.cpu_threshold(), 85);
    EXPECT_EQ(config.memory_threshold(), 80);
    EXPECT_EQ(config.disk_threshold(), 75);
    EXPECT_EQ(config.temperature_threshold(), 70);
    EXPECT_EQ(config.disk_path(), "/home");
    EXPECT_EQ(config.network_interface(), "wlan0");
    EXPECT_EQ(config.critical_service(), "nginx");
    EXPECT_EQ(config.log_file(), "/tmp/test.log");
    EXPECT_EQ(config.recovery_wait_seconds(), 10);

    remove_temp_config();
}

// ============================================================================
// Test: missing config file uses defaults
// ============================================================================
TEST(ConfigManagerTest, MissingConfigUsesDefaults) {
    ConfigManager config;
    bool loaded = config.load("/nonexistent/path/config.json");

    EXPECT_FALSE(loaded);
    // Defaults should be active
    EXPECT_EQ(config.check_interval_seconds(), 10);
    EXPECT_EQ(config.cpu_threshold(), 90);
    EXPECT_EQ(config.memory_threshold(), 90);
    EXPECT_EQ(config.disk_threshold(), 90);
    EXPECT_EQ(config.temperature_threshold(), 80);
    EXPECT_EQ(config.disk_path(), "/");
    EXPECT_EQ(config.network_interface(), "eth0");
    EXPECT_EQ(config.critical_service(), "ssh");
}

// ============================================================================
// Test: invalid JSON uses defaults
// ============================================================================
TEST(ConfigManagerTest, InvalidJsonUsesDefaults) {
    std::string path = write_temp_config("this is not valid json at all");

    ConfigManager config;
    bool loaded = config.load(path);

    EXPECT_FALSE(loaded);
    EXPECT_EQ(config.cpu_threshold(), 90);
    EXPECT_EQ(config.memory_threshold(), 90);

    remove_temp_config();
}

// ============================================================================
// Test: partial config - missing fields use defaults
// ============================================================================
TEST(ConfigManagerTest, PartialConfig) {
    std::string path = write_temp_config(R"({
        "cpu_threshold": 75,
        "network_interface": "wlan0"
    })");

    ConfigManager config;
    ASSERT_TRUE(config.load(path));

    EXPECT_EQ(config.cpu_threshold(), 75);
    EXPECT_EQ(config.network_interface(), "wlan0");
    // Other fields should retain defaults
    EXPECT_EQ(config.memory_threshold(), 90);
    EXPECT_EQ(config.disk_threshold(), 90);
    EXPECT_EQ(config.check_interval_seconds(), 10);
    EXPECT_EQ(config.critical_service(), "ssh");

    remove_temp_config();
}

// ============================================================================
// Test: out-of-range values use defaults
// ============================================================================
TEST(ConfigManagerTest, OutOfRangeValues) {
    std::string path = write_temp_config(R"({
        "cpu_threshold": 200,
        "check_interval_seconds": -5,
        "memory_threshold": 0
    })");

    ConfigManager config;
    ASSERT_TRUE(config.load(path));

    // Out of range values should be rejected, defaults used
    EXPECT_EQ(config.cpu_threshold(), 90);       // 200 > 100, rejected
    EXPECT_EQ(config.check_interval_seconds(), 10); // -5 < 1, rejected
    EXPECT_EQ(config.memory_threshold(), 90);     // 0 < 1, rejected

    remove_temp_config();
}

// ============================================================================
// Test: default values are correct
// ============================================================================
TEST(ConfigManagerTest, DefaultValues) {
    ConfigManager config;
    // No load called - pure defaults
    EXPECT_EQ(config.check_interval_seconds(), 10);
    EXPECT_EQ(config.cpu_threshold(), 90);
    EXPECT_EQ(config.memory_threshold(), 90);
    EXPECT_EQ(config.disk_threshold(), 90);
    EXPECT_EQ(config.temperature_threshold(), 80);
    EXPECT_EQ(config.disk_path(), "/");
    EXPECT_EQ(config.network_interface(), "eth0");
    EXPECT_EQ(config.critical_service(), "ssh");
    EXPECT_EQ(config.recovery_wait_seconds(), 5);
}

// ============================================================================
// Test: empty JSON object uses defaults
// ============================================================================
TEST(ConfigManagerTest, EmptyJsonObject) {
    std::string path = write_temp_config("{}");

    ConfigManager config;
    ASSERT_TRUE(config.load(path));

    EXPECT_EQ(config.cpu_threshold(), 90);
    EXPECT_EQ(config.check_interval_seconds(), 10);

    remove_temp_config();
}
