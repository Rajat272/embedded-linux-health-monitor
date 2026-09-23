# Test Report — Device Health Monitor

## Build Environment

| Item | Value |
|---|---|
| OS | Linux (tested on Ubuntu 22.04+/Debian 12+) |
| Compiler | GCC 11+ / Clang 14+ |
| C++ Standard | C++17 |
| Build System | CMake 3.10+ |
| Test Framework | GoogleTest v1.14.0 (fetched via CMake FetchContent) |

## Build Process

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --parallel $(nproc)
```

## Test Execution

```bash
cd build
ctest --output-on-failure
```

---

## Unit Test Results

### CPU Monitor Tests (`test_cpu`)

| # | Test Case | Description | Expected | Status |
|---|---|---|---|---|
| 1 | `ParseValidProcStat` | Parse a valid /proc/stat cpu line | All fields correctly parsed | ⏳ Pending |
| 2 | `ParseEmptyProcStat` | Parse empty string | Returns false | ⏳ Pending |
| 3 | `ParseInvalidProcStat` | Parse garbage data | Returns false | ⏳ Pending |
| 4 | `CalculateUsage` | Calculate CPU % from two snapshots | 75.0% ± 0.1 | ⏳ Pending |
| 5 | `CalculateUsageZeroDiff` | Same stats → 0% | 0.0% | ⏳ Pending |
| 6 | `FirstMeasurementIsInvalid` | First call returns valid=false | valid=false | ⏳ Pending |
| 7 | `SecondMeasurementIsValid` | Second call returns real data | valid=true, 0-100% | ⏳ Pending |
| 8 | `HighCpuDetection` | 90% CPU with 50% threshold | threshold_exceeded=true | ⏳ Pending |
| 9 | `NormalCpuBelowThreshold` | 10% CPU with 90% threshold | threshold_exceeded=false | ⏳ Pending |

### Memory Monitor Tests (`test_memory`)

| # | Test Case | Description | Expected | Status |
|---|---|---|---|---|
| 1 | `ParseFullMeminfo` | Parse with MemAvailable present | Uses MemAvailable | ⏳ Pending |
| 2 | `ParseMeminfoFallbackToFree` | Parse without MemAvailable | Falls back to MemFree | ⏳ Pending |
| 3 | `ParseEmptyMeminfo` | Empty string | Returns false | ⏳ Pending |
| 4 | `ParseMissingTotal` | Missing MemTotal | Returns false | ⏳ Pending |
| 5 | `UtilizationCalculation` | 50% memory usage | 50.0% ± 0.1 | ⏳ Pending |
| 6 | `ThresholdExceeded` | 95% usage with 80% threshold | threshold_exceeded=true | ⏳ Pending |
| 7 | `ThresholdNotExceeded` | 30% usage with 90% threshold | threshold_exceeded=false | ⏳ Pending |
| 8 | `InvalidDataReturnsInvalid` | Garbage data | valid=false | ⏳ Pending |

### Configuration Manager Tests (`test_config`)

| # | Test Case | Description | Expected | Status |
|---|---|---|---|---|
| 1 | `ValidConfig` | Load complete valid config | All values match JSON | ⏳ Pending |
| 2 | `MissingConfigUsesDefaults` | Non-existent file | Returns false, defaults active | ⏳ Pending |
| 3 | `InvalidJsonUsesDefaults` | Invalid JSON content | Returns false, defaults active | ⏳ Pending |
| 4 | `PartialConfig` | Only some fields specified | Specified fields set, others default | ⏳ Pending |
| 5 | `OutOfRangeValues` | Values outside valid ranges | Rejected, defaults used | ⏳ Pending |
| 6 | `DefaultValues` | No config loaded | All defaults correct | ⏳ Pending |
| 7 | `EmptyJsonObject` | Empty `{}` | All defaults correct | ⏳ Pending |

### Service Monitor Tests (`test_service_monitor`)

| # | Test Case | Description | Expected | Status |
|---|---|---|---|---|
| 1 | `RunningServiceIsHealthy` | Mock returns "active" | is_healthy=true, RUNNING | ⏳ Pending |
| 2 | `StoppedServiceIsUnhealthy` | Mock returns "inactive" | is_healthy=false, STOPPED | ⏳ Pending |
| 3 | `FailedServiceDetected` | Mock returns "failed" | is_healthy=false, FAILED | ⏳ Pending |
| 4 | `UnknownStatus` | Mock returns unexpected | is_healthy=false, UNKNOWN | ⏳ Pending |
| 5 | `SuccessfulRecovery` | Recovery changes state to active | recovery_succeeded=true | ⏳ Pending |
| 6 | `FailedRecovery` | Recovery fails | recovery_succeeded=false | ⏳ Pending |
| 7 | `RecoveryServiceRemainsDown` | Restart succeeds but service stays down | recovery_succeeded=false | ⏳ Pending |

---

## Integration / Manual Testing

### Failure Simulation Tests

| # | Test | Method | Expected Behavior | Status |
|---|---|---|---|---|
| 1 | Service Recovery | `sudo systemctl stop ssh` | Monitor detects, restarts, verifies, logs | ⏳ Pending |
| 2 | CPU Monitoring | CPU stress script | Monitor reports high CPU, logs warning | ⏳ Pending |
| 3 | Memory Monitoring | Memory allocation script | Monitor reports high memory, logs warning | ⏳ Pending |
| 4 | Network Recovery | `sudo ip link set eth0 down` | Monitor detects, attempts recovery, logs | ⏳ Pending |
| 5 | Temperature | Read from thermal zone | Monitor reports current temperature | ⏳ Pending |
| 6 | Disk Monitoring | Check root filesystem | Monitor reports disk utilization | ⏳ Pending |
| 7 | Graceful Shutdown | `sudo systemctl stop device-health-monitor` | Clean shutdown message logged | ⏳ Pending |

### systemd Service Tests

| # | Test | Command | Expected | Status |
|---|---|---|---|---|
| 1 | Service starts | `sudo systemctl start device-health-monitor` | Active (running) | ⏳ Pending |
| 2 | Service auto-restarts | Kill process, check status after 15s | Service restarts | ⏳ Pending |
| 3 | Service enables | `sudo systemctl enable device-health-monitor` | Enabled | ⏳ Pending |
| 4 | Logs to journal | `journalctl -u device-health-monitor` | Log entries visible | ⏳ Pending |

---

## Notes

- All unit tests use mock data injection (virtual method overrides) and do **not** depend on actual machine state
- Tests marked ⏳ Pending should be run on a Linux system after building with `-DBUILD_TESTS=ON`
- Integration tests require root privileges and a running systemd
- Update this document with actual PASS/FAIL results after execution
