#!/bin/bash
#
# Build MaryJaneCoin using the depends system (like Guix does internally)
# This builds all dependencies (including OpenSSL) locally instead of using system packages
#

set -e  # Exit on error

# Get the script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Detect host platform
HOST=$(./depends/config.guess)
echo "Detected host platform: $HOST"

# Configuration
NUM_JOBS=${NUM_JOBS:-$(nproc)}  # Use all CPU cores by default
DEPENDS_DIR="$PROJECT_ROOT/depends"
CONFIG_SITE="$DEPENDS_DIR/$HOST/share/config.site"

echo "================================"
echo "MaryJaneCoin Build with Depends"
echo "================================"
echo ""
echo "Project root: $PROJECT_ROOT"
echo "Host platform: $HOST"
echo "Depends directory: $DEPENDS_DIR"
echo "Config site: $CONFIG_SITE"
echo "Build jobs: $NUM_JOBS"
echo ""

# Step 1: Install system dependencies for building depends
echo "================================"
echo "Step 1: Checking and installing system dependencies"
echo "================================"
echo ""

# All required packages for depends system (based on Guix manifest.scm and depends/README.md)
REQUIRED_PACKAGES=(
    build-essential
    autoconf
    automake
    libtool
    cmake
    pkg-config
    bison
    xz-utils
    curl
    python3
    patch
)

# Map tools to their package names for checking
declare -A TOOL_PACKAGES=(
    [gcc]="build-essential"
    [g++]="build-essential"
    [make]="build-essential"
    [autoconf]="autoconf"
    [automake]="automake"
    [libtoolize]="libtool"
    [cmake]="cmake"
    [pkg-config]="pkg-config"
    [bison]="bison"
    [xz]="xz-utils"
    [curl]="curl"
    [python3]="python3"
    [patch]="patch"
)

# Check if we're on Ubuntu/Debian
if ! command -v apt-get &> /dev/null; then
    echo "ERROR: This script requires apt-get (Ubuntu/Debian)."
    echo "Please install the following packages manually:"
    printf "  %s\n" "${REQUIRED_PACKAGES[@]}"
    exit 1
fi

# Check for required tools and collect missing packages
MISSING_PACKAGES=()
for tool in gcc g++ make autoconf automake libtoolize cmake pkg-config bison xz curl python3 patch; do
    if ! command -v "$tool" &> /dev/null; then
        pkg="${TOOL_PACKAGES[$tool]}"
        if [ -n "$pkg" ] && [[ ! " ${MISSING_PACKAGES[@]} " =~ " ${pkg} " ]]; then
            MISSING_PACKAGES+=("$pkg")
        fi
    fi
done

# Remove duplicates
IFS=$'\n' MISSING_PACKAGES=($(sort -u <<<"${MISSING_PACKAGES[*]}"))
unset IFS

if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
    echo "Missing required packages: ${MISSING_PACKAGES[*]}"
    echo ""
    echo "Installing missing dependencies..."
    echo ""
    
    # Update package list
    sudo apt-get update
    
    # Install missing packages
    sudo apt-get install -y "${MISSING_PACKAGES[@]}"
    
    echo ""
    echo "Dependencies installed successfully!"
else
    echo "All required build tools are already installed."
fi

echo ""

# Step 2: Build dependencies
echo "================================"
echo "Step 2: Building dependencies"
echo "================================"
echo ""
echo "This will build all dependencies (OpenSSL, Boost, Qt, etc.) locally."
echo "This may take a while..."
echo ""

cd "$DEPENDS_DIR"
make -j"$NUM_JOBS" HOST="$HOST"

if [ ! -f "$CONFIG_SITE" ]; then
    echo "Error: config.site was not created at $CONFIG_SITE"
    exit 1
fi

echo ""
echo "Dependencies built successfully!"
echo ""

# Step 3: Generate configure script and bitcoin-config.h.in if needed
cd "$PROJECT_ROOT"
if [ ! -f "./configure" ] || [ ! -f "src/config/bitcoin-config.h.in" ]; then
    echo "================================"
    echo "Step 3: Generating configure script and config files"
    echo "================================"
    echo ""
    ./autogen.sh
    echo ""
fi

# Step 4: Configure build
echo "================================"
echo "Step 4: Configuring build"
echo "================================"
echo ""
echo "Configuring with CONFIG_SITE=$CONFIG_SITE"
echo "This ensures the build uses the dependencies from the depends directory."
echo ""

CONFIG_SITE="$CONFIG_SITE" ./configure --prefix="$DEPENDS_DIR/$HOST" "$@"

echo ""
echo "Configuration complete!"
echo ""

# Step 5: Build
echo "================================"
echo "Step 5: Building MaryJaneCoin"
echo "================================"
echo ""

make -j"$NUM_JOBS"

echo ""
echo "================================"
echo "Build complete!"
echo "================================"
echo ""
echo "Binaries location:"
echo "  - src/maryjanecoind (daemon)"
echo "  - src/qt/maryjanecoin-qt (GUI wallet, if Qt was built)"
echo ""

