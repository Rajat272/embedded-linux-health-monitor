#include "network_monitor.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>

void NetworkMonitor::set_interface(const std::string& iface) {
    interface_ = iface;
}

std::string NetworkMonitor::read_operstate() const {
    // Read /sys/class/net/<interface>/operstate
    std::string path = "/sys/class/net/" + interface_ + "/operstate";
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string state;
    std::getline(file, state);
    // Trim whitespace/newlines
    while (!state.empty() && (state.back() == '\n' || state.back() == '\r' || state.back() == ' ')) {
        state.pop_back();
    }
    return state;
}

NetworkInfo NetworkMonitor::check() {
    NetworkInfo info;
    info.interface_name = interface_;

    // Check if the interface directory exists in /sys/class/net/
    std::string iface_path = "/sys/class/net/" + interface_;
    struct stat st{};
    if (stat(iface_path.c_str(), &st) != 0) {
        info.state = NetworkState::INTERFACE_NOT_FOUND;
        info.state_string = "interface not found";
        info.is_healthy = false;
        return info;
    }

    // Read operational state
    std::string operstate = read_operstate();
    if (operstate.empty()) {
        info.state = NetworkState::UNKNOWN;
        info.state_string = "unknown (cannot read state)";
        info.is_healthy = false;
        return info;
    }

    // operstate values: "up", "down", "unknown", "dormant", "notpresent",
    //                   "lowerlayerdown", "testing"
    if (operstate == "up") {
        info.state = NetworkState::CONNECTED;
        info.state_string = "connected";
        info.is_healthy = true;
    } else if (operstate == "down" || operstate == "lowerlayerdown") {
        info.state = NetworkState::INTERFACE_DOWN;
        info.state_string = "interface down (" + operstate + ")";
        info.is_healthy = false;
    } else if (operstate == "unknown") {
        // "unknown" often means the interface is up but the driver doesn't
        // report carrier state (common for virtual/loopback interfaces).
        // Treat as connected since the interface is functional.
        info.state = NetworkState::CONNECTED;
        info.state_string = "connected (operstate=unknown)";
        info.is_healthy = true;
    } else {
        info.state = NetworkState::DISCONNECTED;
        info.state_string = operstate;
        info.is_healthy = false;
    }

    return info;
}

bool NetworkMonitor::attempt_recovery() {
    // Try to bring the interface back up using ip link set
    // This requires root privileges
    std::string cmd = "ip link set " + interface_ + " up 2>/dev/null";
    int ret = system(cmd.c_str());
    return (ret == 0);
}
