#! /bin/sh

# Write the SCST configuration to a file (we don't hide stderr)
/usr/sbin/scstadmin -force -nonkey -write_config /etc/scst.conf > /dev/null
# Commit any changed files to the local Git repo
cd /etc && /usr/bin/git add . || exit 1
/usr/bin/git --git-dir=/etc/.git commit -m "config sync" || exit 1
# Synchronize the local configuration with the USB flash drive
/usr/local/sbin/usb_sync.sh || exit 1
