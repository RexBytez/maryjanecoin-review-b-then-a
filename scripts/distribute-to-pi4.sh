#!/bin/bash
# distribute-to-pi4.sh — Send premine from WSL wallet to Pi4 escrow wallet
#
# Sends 990,000,000 MARYJ to Pi4's escrow wallet.
# Keeps 10,000,000 MARYJ on WSL for testing and fee reserve.
#
# PREREQUISITES:
#   - MaryJaneCoind running on WSL with full 1B premine
#   - MaryJaneCoind running on Pi4 (build-on-pi4.sh completed)
#   - SSH keys loaded: ssh-add ~/.ssh/pi_key
#
# USAGE:
#   SSH_AUTH_SOCK=$SSH_AUTH_SOCK bash scripts/distribute-to-pi4.sh

set -e

PI4="jahvinci@10.0.0.80"

# WSL daemon settings
WSL_DAEMON="/mnt/d/MaryJaneCoin-Build/src/MaryJaneCoind"
WSL_DATADIR="/mnt/d/maryjanecoin-data"

# Pi4 daemon path
PI4_DAEMON="~/MaryJaneCoin-Build/src/MaryJaneCoind"

SEND_AMOUNT="990000000"
WSL_RESERVE="10000000"
FEE_BUFFER="1000"   # extra buffer for TX fees (0.420 each)

echo "============================================================"
echo "  MaryJaneCoin Premine Distribution"
echo "  WSL -> Pi4 (${PI4})"
echo "============================================================"
echo ""

# --- Check SSH ---
echo "Checking SSH to Pi4..."
if ! ssh -o ConnectTimeout=10 -o BatchMode=yes "${PI4}" "echo 'SSH OK'" 2>/dev/null; then
    echo "ERROR: Cannot connect to Pi4. Load SSH keys first:"
    echo "  eval \$(ssh-agent)"
    echo "  ssh-add ~/.ssh/pi_key"
    exit 1
fi

# --- Check WSL daemon is running ---
echo "Checking WSL MaryJaneCoind..."
WSL_INFO=$("${WSL_DAEMON}" -datadir="${WSL_DATADIR}" getinfo 2>/dev/null) || {
    echo "ERROR: MaryJaneCoind not running on WSL."
    echo "Start it: ${WSL_DAEMON} -datadir=${WSL_DATADIR}"
    exit 1
}

WSL_BALANCE=$(echo "${WSL_INFO}" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['balance'])")
WSL_BLOCKS=$(echo "${WSL_INFO}"  | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['blocks'])")

echo "  WSL balance:    ${WSL_BALANCE} MARYJ"
echo "  WSL blocks:     ${WSL_BLOCKS}"
echo ""

# --- Check Pi4 daemon is running ---
echo "Checking Pi4 MaryJaneCoind..."
PI4_INFO=$(ssh "${PI4}" "${PI4_DAEMON} getinfo 2>/dev/null") || {
    echo "ERROR: MaryJaneCoind not running on Pi4."
    echo "Start it: ssh ${PI4} '${PI4_DAEMON}'"
    exit 1
}

PI4_BLOCKS=$(echo "${PI4_INFO}" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['blocks'])")
echo "  Pi4 blocks:     ${PI4_BLOCKS}"

# Sanity check: blocks should be within a reasonable range of each other
if [ "${PI4_BLOCKS}" -lt 1 ]; then
    echo "ERROR: Pi4 chain is at block 0 — is it synced to WSL?"
    echo "Both nodes must be connected and synced before transferring."
    exit 1
fi
echo ""

# --- Get a fresh receiving address from Pi4 ---
echo "Getting fresh receiving address from Pi4 escrow wallet..."
PI4_ADDR=$(ssh "${PI4}" "${PI4_DAEMON} getnewaddress escrow_main")
echo "  Pi4 escrow address: ${PI4_ADDR}"
echo ""

# --- Confirm with user ---
echo "============================================================"
echo "  TRANSFER PLAN"
echo "============================================================"
echo "  From:     WSL wallet"
echo "  To:       Pi4 escrow (${PI4_ADDR})"
echo "  Amount:   ${SEND_AMOUNT} MARYJ"
echo "  Keeping:  ${WSL_RESERVE} MARYJ on WSL (testing + fees)"
echo ""
echo "  WSL current balance: ${WSL_BALANCE} MARYJ"
echo ""
read -p "Proceed with transfer? (yes/no): " CONFIRM
if [ "${CONFIRM}" != "yes" ]; then
    echo "Aborted."
    exit 0
fi
echo ""

# --- Send the coins ---
echo "Sending ${SEND_AMOUNT} MARYJ to Pi4..."
TXID=$("${WSL_DAEMON}" -datadir="${WSL_DATADIR}" sendtoaddress "${PI4_ADDR}" "${SEND_AMOUNT}")

if [ -z "${TXID}" ]; then
    echo "ERROR: sendtoaddress returned empty TXID — check daemon logs."
    exit 1
fi

echo "  TXID: ${TXID}"
echo ""

# --- Verify: check Pi4 received it (unconfirmed) ---
echo "Verifying transfer (checking unconfirmed balance on Pi4)..."
sleep 5

PI4_INFO_AFTER=$(ssh "${PI4}" "${PI4_DAEMON} getinfo 2>/dev/null")
PI4_UNCONFIRMED=$(echo "${PI4_INFO_AFTER}" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('newmint', 0))" 2>/dev/null || echo "0")

# Check the tx appeared on Pi4 side
PI4_TX=$(ssh "${PI4}" "${PI4_DAEMON} gettransaction ${TXID} 2>/dev/null" || echo "")
if [ -n "${PI4_TX}" ]; then
    echo "  Transaction visible on Pi4."
else
    echo "  NOTE: TX not yet visible on Pi4 — may take 30-60s to propagate."
fi

# Check WSL remaining balance
WSL_INFO_AFTER=$("${WSL_DAEMON}" -datadir="${WSL_DATADIR}" getinfo)
WSL_BALANCE_AFTER=$(echo "${WSL_INFO_AFTER}" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['balance'])")

echo ""
echo "============================================================"
echo "  TRANSFER COMPLETE"
echo "============================================================"
echo "  TXID:                ${TXID}"
echo "  Amount sent:         ${SEND_AMOUNT} MARYJ"
echo "  Pi4 escrow address:  ${PI4_ADDR}"
echo "  WSL remaining:       ${WSL_BALANCE_AFTER} MARYJ"
echo ""
echo "  NEXT STEPS:"
echo "  1. Wait for 10 confirmations (~43 minutes at 260s blocks)"
echo "  2. Generate escrow wallet hierarchy on Pi4:"
echo "       ssh ${PI4} 'python3 ~/maryjanecoin-bridge/generate-escrow-wallets.py --password YOUR_PASSWORD'"
echo "  3. Back up Pi4 wallet.dat to WSL:"
echo "       scp ${PI4}:~/.MaryJaneCoin/wallet.dat ./wallet-pi4-escrow-\$(date +%Y%m%d-%H%M).dat"
echo ""
