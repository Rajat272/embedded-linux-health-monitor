#include "service_monitor.h"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <thread>
#include <chrono>
#include <sstream>

void ServiceMonitor::set_service(const std::string& service_name) {
    service_name_ = service_name;
}

void ServiceMonitor::set_recovery_wait(int seconds) {
    recovery_wait_seconds_ = seconds;
}

int ServiceMonitor::execute_command(const std::string& cmd) const {
    int ret = system(cmd.c_str());
    return WEXITSTATUS(ret);
}

std::string ServiceMonitor::get_systemctl_status() const {
    std::string cmd = "systemctl is-active " + service_name_ + " 2>/dev/null";
    std::array<char, 256> buffer{};
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "unknown";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    // Trim whitespace
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }
    return result;
}

ServiceState ServiceMonitor::get_service_state() const {
    std::string status = get_systemctl_status();
    if (status == "active") return ServiceState::RUNNING;
    if (status == "inactive") return ServiceState::STOPPED;
    if (status == "failed") return ServiceState::FAILED;
    return ServiceState::UNKNOWN;
}

ServiceInfo ServiceMonitor::check() {
    ServiceInfo info;
    info.service_name = service_name_;

    ServiceState state = get_service_state();
    info.state = state;

    switch (state) {
        case ServiceState::RUNNING:
            info.state_string = "running";
            info.is_healthy = true;
            break;
        case ServiceState::STOPPED:
            info.state_string = "stopped";
            info.is_healthy = false;
            break;
        case ServiceState::FAILED:
            info.state_string = "failed";
            info.is_healthy = false;
            break;
        case ServiceState::UNKNOWN:
            info.state_string = "unknown";
            info.is_healthy = false;
            break;
    }

    return info;
}

ServiceInfo ServiceMonitor::attempt_recovery() {
    ServiceInfo info;
    info.service_name = service_name_;
    info.recovery_attempted = true;

    // Try to restart the service via systemctl
    std::string restart_cmd = "systemctl restart " + service_name_ + " 2>/dev/null";
    execute_command(restart_cmd);

    // Wait for the configured recovery period
    std::this_thread::sleep_for(std::chrono::seconds(recovery_wait_seconds_));

    // Verify recovery
    ServiceState new_state = get_service_state();
    info.state = new_state;

    if (new_state == ServiceState::RUNNING) {
        info.state_string = "running";
        info.is_healthy = true;
        info.recovery_succeeded = true;
    } else {
        info.state_string = (new_state == ServiceState::FAILED) ? "failed" : "not running";
        info.is_healthy = false;
        info.recovery_succeeded = false;
    }

    return info;
}
