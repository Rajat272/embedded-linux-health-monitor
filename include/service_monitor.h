#pragma once

#include <string>

enum class ServiceState {
    RUNNING,
    STOPPED,
    FAILED,
    UNKNOWN
};

struct ServiceInfo {
    ServiceState state = ServiceState::UNKNOWN;
    std::string service_name;
    std::string state_string;
    bool is_healthy = false;
    bool recovery_attempted = false;
    bool recovery_succeeded = false;
};

class ServiceMonitor {
public:
    ServiceMonitor() = default;

    void set_service(const std::string& service_name);
    void set_recovery_wait(int seconds);

    /// Check if the critical service is running
    ServiceInfo check();

    /// Attempt to restart the service and verify recovery
    ServiceInfo attempt_recovery();

    /// Check service status via systemctl (public for testing)
    ServiceState get_service_state() const;

protected:
    /// Virtual for testing - execute a command and return exit code
    virtual int execute_command(const std::string& cmd) const;
    /// Virtual for testing - get systemctl output
    virtual std::string get_systemctl_status() const;

private:
    std::string service_name_ = "ssh";
    int recovery_wait_seconds_ = 5;
};
