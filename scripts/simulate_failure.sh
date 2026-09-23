#!/bin/bash
# ============================================================================
# simulate_failure.sh - Safely simulate failures for testing auto-recovery
# ============================================================================
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Read the critical service from config, default to ssh
CONFIG_FILE="/etc/device-health-monitor/health_monitor.json"
PROJECT_CONFIG="$(cd "$(dirname "$0")/.." && pwd)/config/health_monitor.json"

CRITICAL_SERVICE="ssh"
if [[ -f "$CONFIG_FILE" ]]; then
    CRITICAL_SERVICE=$(grep -oP '"critical_service"\s*:\s*"\K[^"]+' "$CONFIG_FILE" 2>/dev/null || echo "ssh")
elif [[ -f "$PROJECT_CONFIG" ]]; then
    CRITICAL_SERVICE=$(grep -oP '"critical_service"\s*:\s*"\K[^"]+' "$PROJECT_CONFIG" 2>/dev/null || echo "ssh")
fi

echo -e "${CYAN}=== Device Health Monitor - Failure Simulator ===${NC}"
echo ""
echo -e "This script safely simulates failures to test auto-recovery."
echo -e "The health monitor should detect these failures and attempt recovery."
echo ""
echo -e "Available simulations:"
echo -e "  ${YELLOW}1${NC} - Stop critical service (${CRITICAL_SERVICE})"
echo -e "  ${YELLOW}2${NC} - Simulate high CPU load (30 seconds)"
echo -e "  ${YELLOW}3${NC} - Simulate high memory usage (30 seconds)"
echo -e "  ${YELLOW}4${NC} - Simulate network interface down (requires root)"
echo -e "  ${YELLOW}5${NC} - Run all simulations sequentially"
echo -e "  ${YELLOW}0${NC} - Exit"
echo ""

read -p "Select simulation [0-5]: " choice

case $choice in
    1)
        echo -e "${YELLOW}[Simulation 1] Stopping ${CRITICAL_SERVICE} service...${NC}"
        echo -e "The health monitor should detect this and restart the service."
        if [[ $EUID -ne 0 ]]; then
            echo -e "${RED}Error: Requires root. Use: sudo $0${NC}"
            exit 1
        fi
        systemctl stop "${CRITICAL_SERVICE}" 2>/dev/null || {
            echo -e "${RED}Failed to stop ${CRITICAL_SERVICE}. Is it installed?${NC}"
            exit 1
        }
        echo -e "${GREEN}${CRITICAL_SERVICE} service stopped.${NC}"
        echo -e "Watch the health monitor logs:"
        echo -e "  ${CYAN}sudo journalctl -u device-health-monitor -f${NC}"
        echo -e "  or: ${CYAN}tail -f /var/log/device-health-monitor.log${NC}"
        ;;

    2)
        echo -e "${YELLOW}[Simulation 2] Generating high CPU load for 30 seconds...${NC}"
        echo -e "Starting CPU stress on all cores..."
        CORES=$(nproc 2>/dev/null || echo 2)
        for i in $(seq 1 "$CORES"); do
            # Infinite loop in background subshell - burns CPU
            (while true; do :; done) &
            PIDS+=($!)
        done
        echo -e "${GREEN}CPU stress started (PIDs: ${PIDS[*]}).${NC}"
        echo -e "Waiting 30 seconds..."
        sleep 30
        for pid in "${PIDS[@]}"; do
            kill "$pid" 2>/dev/null || true
        done
        echo -e "${GREEN}CPU stress stopped.${NC}"
        ;;

    3)
        echo -e "${YELLOW}[Simulation 3] Simulating high memory usage for 30 seconds...${NC}"
        echo -e "Allocating ~512MB of memory..."
        # Use Python or dd to allocate memory temporarily
        python3 -c "
import time
data = bytearray(512 * 1024 * 1024)  # 512MB
print('Allocated 512MB. Holding for 30 seconds...')
time.sleep(30)
print('Released.')
" 2>/dev/null || {
            echo -e "${YELLOW}Python3 not available. Using dd instead...${NC}"
            # Fallback: read from /dev/zero into a tmpfs file
            dd if=/dev/zero of=/dev/shm/health_monitor_test bs=1M count=512 2>/dev/null
            echo -e "Holding for 30 seconds..."
            sleep 30
            rm -f /dev/shm/health_monitor_test
            echo -e "${GREEN}Memory released.${NC}"
        }
        ;;

    4)
        echo -e "${YELLOW}[Simulation 4] Taking network interface down temporarily...${NC}"
        if [[ $EUID -ne 0 ]]; then
            echo -e "${RED}Error: Requires root. Use: sudo $0${NC}"
            exit 1
        fi
        IFACE=$(grep -oP '"network_interface"\s*:\s*"\K[^"]+' "$CONFIG_FILE" 2>/dev/null || echo "eth0")
        echo -e "Bringing down interface: ${IFACE}"
        echo -e "${RED}WARNING: This will temporarily disconnect the network!${NC}"
        read -p "Continue? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            ip link set "$IFACE" down
            echo -e "${GREEN}Interface ${IFACE} is DOWN.${NC}"
            echo -e "The health monitor should detect and attempt recovery."
            echo -e "Waiting 20 seconds before restoring..."
            sleep 20
            ip link set "$IFACE" up
            echo -e "${GREEN}Interface ${IFACE} restored to UP.${NC}"
        else
            echo -e "${YELLOW}Cancelled.${NC}"
        fi
        ;;

    5)
        echo -e "${YELLOW}Running all simulations sequentially...${NC}"
        echo -e "Note: Network simulation will be skipped (requires manual confirmation)."
        echo ""
        # Service
        if [[ $EUID -eq 0 ]]; then
            echo -e "${CYAN}--- Service Simulation ---${NC}"
            systemctl stop "${CRITICAL_SERVICE}" 2>/dev/null || true
            echo -e "Stopped ${CRITICAL_SERVICE}. Waiting 15 seconds for recovery..."
            sleep 15
        fi
        # CPU
        echo -e "${CYAN}--- CPU Simulation ---${NC}"
        CORES=$(nproc 2>/dev/null || echo 2)
        PIDS=()
        for i in $(seq 1 "$CORES"); do
            (while true; do :; done) &
            PIDS+=($!)
        done
        echo -e "CPU stress started. Waiting 15 seconds..."
        sleep 15
        for pid in "${PIDS[@]}"; do
            kill "$pid" 2>/dev/null || true
        done
        echo -e "${GREEN}All simulations complete.${NC}"
        ;;

    0)
        echo -e "${GREEN}Exiting.${NC}"
        exit 0
        ;;

    *)
        echo -e "${RED}Invalid choice.${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Simulation finished.${NC}"
echo -e "Check health monitor logs: ${CYAN}sudo journalctl -u device-health-monitor -f${NC}"
