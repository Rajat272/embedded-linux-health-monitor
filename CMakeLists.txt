cmake_minimum_required(VERSION 3.10)
project(device-health-monitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)

# ============================================================================
# Main executable
# ============================================================================
add_executable(device-health-monitor
    src/main.cpp
    src/logger.cpp
    src/config_manager.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/disk_monitor.cpp
    src/temperature_monitor.cpp
    src/network_monitor.cpp
    src/service_monitor.cpp
)

target_include_directories(device-health-monitor PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(device-health-monitor PRIVATE pthread)

# Install target
install(TARGETS device-health-monitor DESTINATION bin)

# ============================================================================
# Tests (optional: enable with -DBUILD_TESTS=ON)
# ============================================================================
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Prevent GoogleTest from overriding compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # --- Helper library: monitor sources without main ---
    add_library(monitor_lib STATIC
        src/logger.cpp
        src/config_manager.cpp
        src/cpu_monitor.cpp
        src/memory_monitor.cpp
        src/disk_monitor.cpp
        src/temperature_monitor.cpp
        src/network_monitor.cpp
        src/service_monitor.cpp
    )
    target_include_directories(monitor_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(monitor_lib PUBLIC pthread)

    # --- Test: CPU ---
    add_executable(test_cpu tests/test_cpu.cpp)
    target_link_libraries(test_cpu PRIVATE monitor_lib GTest::gtest_main)
    add_test(NAME CpuMonitorTests COMMAND test_cpu)

    # --- Test: Memory ---
    add_executable(test_memory tests/test_memory.cpp)
    target_link_libraries(test_memory PRIVATE monitor_lib GTest::gtest_main)
    add_test(NAME MemoryMonitorTests COMMAND test_memory)

    # --- Test: Config ---
    add_executable(test_config tests/test_config.cpp)
    target_link_libraries(test_config PRIVATE monitor_lib GTest::gtest_main)
    add_test(NAME ConfigManagerTests COMMAND test_config)

    # --- Test: Service Monitor ---
    add_executable(test_service_monitor tests/test_service_monitor.cpp)
    target_link_libraries(test_service_monitor PRIVATE monitor_lib GTest::gtest_main)
    add_test(NAME ServiceMonitorTests COMMAND test_service_monitor)
endif()
