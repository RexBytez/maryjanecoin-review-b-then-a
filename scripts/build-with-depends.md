# Building MaryJaneCoin with Depends System

This document explains how to build MaryJaneCoin using the depends system, which is similar to how Guix builds internally. This approach builds all dependencies (including OpenSSL) locally in the `depends/` directory instead of relying on system packages.

## Why Use the Depends System?

- **Deterministic builds**: Uses specific versions of dependencies (e.g., OpenSSL 1.0.2u)
- **Isolated builds**: Doesn't conflict with system package versions
- **Reproducible**: Same dependencies across different systems
- **No root required**: Dependencies are built in the project directory

## Quick Start

Use the provided script:

```bash
cd /path/to/MaryJaneCoin
./scripts/build-with-depends.sh
```

This script will:
1. Check for and automatically install missing system dependencies (build tools, autotools, cmake, etc.)
2. Build all dependencies in `depends/` directory
3. Generate `configure` script if needed
4. Configure the build using `CONFIG_SITE` to point to the depends
5. Build MaryJaneCoin

**Note**: The script requires sudo access to install system packages. It will only install packages that are missing.

## Manual Build Process

If you prefer to do it manually:

### Step 1: Install System Build Tools

The build script will automatically check for and install missing dependencies. If you prefer to install them manually:

```bash
sudo apt-get update
sudo apt-get install build-essential autoconf automake libtool cmake pkg-config bison xz-utils curl python3 patch
```

### Step 2: Build Dependencies

Build all dependencies for your platform:

```bash
cd depends
make -j$(nproc)  # -j uses all CPU cores
```

This will:
- Download sources for OpenSSL, Boost, Qt, etc.
- Build them with the correct configuration
- Install them to `depends/x86_64-pc-linux-gnu/` (or your platform triplet)
- Create `depends/x86_64-pc-linux-gnu/share/config.site`

### Step 3: Generate Configure Script

If you haven't already:

```bash
cd ..  # back to project root
./autogen.sh
```

### Step 4: Configure with CONFIG_SITE

The key is to set `CONFIG_SITE` to point to the config.site file created by the depends build:

```bash
HOST=$(./depends/config.guess)  # Detect your platform
CONFIG_SITE=$PWD/depends/$HOST/share/config.site ./configure --prefix=$PWD/depends/$HOST
```

This tells `configure` to use the dependencies from the `depends/` directory instead of system packages.

### Step 5: Build

```bash
make -j$(nproc)
```

## How It Works

1. **depends/Makefile**: Builds each dependency package (OpenSSL, Boost, etc.) and installs them to `depends/<HOST>/`
2. **depends/config.site.in**: Template that gets processed to create `config.site`
3. **config.site**: Sets environment variables like `PKG_CONFIG_PATH`, `CPPFLAGS`, `LDFLAGS` to point to the depends directory
4. **CONFIG_SITE**: Environment variable that tells autotools to source the config.site file during configure

The `config.site` file sets up:
- `PKG_CONFIG_PATH` to find `.pc` files in the depends directory
- `CPPFLAGS` to include headers from `depends/<HOST>/include`
- `LDFLAGS` to link libraries from `depends/<HOST>/lib`
- Compiler and tool paths

## Platform Detection

The script automatically detects your platform using `depends/config.guess`. Common platforms:
- `x86_64-pc-linux-gnu` - 64-bit Linux
- `i686-pc-linux-gnu` - 32-bit Linux
- `x86_64-w64-mingw32` - Windows (cross-compile)
- `x86_64-apple-darwin` - macOS (cross-compile)

## Comparison with System Packages

### Old Way (using system packages)
```bash
sudo apt-get install libssl-dev libboost-all-dev ...
./configure
make
```

Problems:
- Uses whatever OpenSSL version is in the system repos (may be too new/old)
- Conflicts between package versions
- Different results on different systems

### New Way (using depends)
```bash
cd depends && make
CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure
make
```

Benefits:
- Uses OpenSSL 1.0.2u (specified in depends/packages/openssl.mk)
- Isolated from system packages
- Reproducible builds

## Troubleshooting

### "config.site not found"
Make sure you built the depends first:
```bash
cd depends && make
```

### "OpenSSL version mismatch"
If you see SSL errors, make sure you're using `CONFIG_SITE`. The configure script will print which OpenSSL it's using - it should be from the depends directory.

### "Package not found"
Check that the depends build completed successfully. Look for error messages in the depends build output.

### Clean Build
To rebuild everything from scratch:
```bash
cd depends
make clean  # Clean depends build artifacts
make        # Rebuild depends
cd ..
make distclean  # Clean main project
CONFIG_SITE=$PWD/depends/$(./depends/config.guess)/share/config.site ./configure
make
```

## Advanced: Building Specific Components

You can disable certain dependencies if you don't need them:

```bash
cd depends
make NO_QT=1      # Skip Qt (for headless builds)
make NO_WALLET=1  # Skip wallet dependencies
make NO_UPNP=1    # Skip UPnP support
```

See `depends/README.md` for all options.

