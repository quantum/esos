#! /bin/sh

# $Id$

# This script is called by mdev and attempts to create a link to
# the SCSI disk using the ID via the sg_vpd utility. If sg_vpd
# fails (eg, no VPD page for the device) then do nothing.

scsi_id=`sg_vpd -i -q /dev/${MDEV} 2>&1`
if [ $? = "0" ]; then
	mkdir -m 0755 -p /dev/disk-by-id
	ln -s /dev/${MDEV} /dev/disk-by-id/${scsi_id}
fi
