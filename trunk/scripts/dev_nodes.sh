#! /bin/sh

# $Id$

# This script is called by mdev and performs different actions
# based on the device node name/type -- similar to udev rules.

if [ "x${MDEV:0:2}" = "xsd" ]; then
    # For SCSI disks, we attempt to create a link to the SCSI disk
    # using the ID returned by the scsi_id.sh script; if it returns
    # non-zero then do nothing.
    scsi_id="$(/usr/local/sbin/scsi_id.sh /dev/${MDEV} 2>&1)"
    if [ ${?} -eq 0 ]; then
        mkdir -m 0755 -p /dev/disk-by-id
        ln -s /dev/${MDEV} /dev/disk-by-id/${scsi_id}
    fi
    # SCSI disks that are part of a bcache device need to be registered;
    # in linux-next they include uevents so we can make a unique link.
    bcache_probe="$(/usr/sbin/probe-bcache -o udev /dev/${MDEV} 2>&1)"
    if [ "x${bcache_probe}" != "x" ]; then
        export ${bcache_probe}
        if [ "x${ID_FS_TYPE}" = "xbcache" ]; then
            echo "/dev/${MDEV}" > /sys/fs/bcache/register_quiet
        fi
    fi
fi
