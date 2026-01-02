#!/bin/bash

set -ouex pipefail

echo "Installing Vivaldi" 

# Prepare staging directory
# mkdir -p /var/opt # -p just in case it exists

#Prepare alternatives directory
# mkdir -p /var/lib/alternatives

# Add Repo
dnf5 config-manager addrepo --from-repofile=https://repo.vivaldi.com/stable/vivaldi-fedora.repo

#Install
dnf5 install -y vivaldi-stable

# Clean up the repo
rm /etc/yum.repos.d/vivaldi-fedora.repo -f

# And then we do the hacky dance! 
# mv /var/opt/vivaldi /usr/lib/vivaldi

# Register path symlink
# We do this via tmpfils.d so tht it is created by the live system
# cat >/usr/lib/tmpfiles.d/vivaldi.cnf <<EOF
# L  /opt/vivaldi  -  -  -  -  /usr/lib/vivaldi
# EOF