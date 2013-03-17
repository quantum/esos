#! /bin/sh

# $Id$

# This script is called by mdev and performs different actions
# based on the device node name/type -- similar to udev rules.

# For SCSI disks, we attempt to create a link to the SCSI disk
# using the ID returned by the scsi_id.sh script; if it returns
# non-zero then do nothing.

# For DLM device nodes, we simply make a link to the given device
# node in the /dev/misc directory.

if [ ${MDEV:0:2} = "sd" ]; then
    scsi_id="$(/usr/local/sbin/scsi_id.sh /dev/${MDEV} 2>&1)"
    if [ ${?} -eq 0 ]; then
        mkdir -m 0755 -p /dev/disk-by-id
        ln -s /dev/${MDEV} /dev/disk-by-id/${scsi_id}
    fi
elif [ ${MDEV:0:3} = "dlm" ]; then
    mkdir -m 0755 -p /dev/misc
    ln -s /dev/${MDEV} /dev/misc/${MDEV}
fi
