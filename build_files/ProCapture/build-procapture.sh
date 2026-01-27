#!/usr/bin/env bash
set -euo pipefail

# Detect kernel version inside the image build environment
KVER=$(rpm -q kernel-core --qf '%{VERSION}-%{RELEASE}.%{ARCH}\n')

echo "Building Magewell ProCapture driver for kernel: $KVER"

# Install build dependencies
dnf install -y \
    gcc \
    make \
    elfutils-libelf-devel \
    kernel-devel-$KVER \
    kernel-core-$KVER \
    cmake

# Unpack source
# tar xf procapture-*.tar.gz
cd /ctx/ProCapture

# Build kernel module
make -C /lib/modules/$KVER/build M=$PWD/src modules

# Install kernel module
mkdir -p /usr/lib/modules/$KVER/extra
install -m 0644 src/ProCapture.ko /usr/lib/modules/$KVER/extra/

# Install PNG assets into immutable /usr/share
mkdir -p /usr/share/ProCapture/src/res
install -m 0644 src/res/*.png /usr/share/ProCapture/src/res/

# Build user-space FE tools
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

# Install udev rule
install -m 0644 scripts/10-procatpure-event-dev.rules /usr/lib/udev/rules.d/

# Install our patched ProCapture.conf
install -m 0644 /tmp/ProCapture.conf /usr/lib/modprobe.d/ProCapture.conf

echo "ProCapture driver + assets installed into image filesystem."
