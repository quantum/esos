#! /bin/sh

# A simple utility script which reads the SBD configuration file and loops
# through all SBD block devices, listing (printing) all allocated slots and
# messages for each.

SBD_CONFIG="/etc/sysconfig/sbd"

# Read the SBD config file and loop over all devices
while read -r device; do
    echo "Device '${device}':"
    sbd -d ${device} list
    echo
done <<< "$(cat ${SBD_CONFIG} | grep SBD_DEVICE | cut -d= -f2 | \
    tr -d '"' | tr ';' '\n')"

