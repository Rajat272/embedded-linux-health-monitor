#!/bin/bash
# ============================================================================
# uninstall.sh - Remove Device Health Monitor
# ============================================================================
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

INSTALL_BIN="/usr/local/bin/device-health-monitor"
INSTALL_CONFIG_DIR="/etc/device-health-monitor"
SERVICE_FILE="/etc/systemd/system/device-health-monitor.service"
LOG_FILE="/var/log/device-health-monitor.log"

echo -e "${YELLOW}=== Device Health Monitor Uninstaller ===${NC}"

# --- Check root ---
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}Error: This script must be run as root (use sudo).${NC}"
    exit 1
fi

# --- Stop service ---
echo -e "${YELLOW}Stopping service...${NC}"
systemctl stop device-health-monitor.service 2>/dev/null || true
systemctl disable device-health-monitor.service 2>/dev/null || true
echo -e "${GREEN}Service stopped and disabled.${NC}"

# --- Remove service file ---
if [[ -f "$SERVICE_FILE" ]]; then
    rm -f "$SERVICE_FILE"
    echo -e "${GREEN}Removed ${SERVICE_FILE}${NC}"
fi

# --- Reload systemd ---
systemctl daemon-reload
echo -e "${GREEN}Systemd daemon reloaded.${NC}"

# --- Remove binary ---
if [[ -f "$INSTALL_BIN" ]]; then
    rm -f "$INSTALL_BIN"
    echo -e "${GREEN}Removed ${INSTALL_BIN}${NC}"
fi

# --- Remove configuration ---
if [[ -d "$INSTALL_CONFIG_DIR" ]]; then
    rm -rf "$INSTALL_CONFIG_DIR"
    echo -e "${GREEN}Removed ${INSTALL_CONFIG_DIR}${NC}"
fi

# --- Optionally remove log file ---
if [[ -f "$LOG_FILE" ]]; then
    read -p "Remove log file ${LOG_FILE}? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f "$LOG_FILE"
        echo -e "${GREEN}Removed ${LOG_FILE}${NC}"
    else
        echo -e "${YELLOW}Log file preserved.${NC}"
    fi
fi

echo -e "${GREEN}=== Uninstallation Complete ===${NC}"
