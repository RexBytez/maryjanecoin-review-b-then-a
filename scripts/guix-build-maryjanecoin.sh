#!/bin/bash
# Build MaryJaneCoin release binaries with Guix
# Outputs to ~/MaryJaneCoin-Build/guix-build-VERSION/output/
# Completely isolated from PotCoin and MotaCoin builds

set -e
BUILD_DIR="$HOME/MaryJaneCoin-Build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Run scripts/setup-guix-build.sh first"
    exit 1
fi

cd "$BUILD_DIR"

# Verify identity
if ! grep -q "MaryJaneCoin" configure.ac; then
    echo "SAFETY CHECK FAILED: $BUILD_DIR is not MaryJaneCoin!"
    exit 1
fi

# Check no PotCoin/MotaCoin contamination
if grep -rq "PotCoind\|MotaCoind" src/makefile.unix 2>/dev/null; then
    echo "CONTAMINATION CHECK FAILED: Found PotCoin/MotaCoin references in build files!"
    exit 1
fi

echo "Building MaryJaneCoin release binaries..."
echo "Build directory: $BUILD_DIR"
echo "PotCoin-Build: $(ls -d $HOME/PotCoin-Build 2>/dev/null || echo 'not present') — UNTOUCHED"
echo "MotaCoin-Build: $(ls -d $HOME/MotaCoin-Build 2>/dev/null || echo 'not present') — UNTOUCHED"
echo ""

# Run Guix build
./contrib/guix/guix-build

echo ""
echo "Build complete! Check output in:"
ls -d "$BUILD_DIR"/guix-build-*/output/ 2>/dev/null
