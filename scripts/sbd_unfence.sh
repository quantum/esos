#! /bin/sh

# A simple utility script which reads the SBD configuration file and loops
# through all SBD block devices, clearing message slots for all nodes. This
# effectively unfences any nodes.

SBD_CONFIG="/etc/sysconfig/sbd"

# Read the SBD config file and loop over all devices
while read -r device; do
    while read -r sbd_list; do
        node="$(echo ${sbd_list} | awk '{print $2}')"
        echo "Clearing message slots for node '${node}'" \
            "on device '${device}'..."
        sbd -d ${device} message ${node} clear
    done <<< "$(sbd -d ${device} list)"
done <<< "$(cat ${SBD_CONFIG} | grep SBD_DEVICE | cut -d= -f2 | \
    tr -d '"' | tr ';' '\n')"

