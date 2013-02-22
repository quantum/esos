#! /bin/sh

# $Id$

# This script is called by mdev and attempts to create a link to
# the SCSI disk using the ID via the sg_vpd utility. If sg_vpd
# fails (eg, no VPD page for the device) then do nothing.

# Check the first two chracters of the 'scsi_id' variable; the
# sg_vpd utility may return 0 even when it doesn't get a proper ID.

# The sg_vpd tool may return multiple device identifiers for some
# devices (eg, EUI and NAA) so we only use the first line.

IFS='\n'

vpd_result="$(sg_vpd -i -q /dev/${MDEV} 2>&1)"
if [ ${?} -eq 0 ]; then
    scsi_id="$(echo ${vpd_result} | head -1)"
    if [ "${scsi_id:0:2}" == "0x" ]; then
        mkdir -m 0755 -p /dev/disk-by-id
        ln -s /dev/${MDEV} /dev/disk-by-id/${scsi_id}
    fi
fi
