# Architecture — Device Health Monitor

## Overview

The Device Health Monitor is a modular C++17 daemon that monitors 6 key Linux system health metrics, detects abnormal conditions, performs safe automatic recovery where appropriate, and logs all events.

## System Architecture

```
                    ┌──────────────────────┐
                    │       main.cpp       │
                    │  Monitoring Controller│
                    │  (Signal handling,   │
                    │   Main loop,         │
                    │   Recovery logic)    │
                    └──────────┬───────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │         │          │          │          │
          ▼         ▼          ▼          ▼          ▼
     ┌─────────┐┌────────┐┌────────┐┌────────┐┌─────────┐
     │  CPU    ││ Memory ││  Disk  ││  Temp  ││ Network │
     │ Monitor ││ Monitor││ Monitor││ Monitor││ Monitor │
     └─────────┘└────────┘└────────┘└────────┘└────┬────┘
                                                    │
                                              ┌─────┴─────┐
                                              │  Service   │
                                              │  Monitor   │
                                              └─────┬──────┘
                                                    │
                                              ┌─────┴──────┐
                                              │  Recovery   │
                                              │  Actions    │
                                              └─────┬──────┘
                                                    │
                ┌───────────────────────────────────┘
                ▼
          ┌───────────┐          ┌─────────────────┐
          │  Logger   │◄─────── │  ConfigManager   │
          │(Singleton)│          │  (JSON config)   │
          └───────────┘          └─────────────────┘
```

## Components

### 1. Main Controller (`main.cpp`)

**Responsibilities:**
- Initialize all components from configuration
- Install signal handlers (SIGTERM, SIGINT) for graceful shutdown
- Execute the monitoring loop on a configurable interval
- Evaluate health results and trigger recovery
- Coordinate logging

**Monitoring Cycle:**
```
Load Config → Init Logger → Init Monitors → Loop {
    Check CPU → Check Memory → Check Disk → Check Temp →
    Check Network → Check Service →
    Evaluate & Recover → Log → Sleep
} → Graceful Shutdown
```

### 2. CPU Monitor (`cpu_monitor.h/cpp`)

**Data Source:** `/proc/stat`

**Method:**
- Reads aggregate `cpu` line from `/proc/stat`
- Parses user, nice, system, idle, iowait, irq, softirq, steal fields
- Computes delta between two consecutive samples
- Usage = `(total_diff - idle_diff) / total_diff * 100`
- First measurement returns `valid=false` to avoid false 100% reading

**Recovery:** Logging only (no process killing)

### 3. Memory Monitor (`memory_monitor.h/cpp`)

**Data Source:** `/proc/meminfo`

**Method:**
- Parses `MemTotal`, `MemAvailable`, `MemFree`
- Prefers `MemAvailable` over `MemFree` (more accurate)
- Usage = `(total - available) / total * 100`

**Recovery:** Logging only (no OOM killing)

### 4. Disk Monitor (`disk_monitor.h/cpp`)

**Data Source:** POSIX `statvfs()` API

**Method:**
- Calls `statvfs()` on the configured mount path
- Calculates total, used, available bytes
- Usage matches `df` output: `used / (used + available) * 100`

**Recovery:** Logging only (no file deletion)

### 5. Temperature Monitor (`temperature_monitor.h/cpp`)

**Data Source:** `/sys/class/thermal/thermal_zone*/temp`

**Method:**
- Discovers available thermal zones by scanning `/sys/class/thermal/`
- Reads temperature in millidegrees Celsius, converts to °C
- Caches discovered zone path for efficiency
- Handles missing sensors gracefully

**Recovery:** Logging only (no unsafe shutdown actions)

### 6. Network Monitor (`network_monitor.h/cpp`)

**Data Source:** `/sys/class/net/<interface>/operstate`

**Method:**
- Checks if interface directory exists in `/sys/class/net/`
- Reads `operstate` file (values: up, down, unknown, etc.)
- Maps to NetworkState enum

**Recovery:** Attempts `ip link set <iface> up` if interface is down

### 7. Service Monitor (`service_monitor.h/cpp`)

**Data Source:** `systemctl is-active <service>`

**Method:**
- Queries service state via systemctl
- Maps "active", "inactive", "failed" to ServiceState enum

**Recovery:**
1. Runs `systemctl restart <service>`
2. Waits for configured recovery period
3. Re-checks service state
4. Logs success or failure

### 8. Logger (`logger.h/cpp`)

**Type:** Thread-safe singleton

**Output:** Dual output to log file + stderr (captured by journald for systemd services)

**Format:** `[YYYY-MM-DD HH:MM:SS] [LEVEL] [COMPONENT] Message`

**Levels:** INFO, WARNING, ERROR, CRITICAL

### 9. ConfigManager (`config_manager.h/cpp`)

**Format:** JSON configuration file

**Features:**
- Built-in minimal JSON parser (no external dependency)
- Validates ranges for all numeric values
- Falls back to sensible defaults for missing/invalid values
- Config path configurable via command-line argument

## Data Flow

```
/proc/stat ──────────► CpuMonitor ──────► CpuInfo
/proc/meminfo ───────► MemoryMonitor ───► MemoryInfo
statvfs("/") ────────► DiskMonitor ─────► DiskInfo
/sys/class/thermal/ ─► TempMonitor ─────► TemperatureInfo    ──► main.cpp
/sys/class/net/ ─────► NetworkMonitor ──► NetworkInfo              │
systemctl ───────────► ServiceMonitor ──► ServiceInfo              │
                                                                    ▼
config.json ─────────► ConfigManager                          Logger ──► file + stderr
```

## Failure Detection & Recovery Flow

```
Monitor.check()
    │
    ├── valid=true, threshold_exceeded=false → INFO log, continue
    │
    ├── valid=true, threshold_exceeded=true
    │       │
    │       ├── CPU/Memory/Disk/Temp → WARNING/CRITICAL log only
    │       │
    │       ├── Network (down) → attempt_recovery() → re-check → log result
    │       │
    │       └── Service (stopped/failed) → attempt_recovery() → verify → log result
    │
    └── valid=false → ERROR log, continue monitoring
```

## Threading Model

Single-threaded event loop. The main thread runs all monitors sequentially in each cycle. The logger uses a mutex for thread safety in case of future multi-threading expansion.

## Signal Handling

- `SIGTERM` / `SIGINT` → Sets atomic flag → Main loop exits cleanly → Shutdown message logged

## Security Considerations

- Runs as root (required for service restarts and network recovery)
- systemd hardening: `ProtectSystem=strict`, `ProtectHome=yes`, `PrivateTmp=yes`
- No destructive recovery actions (no process killing, file deletion, or reboots)
- Recovery actions are conservative and logged
