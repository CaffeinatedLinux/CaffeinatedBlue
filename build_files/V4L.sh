#!/bin/bash

set -ouex pipefail

# Add RPMFusion
dnf5 install -y https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

# Install Packages

dnf5 install -y v4l2loopback /tmp/akmods-common/rpms/kmods/kmod-v4l2loopback*.rpm

# Remove RPMFusion
dnf5 -y remove rpmfusion-free-release
dnf5 -y remove rpmfusion-nonfree-release

#RUN dnf install /tmp/akmods-common/rpms/ublue-os/ublue-os-akmods*.rpm
#RUN dnf install /tmp/akmods-common/rpms/kmods/kmod-v4l2loopback*.rpm

