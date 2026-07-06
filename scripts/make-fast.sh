#!/bin/bash
#
# Fast make build script for MaryJaneCoin
# Uses most processors but leaves 2 free for system responsiveness
#

set -e  # Exit on error

# Get the script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Calculate number of jobs: use all processors minus 2, but at least 1
NUM_JOBS=$(($(nproc) - 2))
if [ $NUM_JOBS -lt 1 ]; then
    NUM_JOBS=1
fi

echo "================================"
echo "MaryJaneCoin Fast Build"
echo "================================"
echo ""
echo "Available processors: $(nproc)"
echo "Using processors: $NUM_JOBS (leaving 2 free)"
echo ""

# Run make with optimized job count
make -j$NUM_JOBS "$@"
