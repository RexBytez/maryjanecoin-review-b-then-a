#!/bin/bash
# Set up MaryJaneCoin Guix build environment
# ISOLATED from MotaCoin and PotCoin builds
#
# This creates ~/MaryJaneCoin-Build/ in WSL native filesystem
# (NOT on /mnt/c/ — Guix needs native Linux filesystem)

BUILD_DIR="$HOME/MaryJaneCoin-Build"
GITLAB_URL="https://StatMaxxer:glpat-phbeGB1F1UrX57DMNml-H2M6MQpvOjEKdTprcWJ5aA8.01.170ww8r2z@gitlab.com/StatMaxxer/maryjanecoin.git"

echo "Setting up MaryJaneCoin Guix build environment..."
echo "Build directory: $BUILD_DIR"
echo ""

# Check not conflicting with other builds
if [ -d "$HOME/PotCoin-Build" ]; then
    echo "NOTE: PotCoin-Build exists at $HOME/PotCoin-Build — will NOT be touched"
fi
if [ -d "$HOME/MotaCoin-Build" ]; then
    echo "NOTE: MotaCoin-Build exists at $HOME/MotaCoin-Build — will NOT be touched"
fi

# Clone or update
if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists, pulling latest..."
    cd "$BUILD_DIR" && git pull
else
    echo "Cloning MaryJaneCoin..."
    git clone "$GITLAB_URL" "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Verify it's MaryJaneCoin, not PotCoin
if grep -q "MaryJaneCoin" configure.ac; then
    echo "Verified: This is MaryJaneCoin (not PotCoin)"
else
    echo "ERROR: This does not appear to be MaryJaneCoin!"
    exit 1
fi

echo ""
echo "Ready for Guix build. Run:"
echo "  cd $BUILD_DIR"
echo "  ./contrib/guix/guix-build"
echo ""
echo "Output will be in: $BUILD_DIR/guix-build-1.0.0/output/"
