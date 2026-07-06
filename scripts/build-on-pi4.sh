#!/bin/bash
# build-on-pi4.sh — Build MaryJaneCoind natively on Pi4 (ARM64)
#
# Run this script FROM WSL — it SSHes into Pi4 and runs the build there.
#
# PREREQUISITES (on Pi4, installed once):
#   sudo apt update && sudo apt install -y \
#     build-essential git cmake \
#     libboost-all-dev libssl-dev libdb++-dev libdb-dev \
#     libminiupnpc-dev libqrencode-dev \
#     pkg-config autoconf automake libtool
#
# USAGE (from WSL after loading SSH keys):
#   SSH_AUTH_SOCK=$SSH_AUTH_SOCK bash scripts/build-on-pi4.sh
#
# The script will:
#   1. Install build dependencies on Pi4
#   2. Clone the MaryJaneCoin repo from GitLab
#   3. Build LevelDB for ARM64 using cmake
#   4. Build MaryJaneCoind using makefile.unix
#   5. Verify the binary runs

set -e

PI4="jahvinci@10.0.0.80"
GITLAB_TOKEN="glpat-phbeGB1F1UrX57DMNml-H2M6MQpvOjEKdTprcWJ5aA8.01.170ww8r2z"
REPO_URL="https://StatMaxxer:${GITLAB_TOKEN}@gitlab.com/StatMaxxer/maryjanecoin.git"
BUILD_DIR="~/MaryJaneCoin-Build"

echo "============================================================"
echo "  Build MaryJaneCoind on Pi4 (ARM64 — native build)"
echo "  Target: ${PI4}"
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

# Get Pi4 CPU info
PI4_ARCH=$(ssh "${PI4}" "uname -m")
PI4_CPUS=$(ssh "${PI4}" "nproc")
echo "  Pi4 arch:  ${PI4_ARCH}"
echo "  Pi4 CPUs:  ${PI4_CPUS}"
echo ""

if [ "${PI4_ARCH}" != "aarch64" ]; then
    echo "WARNING: Expected aarch64 (ARM64), got ${PI4_ARCH}"
    echo "This script is for native ARM64 builds. Proceed anyway? (yes/no)"
    read -p "> " CONFIRM
    [ "${CONFIRM}" = "yes" ] || exit 1
fi

# --- Install dependencies on Pi4 ---
echo "[1/5] Installing build dependencies on Pi4..."
ssh "${PI4}" bash <<'REMOTE_EOF'
set -e
sudo apt-get update -qq
sudo apt-get install -y \
    build-essential git cmake \
    libboost-all-dev libssl-dev \
    libdb++-dev libdb-dev \
    libminiupnpc-dev libqrencode-dev \
    pkg-config autoconf automake libtool \
    2>&1 | tail -10
echo "  Dependencies installed."
REMOTE_EOF
echo ""

# --- Clone or update repo ---
echo "[2/5] Cloning MaryJaneCoin repo on Pi4..."
ssh "${PI4}" bash <<REMOTE_EOF
set -e
if [ -d ${BUILD_DIR}/.git ]; then
    echo "  Repo already exists — pulling latest..."
    cd ${BUILD_DIR}
    git pull origin main 2>&1 | tail -5
else
    echo "  Cloning from GitLab..."
    git clone "${REPO_URL}" ${BUILD_DIR} 2>&1 | tail -10
fi
echo "  Repo ready at ${BUILD_DIR}"
REMOTE_EOF
echo ""

# --- Build LevelDB for ARM64 ---
echo "[3/5] Building LevelDB for ARM64..."
ssh "${PI4}" bash <<'REMOTE_EOF'
set -e
BUILD_DIR=~/MaryJaneCoin-Build
cd ${BUILD_DIR}/src/leveldb

# Clean any previous build artifacts
if [ -d build ]; then
    rm -rf build
fi

mkdir -p build && cd build

# Configure LevelDB with cmake (proper ARM64 support)
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DLEVELDB_BUILD_TESTS=OFF \
    -DLEVELDB_BUILD_BENCHMARKS=OFF \
    ..

# Build
cmake --build . -- -j$(nproc)

# Copy static libs to expected location
cp libleveldb.a ../ 2>/dev/null || true
cp libmemenv.a  ../ 2>/dev/null || true

# Verify
if [ -f libleveldb.a ]; then
    echo "  libleveldb.a built successfully"
    ls -lh libleveldb.a libmemenv.a 2>/dev/null || true
else
    # Some cmake versions name it differently
    find . -name "*.a" | head -5
    echo "  WARNING: libleveldb.a not found at expected path — check cmake output"
    exit 1
fi
REMOTE_EOF
echo ""

# --- Build MaryJaneCoind ---
echo "[4/5] Building MaryJaneCoind (this takes 10-20 minutes on Pi4)..."
ssh "${PI4}" bash <<'REMOTE_EOF'
set -e
BUILD_DIR=~/MaryJaneCoin-Build
cd ${BUILD_DIR}/src

# Build daemon only (no Qt GUI — headless Pi4)
# USE_UPNP=- disables UPnP (optional on Pi4)
# USE_IPV6=1 enable IPv6
NPROC=$(nproc)
echo "  Building with ${NPROC} cores..."
echo "  Start time: $(date)"

make -f makefile.unix \
    USE_UPNP=- \
    USE_IPV6=1 \
    -j${NPROC} \
    2>&1

echo "  End time: $(date)"
REMOTE_EOF
echo ""

# --- Verify the binary ---
echo "[5/5] Verifying binary..."
ssh "${PI4}" bash <<'REMOTE_EOF'
set -e
BINARY=~/MaryJaneCoin-Build/src/MaryJaneCoind

if [ ! -f "${BINARY}" ]; then
    echo "ERROR: ${BINARY} not found — build failed!"
    exit 1
fi

# Check it's the right architecture
ARCH=$(file "${BINARY}")
echo "  Binary: ${ARCH}"

# Check it can at least print --version or help
VERSION_OUTPUT=$(${BINARY} --version 2>&1 || ${BINARY} -help 2>&1 | head -3 || echo "(no --version flag)")
echo "  Output: ${VERSION_OUTPUT}"

ls -lh "${BINARY}"
echo ""
echo "  MaryJaneCoind built successfully on ARM64!"
REMOTE_EOF
echo ""

echo "============================================================"
echo "  BUILD COMPLETE"
echo "============================================================"
echo ""
echo "  Binary at: Pi4:~/MaryJaneCoin-Build/src/MaryJaneCoind"
echo ""
echo "  NEXT STEPS:"
echo ""
echo "  1. Ensure config exists:"
echo "     ssh ${PI4} 'cat ~/.MaryJaneCoin/MaryJaneCoin.conf'"
echo "     (Run deploy-to-pi4.sh first if config is missing)"
echo ""
echo "  2. Start the daemon:"
echo "     ssh ${PI4} '~/MaryJaneCoin-Build/src/MaryJaneCoind'"
echo ""
echo "  3. Check it started:"
echo "     ssh ${PI4} '~/MaryJaneCoin-Build/src/MaryJaneCoind getinfo'"
echo ""
echo "  4. Check peers (should connect to AWS seed 35.155.206.7):"
echo "     ssh ${PI4} '~/MaryJaneCoin-Build/src/MaryJaneCoind getpeerinfo'"
echo ""
echo "  5. Once synced, run distribute-to-pi4.sh to send the premine."
echo ""
