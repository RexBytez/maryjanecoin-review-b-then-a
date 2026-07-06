#!/bin/bash
#
# Copy guix-build output to Dropbox releases folder
# Determines version from current git branch/tag and copies output folder
# Skips existing files to allow incremental updates for multiple platforms
# Works in WSL (Windows Subsystem for Linux) on Windows
#

set -e  # Exit on error

# Get the script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Dropbox target directory (Windows path)
DROPBOX_BASE="C:/Users/timuy/Dropbox/gig8/archive/jobs/maryjanecoin/maryjanecoin-releases"

# Detect if we're in WSL
IS_WSL=false
if [ -f /proc/version ] && grep -qi microsoft /proc/version; then
    IS_WSL=true
    echo "Detected WSL environment"
fi

# Function to convert Windows path to WSL path
wsl_path() {
    local win_path="$1"
    # Convert C:/Users/... to /mnt/c/Users/...
    # Handle both C:/ and C:\ formats
    echo "$win_path" | sed 's|^[Cc]:/|/mnt/c/|' | sed 's|^[Cc]:\\\\|/mnt/c/|' | sed 's|\\|/|g'
}

# Function to convert WSL path to Windows path (for mklink)
win_path() {
    local wsl_path="$1"
    # Convert /mnt/c/... to C:\...
    echo "$wsl_path" | sed 's|^/mnt/c/|C:\\|' | sed 's|/|\\|g'
}

# Function to convert WSL Linux path to Windows UNC path for accessing WSL filesystem
# This allows Windows to access files in WSL's Linux filesystem via \\wsl$\
wsl_unc_path() {
    local wsl_path="$1"
    # Convert /home/... to \\wsl$\<distro>\home\...
    # First get the WSL distribution name
    local distro=$(wsl.exe -l -q 2>/dev/null | grep -i "running" | head -1 | awk '{print $1}' 2>/dev/null || echo "Ubuntu")
    # Convert /home/tim/... to \\wsl$\Ubuntu\home\tim\...
    # Escape backslashes properly for sed
    local unc_start="\\\\\\\\wsl\\$$distro"
    echo "$wsl_path" | sed "s|^/|${unc_start}\\\\|" | sed 's|/|\\|g'
}

# Function to convert any path to Windows format (handles both WSL and Windows paths)
to_win_path() {
    local path="$1"
    if [[ "$path" == /mnt/* ]]; then
        # Already in WSL Windows mount format, convert to Windows
        win_path "$path"
    elif [[ "$path" == [Cc]:* ]]; then
        # Already Windows format, normalize
        echo "$path" | sed 's|/|\\|g' | sed 's|^[Cc]:|C:|'
    elif [[ "$path" == \\\\wsl\$* ]] || [[ "$path" == \\wsl\$* ]]; then
        # Already in UNC format
        echo "$path"
    else
        # WSL Linux path, convert to UNC path for Windows access
        wsl_unc_path "$path"
    fi
}

# Get current git branch or tag
get_version() {
    # First, try to get the current tag
    local tag=$(git describe --exact-match --tags 2>/dev/null || echo "")
    
    if [ -n "$tag" ]; then
        # Remove 'v' prefix if present (e.g., v2.0.0 -> 2.0.0)
        echo "$tag" | sed 's/^v//'
        return
    fi
    
    # If not on a tag, check if we're on a release branch
    local branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "")
    
    if [[ "$branch" == release/* ]]; then
        # Extract version from release/2.0.0 -> 2.0.0
        echo "$branch" | sed 's|^release/||'
        return
    fi
    
    # Try to get version from latest tag
    local latest_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    if [ -n "$latest_tag" ]; then
        echo "$latest_tag" | sed 's/^v//'
        return
    fi
    
    # Fallback: try to get from configure.ac
    if [ -f "configure.ac" ]; then
        local major=$(grep "define(_CLIENT_VERSION_MAJOR" configure.ac | sed 's/.*, *\([0-9]*\))/\1/')
        local minor=$(grep "define(_CLIENT_VERSION_MINOR" configure.ac | sed 's/.*, *\([0-9]*\))/\1/')
        local build=$(grep "define(_CLIENT_VERSION_BUILD" configure.ac | sed 's/.*, *\([0-9]*\))/\1/')
        if [ -n "$major" ] && [ -n "$minor" ] && [ -n "$build" ]; then
            echo "${major}.${minor}.${build}"
            return
        fi
    fi
    
    echo ""
}

# Get version
VERSION=$(get_version)

if [ -z "$VERSION" ]; then
    echo "ERROR: Could not determine version from git tag, branch, or configure.ac"
    exit 1
fi

echo "Detected version: $VERSION"

# Find guix-build folder
GUIX_BUILD_DIR="$PROJECT_ROOT/guix-build-$VERSION"

if [ ! -d "$GUIX_BUILD_DIR" ]; then
    echo "ERROR: Guix build directory not found: $GUIX_BUILD_DIR"
    echo "Available guix-build directories:"
    ls -d guix-build-* 2>/dev/null | head -5 || echo "  (none found)"
    exit 1
fi

# Find output folder
OUTPUT_DIR="$GUIX_BUILD_DIR/output"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "ERROR: Output directory not found: $OUTPUT_DIR"
    exit 1
fi

echo "Found guix-build output: $OUTPUT_DIR"

# Convert Dropbox path for WSL
DROPBOX_WSL=$(wsl_path "$DROPBOX_BASE")
DROPBOX_TARGET="$DROPBOX_WSL/$VERSION"

# Create Dropbox directory if it doesn't exist
if [ ! -d "$DROPBOX_WSL" ]; then
    echo "Creating Dropbox directory: $DROPBOX_WSL"
    mkdir -p "$DROPBOX_WSL"
fi

# Check if target already exists
if [ -e "$DROPBOX_TARGET" ]; then
    if [ -d "$DROPBOX_TARGET" ]; then
        echo "Target directory already exists: $DROPBOX_TARGET"
        echo "Will copy and skip existing files (incremental update for multiple platforms)"
    else
        echo "WARNING: $DROPBOX_TARGET exists but is not a directory. Removing..."
        rm -f "$DROPBOX_TARGET"
    fi
fi

# Copy output folder to Dropbox
echo "Copying guix-build output to Dropbox..."
echo "  Source: $OUTPUT_DIR"
echo "  Target: $DROPBOX_TARGET"

# Use rsync if available (better for incremental updates), otherwise use cp
if command -v rsync >/dev/null 2>&1; then
    echo "Using rsync for efficient copying (skips existing files)..."
    mkdir -p "$DROPBOX_TARGET"
    rsync -av --ignore-existing "$OUTPUT_DIR/" "$DROPBOX_TARGET/" || {
        echo "rsync completed (some files may have been skipped)"
    }
else
    echo "Using cp (rsync not available, will skip existing files)..."
    mkdir -p "$DROPBOX_TARGET"
    # Copy files, skipping existing ones (-n flag)
    cp -r -n "$OUTPUT_DIR"/* "$DROPBOX_TARGET/" 2>/dev/null || {
        # If that fails, try copying directory structure first
        find "$OUTPUT_DIR" -type d -exec mkdir -p "$DROPBOX_TARGET/{}" \; 2>/dev/null || true
        find "$OUTPUT_DIR" -type f -exec cp -n "{}" "$DROPBOX_TARGET/{}" \; 2>/dev/null || true
    }
fi

# Verify copy
if [ -d "$DROPBOX_TARGET" ] && [ "$(ls -A "$DROPBOX_TARGET" 2>/dev/null)" ]; then
    echo ""
    echo "SUCCESS: Output copied successfully"
    echo "  Target: $DROPBOX_TARGET"
    echo "  Windows path: $(win_path "$DROPBOX_TARGET")"
    echo ""
    echo "Contents:"
    ls -lh "$DROPBOX_TARGET" | head -10
    echo ""
    echo "Note: Existing files were skipped. Run again to update with new platform builds."
else
    echo "ERROR: Failed to copy or verify output directory"
    exit 1
fi
