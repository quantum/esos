#! /bin/sh

# $Id$

# Write the SCST configuration to a file (we don't hide stderr)
/usr/sbin/scstadmin -force -nonkey -write_config /etc/scst.conf > /dev/null || exit 1
# Synchronize the local configuration with the USB flash drive
/usr/local/sbin/usb_sync.sh || exit 1
