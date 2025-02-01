#!/bin/bash

# Git root directory
GIT_ROOT=$(git rev-parse --show-toplevel)

# ANSI escape codes for colors and formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Logging directory
DEV_OPS_LOGS_DIR="${GIT_ROOT}/devops/logs/"
mkdir -p "${DEV_OPS_LOGS_DIR}"

# Logging function
log() {
    local level=$1
    local message=$2
    local timestamp=$(date +"%H:%M:%S")

    # Log to stdout
    case $level in
        "info")
            echo -e "${BLUE}[${timestamp} INFO]${NC} ${message}"
            ;;
        "success")
            echo -e "${GREEN}[$timestamp SUCCESS]${NC} ${message}"
            ;;
        "warning")
            echo -e "${YELLOW}[$timestamp WARNING]${NC} ${message}"
            ;;
        "error")
            echo -e "${RED}[$timestamp ${BOLD}ERROR${NC}${RED}]${NC} ${message}"
            ;;
        "debug")
            echo -e "${YELLOW}[${timestamp} DEBUG]${NC} ${message}"
            ;;
        *)
            echo -e "${BOLD}[$timestamp LOG]${NC} ${message}"
            ;;
    esac

    # Log to file
    local log_file="${DEV_OPS_LOGS_DIR}/script.log"
    echo "[${timestamp} ${level^^}] ${message}" >> "$log_file"
}

# Helper function to log and execute a command
run_command() {
    local message=$1
    local command=$2

    log "info" "$message"
    eval "$command" || exit_with_error "Failed to execute: $command"
}

# Function to handle SIGINT (Ctrl+C)
handle_sigint() {
    log "debug" "Script interrupted by user (Ctrl+C)."
    exit 1
}

# Set up the trap for SIGINT
trap handle_sigint SIGINT

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to exit with an error message
exit_with_error() {
    log "error" "$1"
    exit 1
}