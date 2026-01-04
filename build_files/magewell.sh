#!/bin/bash

set -ouex pipefail

echo "Installing Magewell Drivers"

# Attempt Magewell
/ctx/ProCapture/scripts/mwcap-install.sh
# Register path symlink
# We do this via tmpfils.d so tht it is created by the live system

cat >/usr/lib/tmpfiles.d/ProCapture.cnf <<EOF
 L  /usr/local/share/ProCapture  -  -  -  -  /opt/ProCapture
EOF