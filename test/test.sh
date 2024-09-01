#!/bin/bash
PROGRAM_NAME="Matt_daemon"
DAEMON_BINARY="./bin/Matt_daemon"
LOCK_FILE="/var/run/matt_daemon.lock"
LOG_DIR="/var/log/"
LOG_FILE="${LOG_DIR}/matt_daemon.log"
SOURCE_DIR="./src"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

cleanup() {
    # echo -e "${YELLOW}Cleaning up...${NC}"


    if pgrep -x "$DAEMON_BINARY" > /dev/null; then
        pkill -x "$DAEMON_BINARY"
    #     echo -e "${GREEN}Killed all instances of Matt_daemon.${NC}"
    # else
    #     echo -e "${RED}No running instances of Matt_daemon found.${NC}"
    fi


    if [ -f "$LOCK_FILE" ]; then
        rm -f "$LOCK_FILE"
    #     echo -e "${GREEN}Removed lock file: $LOCK_FILE.${NC}"
    # else
    #     echo -e "${RED}No lock file found at $LOCK_FILE.${NC}"
    fi


    if [ -d "$LOG_DIR" ]; then
        rm -rf "$LOG_DIR"
        # echo -e "${GREEN}Removed log directory: $LOG_DIR.${NC}"
    # else
    #     echo -e "${RED}No log directory found at $LOG_DIR.${NC}"
    fi
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        echo -e "${RED}Test failed: The daemon must run as root.${NC}"
        exit 1
    else
        echo -e "${GREEN}Test passed: Daemon is running as root.${NC}"
    fi
}

check_binary_exists() {
    if [ -f "$DAEMON_BINARY" ]; then
        echo -e "${GREEN}Test passed: Daemon binary exists.${NC}"
    else
        echo -e "${RED}Test failed: Daemon binary not found.${NC}"
        exit 1
    fi
}

check_daemon_running() {
    if pgrep $PROGRAM_NAME > /dev/null; then
        echo -e "${GREEN}Test passed: Daemon is running in the background.${NC}"
    else
        echo -e "${RED}Test failed: Daemon is not running.${NC}"
        exit 1
    fi
}

check_port_listening() {
    if ss -tulwn | grep ":4242" > /dev/null; then
        echo -e "${GREEN}Test passed: Daemon is listening on port 4242.${NC}"
    else
        echo -e "${RED}Test failed: Daemon is not listening on port 4242.${NC}"
        exit 1
    fi
}

check_tintin_reporter() {
    if grep -R "TintinReporter()" $SOURCE_DIR/*.cpp > /dev/null; then
        echo -e "${GREEN}Test passed: TintinReporter class exists in the source code.${NC}"
    else
        echo -e "${RED}Test failed: TintinReporter class not found.${NC}"
        exit 1
    fi
}

check_log_entries() {
    if [ -f "$LOG_FILE" ]; then
        echo -e "${GREEN}Test passed: Log file exists.${NC}"
        if tail -n 1 "$LOG_FILE" | grep -E "^\[[[:digit:]]{2}/[[:digit:]]{2}/[[:digit:]]{4} - [[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}\]" > /dev/null; then
            echo -e "${GREEN}Test passed: Log entries are in the correct format.${NC}"
        else
            echo -e "${RED}Test failed: Log entries are not in the correct format.${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Test failed: Log file does not exist.${NC}"
        exit 1
    fi
}
check_single_instance() {
    $DAEMON_BINARY 2> /dev/null &
    $DAEMON_BINARY 2> /dev/null &
    sleep 1
    INSTANCE_COUNT=$(pgrep -x $DAEMON_BINARY | wc -l)
    if [ "$INSTANCE_COUNT" -eq 1 ]; then
        echo -e "${GREEN}Test passed: Only one instance of the daemon is running.${NC}"
    else
        echo -e "${RED}Test failed: More than one instance of the daemon is running.${NC}"
        exit 1
    fi
}

check_lock_file() {
    if [ -f "$LOCK_FILE" ]; then
        echo -e "${GREEN}Test passed: Lock file exists.${NC}"
    else
        echo -e "${RED}Test failed: Lock file does not exist.${NC}"
        exit 1
    fi
}

check_cleanup_on_sigint() {
    pkill -2 $PROGRAM_NAME
    sleep 1
    if [ -f "$LOCK_FILE" ]; then
        echo -e "${RED}Test failed: Lock file was not removed on SIGINT.${NC}"
        exit 1
    else
        echo -e "${GREEN}Test passed: Lock file was removed on SIGINT.${NC}"
    fi
}

check_max_clients() {
    nc localhost 4242 < /dev/null &
    nc localhost 4242 < /dev/null &
    nc localhost 4242 < /dev/null &
    nc localhost 4242 < /dev/null &
    sleep 1
    CLIENT_COUNT=$(pgrep -f "nc localhost 4242" | wc -l)
    if [ "$CLIENT_COUNT" -eq 3 ]; then
        echo -e "${GREEN}Test passed: Only 3 clients can connect simultaneously.${NC}"
    else
        if [ "$CLIENT_COUNT" -eq 4 ]; then
            echo -e "${RED}Test failed: More than 3 clients connected simultaneously.${NC}"
        else
            echo -e "${RED}Test failed: No clients connected.${NC}"
        fi
        pkill -f "nc localhost 4242"
        exit 1
    fi
    pkill -f "nc localhost 4242"
}

cleanup

make && $DAEMON_BINARY

check_root
check_binary_exists
check_daemon_running
check_port_listening
check_tintin_reporter
check_log_entries
check_single_instance
check_lock_file
check_cleanup_on_sigint
# check_max_clients


echo -e "${GREEN}All tests passed successfully.${NC}"
