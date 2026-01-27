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

# Install CLI tools
install -m 0755 bin/mwcap-info_* /usr/bin/mwcap-info
install -m 0755 bin/mwcap-control_* /usr/bin/mwcap-control
install -m 0755 scripts/mwcap-repair.sh /usr/bin/mwcap-repair
install -m 0755 scripts/mwcap-uninstall.sh /usr/bin/mwcap-uninstall

# Install udev rule + modprobe config
install -m 0644 scripts/10-procatpure-event-dev.rules /usr/lib/udev/rules.d/
install -m 0644 /tmp/ProCapture.conf /usr/lib/modprobe.d/ProCapture.conf

echo "ProCapture driver installed successfully."
