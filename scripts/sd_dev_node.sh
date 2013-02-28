#! /bin/sh

# $Id$

# This script is called by mdev and attempts to create a link to
# the SCSI disk using the ID returned by the scsi_id.sh script; if
# it returns non-zero then do nothing.

scsi_id="$(scsi_id.sh /dev/${MDEV} 2>&1)"
if [ ${?} -eq 0 ]; then
    mkdir -m 0755 -p /dev/disk-by-id
    ln -s /dev/${MDEV} /dev/disk-by-id/${scsi_id}
fi
