#!/bin/bash

set -ouex pipefail

### Install packages

# Packages can be installed from any enabled yum repo on the image.
# RPMfusion repos are available by default in ublue main images
# List of rpmfusion packages can be found here:
# https://mirrors.rpmfusion.org/mirrorlist?path=free/fedora/updates/39/x86_64/repoview/index.html&protocol=https&redirect=1

# this installs a package from fedora repos



# Install Groups
dnf5 group install -y cosmic-desktop
dnf5 group install -y cosmic-desktop-apps

# Install Core Packages
dnf5 install -y tmux zsh fastfetch lm_sensors oddjob make oddjob-mkhomedir freeipa-client kernel-devel-matched kernel-devel libcamera-v4l2 v4l2loopback

# Install Other Software
/ctx/install-vivaldi.sh
/ctx/install-tailscale.sh

# Magewell Install Script
# /ctx/magewell.sh

systemctl enable podman.socket







