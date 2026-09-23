# Embedded Linux Device Health Monitor & Auto-Recovery Agent

A lightweight C++17 daemon that continuously monitors Linux system health across 6 key areas and automatically recovers from failures. Designed for embedded Linux devices.

## Features

- **CPU Monitoring** — Reads `/proc/stat`, calculates real usage percentage with two-sample delta
- **Memory Monitoring** — Reads `/proc/meminfo`, prefers `MemAvailable` for accuracy
- **Disk Monitoring** — Uses POSIX `statvfs()` for filesystem utilization
- **Temperature Monitoring** — Discovers thermal zones under `/sys/class/thermal/`, handles missing sensors
- **Network Monitoring** — Reads `/sys/class/net/<iface>/operstate`, attempts interface recovery
- **Service Monitoring** — Monitors any configurable systemd service, auto-restarts on failure
- **Auto-Recovery** — Conservative, safe recovery actions with verification
- **systemd Integration** — Runs as a background service with watchdog and auto-restart
- **JSON Configuration** — All thresholds and settings configurable
- **Thread-safe Logging** — Timestamped, leveled logging to file and journal
- **Unit Tests** — GoogleTest-based tests with mock data injection

## Architecture Overview

```
                    +----------------------+
                    |       main.cpp       |
                    | Monitoring Controller|
                    +----------+-----------+
                               |
             +-----------------+------------------+
             |        |        |        |         |
             v        v        v        v         v
           CPU      Memory    Disk     Temp     Network
         Monitor   Monitor  Monitor  Monitor   Monitor
                                                  |
                                          Service Monitor
                                                  |
                                          Recovery Actions
                                                  |
                                               Logger
```

## Requirements

- Linux (kernel 3.x+)
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.10+
- systemd (for service management and service monitor)

## Dependencies

- **No external runtime dependencies** — reads Linux interfaces directly
- **Build-time only**: CMake, C++ compiler
- **Tests only**: GoogleTest (auto-fetched via CMake FetchContent)

## Build Instructions

### Quick Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Build with Tests

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual tests:

```bash
./test_cpu
./test_memory
./test_config
./test_service_monitor
```

## Installation (systemd Service)

### Automated Install

```bash
sudo bash scripts/install.sh
```

This will:
1. Build the project
2. Install binary to `/usr/local/bin/`
3. Install config to `/etc/device-health-monitor/`
4. Install and enable the systemd service
5. Start the monitor

### Manual Install

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Install
sudo cp device-health-monitor /usr/local/bin/
sudo mkdir -p /etc/device-health-monitor
sudo cp ../config/health_monitor.json /etc/device-health-monitor/
sudo cp ../systemd/device-health-monitor.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable device-health-monitor
sudo systemctl start device-health-monitor
```

### Uninstall

```bash
sudo bash scripts/uninstall.sh
```

## Configuration

Edit `/etc/device-health-monitor/health_monitor.json`:

```json
{
    "check_interval_seconds": 10,
    "cpu_threshold": 90,
    "memory_threshold": 90,
    "disk_threshold": 90,
    "temperature_threshold": 80,
    "disk_path": "/",
    "network_interface": "eth0",
    "critical_service": "ssh",
    "log_file": "/var/log/device-health-monitor.log",
    "recovery_wait_seconds": 5
}
```

| Parameter | Default | Description |
|---|---|---|
| `check_interval_seconds` | 10 | Seconds between monitoring cycles |
| `cpu_threshold` | 90 | CPU usage % to trigger warning |
| `memory_threshold` | 90 | Memory usage % to trigger warning |
| `disk_threshold` | 90 | Disk usage % to trigger warning |
| `temperature_threshold` | 80 | Temperature °C to trigger critical |
| `disk_path` | `/` | Filesystem path to monitor |
| `network_interface` | `eth0` | Network interface to monitor |
| `critical_service` | `ssh` | systemd service to monitor/recover |
| `log_file` | `/var/log/device-health-monitor.log` | Log file path |
| `recovery_wait_seconds` | 5 | Seconds to wait after recovery attempt |

### Run with Custom Config

```bash
./device-health-monitor /path/to/custom/config.json
```

## Checking Service Status

```bash
sudo systemctl status device-health-monitor
```

## Viewing Logs

```bash
# Via journald (recommended)
sudo journalctl -u device-health-monitor -f

# Via log file
tail -f /var/log/device-health-monitor.log
```

### Example Log Output

```
[2026-09-23 19:30:10] [INFO] [MAIN] === Device Health Monitor Starting ===
[2026-09-23 19:30:10] [INFO] [CPU] CPU usage: 23.4%
[2026-09-23 19:30:10] [INFO] [MEMORY] Memory usage: 45.2% (used: 3670 MB / total: 8123 MB)
[2026-09-23 19:30:10] [INFO] [DISK] Disk usage (/): 62.1% (used: 28.4 GB / total: 45.7 GB)
[2026-09-23 19:30:10] [INFO] [TEMPERATURE] Temperature (x86_pkg_temp): 52.0°C
[2026-09-23 19:30:10] [INFO] [NETWORK] Interface eth0: connected
[2026-09-23 19:30:10] [INFO] [SERVICE] ssh service is running
[2026-09-23 19:30:20] [CRITICAL] [SERVICE] ssh service is stopped
[2026-09-23 19:30:20] [INFO] [SERVICE] Attempting to restart ssh...
[2026-09-23 19:30:25] [INFO] [SERVICE] ssh service recovery SUCCEEDED
```

## Simulate Failures

```bash
sudo bash scripts/simulate_failure.sh
```

Available simulations:
1. **Stop critical service** — Monitor should detect and restart it
2. **High CPU load** — 30 seconds of CPU stress
3. **High memory usage** — Temporary 512MB allocation
4. **Network interface down** — Temporarily takes interface down

## Troubleshooting

| Problem | Solution |
|---|---|
| "No thermal sensor available" | Normal on VMs/containers without thermal zones |
| "Interface not found" | Change `network_interface` in config to your actual interface (`ip link show`) |
| Service monitor shows "unknown" | Verify the configured service exists: `systemctl list-units \| grep <service>` |
| Permission denied on log file | Run as root or adjust log file path |
| Build fails on /proc/stat | Project requires Linux; won't compile on macOS/Windows |

## Project Structure

```
embedded-linux-health-monitor/
├── README.md                              # This file
├── CMakeLists.txt                         # Build configuration
├── .gitignore
├── config/
│   └── health_monitor.json                # Default configuration
├── include/
│   ├── cpu_monitor.h                      # CPU monitor interface
│   ├── memory_monitor.h                   # Memory monitor interface
│   ├── disk_monitor.h                     # Disk monitor interface
│   ├── temperature_monitor.h              # Temperature monitor interface
│   ├── network_monitor.h                  # Network monitor interface
│   ├── service_monitor.h                  # Service monitor interface
│   ├── logger.h                           # Logger interface
│   └── config_manager.h                   # Configuration manager
├── src/
│   ├── main.cpp                           # Main monitoring loop
│   ├── cpu_monitor.cpp                    # CPU usage from /proc/stat
│   ├── memory_monitor.cpp                 # RAM usage from /proc/meminfo
│   ├── disk_monitor.cpp                   # Disk usage via statvfs()
│   ├── temperature_monitor.cpp            # Thermal zone discovery
│   ├── network_monitor.cpp                # Network interface state
│   ├── service_monitor.cpp                # systemd service monitoring
│   ├── logger.cpp                         # Thread-safe logging
│   └── config_manager.cpp                 # JSON config parser
├── systemd/
│   └── device-health-monitor.service      # systemd unit file
├── scripts/
│   ├── install.sh                         # Automated installer
│   ├── uninstall.sh                       # Automated uninstaller
│   └── simulate_failure.sh                # Failure simulation tool
├── tests/
│   ├── test_cpu.cpp                       # CPU monitor tests
│   ├── test_memory.cpp                    # Memory monitor tests
│   ├── test_config.cpp                    # Config manager tests
│   └── test_service_monitor.cpp           # Service monitor tests
└── docs/
    ├── architecture.md                    # Architecture documentation
    └── test-report.md                     # Test results report
```

## License

This project is provided for educational and operational use on embedded Linux devices.
