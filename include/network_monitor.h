#pragma once

#include <string>

enum class NetworkState {
    CONNECTED,
    DISCONNECTED,
    INTERFACE_DOWN,
    INTERFACE_NOT_FOUND,
    UNKNOWN
};

struct NetworkInfo {
    NetworkState state = NetworkState::UNKNOWN;
    std::string interface_name;
    std::string state_string;
    bool is_healthy = false;
};

class NetworkMonitor {
public:
    NetworkMonitor() = default;

    void set_interface(const std::string& iface);
    NetworkInfo check();

    /// Attempt network recovery (restart interface)
    bool attempt_recovery();

private:
    std::string read_operstate() const;
    std::string interface_ = "eth0";
};
