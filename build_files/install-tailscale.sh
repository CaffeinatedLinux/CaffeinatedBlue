#!/bin/bash

set -ouex pipefail

echo "Installing Tailscale" 

# Add Repo
dnf5 config-manager addrepo --from-repofile=https://pkgs.tailscale.com/stable/fedora/tailscale.repo

# Install Tailscale
dnf5 install -y tailscale

# Clean up the repo
rm /etc/um.repos.d/tailscale.repo -f

# Enable Service
systemctl enable tailscaled.service
