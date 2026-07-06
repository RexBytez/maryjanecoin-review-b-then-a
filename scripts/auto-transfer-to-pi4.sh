#!/bin/bash
# auto-transfer-to-pi4.sh — Send 200M MARYJ to Pi4 escrow
# Designed to run every 2 hours via cron until balance is drained
#
# Each run sends 200,000,000 MARYJ (20% of original 1B) to a fresh Pi4 address
# After 5 runs (10 hours), all coins will be on Pi4
# If balance < 200M, sends remaining balance instead

export SSH_AUTH_SOCK=/tmp/ssh-3V7x1nYq7zHB/agent.42395

WSL_DAEMON="/mnt/c/Users/balle/Desktop/MagnusOpus/Projects/MaryJaneCoin/core/src/MaryJaneCoind"
WSL_DATADIR="/mnt/d/maryjanecoin-data"
PI4_DAEMON="~/MaryJaneCoin-Build/src/MaryJaneCoind"
BATCH=200000000
LOGFILE="/mnt/d/maryjanecoin-data/auto-transfer.log"

echo "$(date): === Auto-transfer batch starting ===" >> "$LOGFILE"

# Check WSL daemon
BAL=$("$WSL_DAEMON" -datadir="$WSL_DATADIR" getbalance 2>/dev/null)
if [ -z "$BAL" ]; then
    echo "$(date): ERROR — WSL daemon not running" >> "$LOGFILE"
    exit 1
fi

# Check if balance is too low
BAL_INT=$(python3 -c "print(int(float('$BAL')))")
if [ "$BAL_INT" -lt 1000 ]; then
    echo "$(date): Balance $BAL too low, all coins transferred. Done!" >> "$LOGFILE"
    # Remove cron job
    crontab -l 2>/dev/null | grep -v "auto-transfer-to-pi4" | crontab -
    echo "$(date): Cron job removed — transfer complete" >> "$LOGFILE"
    exit 0
fi

# If remaining balance < BATCH, send everything minus 100 for fees
if [ "$BAL_INT" -lt "$BATCH" ]; then
    SEND_AMOUNT=$(python3 -c "print(int(float('$BAL')) - 100)")
    echo "$(date): Final batch — sending remaining $SEND_AMOUNT MARYJ" >> "$LOGFILE"
else
    SEND_AMOUNT=$BATCH
fi

# Get fresh address from Pi4
BATCH_NUM=$(grep -c "TX:" "$LOGFILE" 2>/dev/null || echo 0)
BATCH_NUM=$((BATCH_NUM + 1))
PI4_ADDR=$(ssh -o ConnectTimeout=10 sshpi4 "$PI4_DAEMON getnewaddress escrow_batch_${BATCH_NUM}" 2>/dev/null)

if [ -z "$PI4_ADDR" ]; then
    echo "$(date): ERROR — Cannot get address from Pi4" >> "$LOGFILE"
    exit 1
fi

# Send
TXID=$("$WSL_DAEMON" -datadir="$WSL_DATADIR" sendtoaddress "$PI4_ADDR" "$SEND_AMOUNT" 2>&1)

if echo "$TXID" | grep -q "^[a-f0-9]\{64\}$"; then
    echo "$(date): TX: $TXID — Sent $SEND_AMOUNT MARYJ to $PI4_ADDR (batch $BATCH_NUM)" >> "$LOGFILE"
    NEW_BAL=$("$WSL_DAEMON" -datadir="$WSL_DATADIR" getbalance 2>/dev/null)
    echo "$(date): Remaining WSL balance: $NEW_BAL" >> "$LOGFILE"
else
    echo "$(date): ERROR — Send failed: $TXID" >> "$LOGFILE"
    exit 1
fi
