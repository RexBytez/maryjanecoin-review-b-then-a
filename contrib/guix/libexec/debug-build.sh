#!/bin/bash
# Set this to the point where you want the build to stop
# Possible values: "env_setup", "configure", "make"
STOP_AT="make"

# Source the original build script but intercept at the right point
cd /maryjanecoin

# First, copy the original build.sh
cp contrib/guix/libexec/build.sh /tmp/modified-build.sh

# Now insert our stop points with direct file edits instead of sed
if [ "$STOP_AT" = "env_setup" ]; then
  # Add code right after environment setup
  line_num=$(grep -n "Environment variables for determinism" /tmp/modified-build.sh | cut -d: -f1)
  if [ -n "$line_num" ]; then
    line_num=$((line_num + 3))  # Move past the export lines
    sed -i "${line_num}i\\
echo \"=== Stopped at environment setup as requested ===\"\\
echo \"You can now manually run ./autogen.sh, ./configure, and make\"\\
cd \$DISTSRC\\
exec bash -i" /tmp/modified-build.sh
  fi
elif [ "$STOP_AT" = "configure" ]; then
  # Add code right after configure
  line_num=$(grep -n "\.\/configure" /tmp/modified-build.sh | cut -d: -f1)
  if [ -n "$line_num" ]; then
    line_num=$((line_num + 1))
    sed -i "${line_num}i\\
echo \"=== Stopped after configure as requested ===\"\\
echo \"You can now manually run make and debug linking issues\"\\
cd \$DISTSRC\\
exec bash -i" /tmp/modified-build.sh
  fi
elif [ "$STOP_AT" = "make" ]; then
  # Add code right before make
  line_num=$(grep -n "make --jobs" /tmp/modified-build.sh | cut -d: -f1)
  if [ -n "$line_num" ]; then
    sed -i "${line_num}i\\
echo \"=== Stopped before make as requested ===\"\\
echo \"You can now manually run make and debug linking issues\"\\
cd \$DISTSRC\\
exec bash -i" /tmp/modified-build.sh
  fi
fi

chmod +x /tmp/modified-build.sh
bash /tmp/modified-build.sh