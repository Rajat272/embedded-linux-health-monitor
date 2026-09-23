#include "config_manager.h"
#include "logger.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "disk_monitor.h"
#include "temperature_monitor.h"
#include "network_monitor.h"
#include "service_monitor.h"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

// ============================================================================
// Global flag for graceful shutdown on SIGTERM / SIGINT
// ============================================================================
static std::atomic<bool> g_running{true};

static void signal_handler(int signum) {
    (void)signum;
    g_running.store(false);
}

// ============================================================================
// Helper: format bytes to human-readable string
// ============================================================================
static std::string format_bytes(uint64_t bytes) {
    std::ostringstream oss;
    if (bytes >= (1ULL << 30)) {
        oss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1ULL << 30)) << " GB";
    } else if (bytes >= (1ULL << 20)) {
        oss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1ULL << 20)) << " MB";
    } else if (bytes >= (1ULL << 10)) {
        oss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1ULL << 10)) << " KB";
    } else {
        oss << bytes << " B";
    }
    return oss.str();
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char* argv[]) {
    // --- Install signal handlers for graceful shutdown ---
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // --- Load configuration ---
    ConfigManager config;
    std::string config_path = "/etc/device-health-monitor/health_monitor.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    config.load(config_path);

    // --- Initialize logger ---
    Logger::instance().init(config.log_file());
    Logger::instance().info("MAIN", "=== Device Health Monitor Starting ===");
    Logger::instance().info("MAIN", "Config loaded from: " + config_path);
    Logger::instance().info("MAIN", "Check interval: " + std::to_string(config.check_interval_seconds()) + "s");

    // --- Initialize monitors ---
    CpuMonitor cpu_monitor;
    cpu_monitor.set_threshold(config.cpu_threshold());

    MemoryMonitor memory_monitor;
    memory_monitor.set_threshold(config.memory_threshold());

    DiskMonitor disk_monitor;
    disk_monitor.set_threshold(config.disk_threshold());
    disk_monitor.set_path(config.disk_path());

    TemperatureMonitor temp_monitor;
    temp_monitor.set_threshold(config.temperature_threshold());

    NetworkMonitor network_monitor;
    network_monitor.set_interface(config.network_interface());

    ServiceMonitor service_monitor;
    service_monitor.set_service(config.critical_service());
    service_monitor.set_recovery_wait(config.recovery_wait_seconds());

    Logger::instance().info("MAIN", "All monitors initialized. Entering monitoring loop.");

    // ====================================================================
    // Main monitoring loop
    // ====================================================================
    while (g_running.load()) {

        // --- 1. CPU ---
        {
            CpuInfo cpu = cpu_monitor.check();
            if (cpu.valid) {
                std::ostringstream oss;
                oss << "CPU usage: " << std::fixed << std::setprecision(1)
                    << cpu.usage_percent << "%";
                if (cpu.threshold_exceeded) {
                    Logger::instance().warning("CPU",
                        "CPU usage exceeded threshold: " + oss.str());
                } else {
                    Logger::instance().info("CPU", oss.str());
                }
            } else {
                Logger::instance().info("CPU", "Collecting initial CPU measurement...");
            }
        }

        // --- 2. Memory ---
        {
            MemoryInfo mem = memory_monitor.check();
            if (mem.valid) {
                std::ostringstream oss;
                oss << "Memory usage: " << std::fixed << std::setprecision(1)
                    << mem.usage_percent << "% (used: "
                    << (mem.used_kb / 1024) << " MB / total: "
                    << (mem.total_kb / 1024) << " MB)";
                if (mem.threshold_exceeded) {
                    Logger::instance().warning("MEMORY",
                        "Memory usage exceeded threshold: " + oss.str());
                } else {
                    Logger::instance().info("MEMORY", oss.str());
                }
            } else {
                Logger::instance().error("MEMORY", "Failed to read memory information");
            }
        }

        // --- 3. Disk ---
        {
            DiskInfo disk = disk_monitor.check();
            if (disk.valid) {
                std::ostringstream oss;
                oss << "Disk usage (" << config.disk_path() << "): "
                    << std::fixed << std::setprecision(1)
                    << disk.usage_percent << "% (used: "
                    << format_bytes(disk.used_bytes) << " / total: "
                    << format_bytes(disk.total_bytes) << ")";
                if (disk.threshold_exceeded) {
                    Logger::instance().warning("DISK",
                        "Disk usage exceeded threshold: " + oss.str());
                } else {
                    Logger::instance().info("DISK", oss.str());
                }
            } else {
                Logger::instance().error("DISK",
                    "Failed to read disk information for " + config.disk_path());
            }
        }

        // --- 4. Temperature ---
        {
            TemperatureInfo temp = temp_monitor.check();
            if (temp.valid) {
                std::ostringstream oss;
                oss << "Temperature (" << temp.zone_name << "): "
                    << std::fixed << std::setprecision(1)
                    << temp.temperature_celsius << "°C";
                if (temp.threshold_exceeded) {
                    Logger::instance().critical("TEMPERATURE",
                        "Temperature exceeded threshold! " + oss.str());
                } else {
                    Logger::instance().info("TEMPERATURE", oss.str());
                }
            } else if (!temp.sensor_available) {
                Logger::instance().info("TEMPERATURE",
                    "No thermal sensor available on this system");
            } else {
                Logger::instance().error("TEMPERATURE",
                    "Failed to read temperature");
            }
        }

        // --- 5. Network ---
        {
            NetworkInfo net = network_monitor.check();
            if (net.is_healthy) {
                Logger::instance().info("NETWORK",
                    "Interface " + net.interface_name + ": " + net.state_string);
            } else {
                Logger::instance().warning("NETWORK",
                    "Interface " + net.interface_name + ": " + net.state_string);

                // Attempt recovery if interface is down
                if (net.state == NetworkState::INTERFACE_DOWN) {
                    Logger::instance().info("NETWORK",
                        "Attempting network recovery for " + net.interface_name + "...");
                    network_monitor.attempt_recovery();

                    // Re-check after recovery attempt
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    NetworkInfo recheck = network_monitor.check();
                    if (recheck.is_healthy) {
                        Logger::instance().info("NETWORK",
                            "Network recovery SUCCEEDED for " + net.interface_name);
                    } else {
                        Logger::instance().error("NETWORK",
                            "Network recovery FAILED for " + net.interface_name);
                    }
                }
            }
        }

        // --- 6. Critical Service ---
        {
            ServiceInfo svc = service_monitor.check();
            if (svc.is_healthy) {
                Logger::instance().info("SERVICE",
                    svc.service_name + " service is " + svc.state_string);
            } else {
                Logger::instance().critical("SERVICE",
                    svc.service_name + " service is " + svc.state_string);

                // Attempt recovery
                Logger::instance().info("SERVICE",
                    "Attempting to restart " + svc.service_name + "...");
                ServiceInfo recovery = service_monitor.attempt_recovery();
                if (recovery.recovery_succeeded) {
                    Logger::instance().info("SERVICE",
                        svc.service_name + " service recovery SUCCEEDED");
                } else {
                    Logger::instance().error("SERVICE",
                        svc.service_name + " service recovery FAILED. State: "
                        + recovery.state_string);
                }
            }
        }

        // --- Sleep for configured interval (interruptible by signal) ---
        for (int i = 0; i < config.check_interval_seconds() && g_running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    Logger::instance().info("MAIN", "=== Device Health Monitor Shutting Down ===");
    return 0;
}
