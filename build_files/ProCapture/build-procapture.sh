#!/usr/bin/env bash
set -euo pipefail

KVER=$(rpm -q kernel-core --qf '%{VERSION}-%{RELEASE}.%{ARCH}\n')

echo "Building Magewell ProCapture driver for kernel: $KVER"

dnf install -y \
    gcc \
    make \
    elfutils-libelf-devel \
    kernel-devel-$KVER \
    kernel-core-$KVER \
    cmake

# Writable kernel headers
WRITABLE_KERNEL=/tmp/kernel-$KVER
mkdir -p "$WRITABLE_KERNEL"
cp -a /usr/src/kernels/$KVER/* "$WRITABLE_KERNEL"

# Writable module source
WRITABLE_SRC=/tmp/procapture-src
mkdir -p "$WRITABLE_SRC"
cd /ctx/ProCapture
cp -a src/* "$WRITABLE_SRC"

# Build module
make -C "$WRITABLE_KERNEL" M="$WRITABLE_SRC" modules

# Install module
mkdir -p /usr/lib/modules/$KVER/extra
install -m 0644 "$WRITABLE_SRC/ProCapture.ko" /usr/lib/modules/$KVER/extra/

# Install assets
mkdir -p /usr/share/ProCapture/src/res
install -m 0644 src/res/*.png /usr/share/ProCapture/src/res/

# Build FE tools
if [ -d ProCaptureUserFE ]; then
    cmake -S ProCaptureUserFE -B ProCaptureUserFE/build
    make -C ProCaptureUserFE/build -j"$(nproc)"
    install -m 0755 ProCaptureUserFE/build/bin/*/mw_fe /usr/bin/mw_fe
fi

# Install CLI tools (prebuilt binaries)
ARCHBITS=$(uname -m)
case "$ARCHBITS" in
    x86_64) TOOL_SUFFIX="64" ;;
    aarch64) TOOL_SUFFIX="aarch64" ;;
    i386|i686) TOOL_SUFFIX="32" ;;
    *) TOOL_SUFFIX="64" ;;
esac

# mwcap-info
if ls bin/mwcap-info_* >/dev/null 2>&1; then
    INFO_BIN=$(ls bin/mwcap-info_* | head -n1)
    install -m 0755 "$INFO_BIN" /usr/bin/mwcap-info
else
    echo "Warning: mwcap-info binary not found, skipping"
fi

# mwcap-control
if ls bin/mwcap-control_* >/dev/null 2>&1; then
    CTRL_BIN=$(ls bin/mwcap-control_* | head -n1)
    install -m 0755 "$CTRL_BIN" /usr/bin/mwcap-control
else
    echo "Warning: mwcap-control binary not found, skipping"
fi

# Install udev rule + modprobe config
install -m 0644 scripts/10-procatpure-event-dev.rules /usr/lib/udev/rules.d/
install -m 0644 /tmp/ProCapture.conf /usr/lib/modprobe.d/ProCapture.conf

echo "ProCapture driver installed successfully."
