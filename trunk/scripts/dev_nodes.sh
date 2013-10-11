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
elif [ "x${MDEV:0:3}" = "xdlm" ]; then
    # For DLM device nodes, we simply make a link to the given device
    # node in the /dev/misc directory.
    mkdir -m 0755 -p /dev/misc
    ln -s /dev/${MDEV} /dev/misc/${MDEV}
elif [ "x${MDEV:0:4}" = "xumad" ] ||
    [ "x${MDEV:0:4}" = "xissm" ] ||
    [ "x${MDEV:0:6}" = "xuverbs" ]; then
    # With IB device nodes, simply create a link in /dev/infiniband for
    # each (most applications seem to expect this location).
    mkdir -m 0755 -p /dev/infiniband
    ln -s /dev/${MDEV} /dev/infiniband/${MDEV}
fi
