#!/bin/bash
# ============================================================================
# install.sh - Build and install Device Health Monitor
# ============================================================================
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

INSTALL_BIN="/usr/local/bin/device-health-monitor"
INSTALL_CONFIG_DIR="/etc/device-health-monitor"
INSTALL_CONFIG="${INSTALL_CONFIG_DIR}/health_monitor.json"
SERVICE_FILE="/etc/systemd/system/device-health-monitor.service"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo -e "${GREEN}=== Device Health Monitor Installer ===${NC}"

# --- Check OS ---
if [[ "$(uname -s)" != "Linux" ]]; then
    echo -e "${RED}Error: This installer only runs on Linux.${NC}"
    exit 1
fi

# --- Check root ---
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}Error: This script must be run as root (use sudo).${NC}"
    exit 1
fi

# --- Check dependencies ---
echo -e "${YELLOW}Checking dependencies...${NC}"
for cmd in cmake g++ make; do
    if ! command -v "$cmd" &>/dev/null; then
        echo -e "${RED}Error: '$cmd' is not installed. Please install it first.${NC}"
        exit 1
    fi
done
echo -e "${GREEN}All dependencies found.${NC}"

# --- Build ---
echo -e "${YELLOW}Building project...${NC}"
BUILD_DIR="${PROJECT_DIR}/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel "$(nproc)"
echo -e "${GREEN}Build completed successfully.${NC}"

# --- Install binary ---
echo -e "${YELLOW}Installing binary...${NC}"
install -m 0755 "${BUILD_DIR}/device-health-monitor" "$INSTALL_BIN"
echo -e "${GREEN}Binary installed to ${INSTALL_BIN}${NC}"

# --- Install configuration ---
echo -e "${YELLOW}Installing configuration...${NC}"
mkdir -p "$INSTALL_CONFIG_DIR"
if [[ ! -f "$INSTALL_CONFIG" ]]; then
    install -m 0644 "${PROJECT_DIR}/config/health_monitor.json" "$INSTALL_CONFIG"
    echo -e "${GREEN}Configuration installed to ${INSTALL_CONFIG}${NC}"
else
    echo -e "${YELLOW}Configuration already exists at ${INSTALL_CONFIG}. Skipping (not overwriting).${NC}"
fi

# --- Install systemd service ---
echo -e "${YELLOW}Installing systemd service...${NC}"
install -m 0644 "${PROJECT_DIR}/systemd/device-health-monitor.service" "$SERVICE_FILE"
echo -e "${GREEN}Service file installed to ${SERVICE_FILE}${NC}"

# --- Reload systemd and enable/start ---
echo -e "${YELLOW}Reloading systemd and enabling service...${NC}"
systemctl daemon-reload
systemctl enable device-health-monitor.service
systemctl start device-health-monitor.service

echo ""
echo -e "${GREEN}=== Installation Complete ===${NC}"
echo -e "Service status:"
systemctl status device-health-monitor.service --no-pager || true
echo ""
echo -e "Useful commands:"
echo -e "  ${YELLOW}sudo systemctl status device-health-monitor${NC}   - Check status"
echo -e "  ${YELLOW}sudo journalctl -u device-health-monitor -f${NC}  - View logs"
echo -e "  ${YELLOW}sudo systemctl restart device-health-monitor${NC} - Restart"
echo -e "  ${YELLOW}sudo systemctl stop device-health-monitor${NC}    - Stop"
