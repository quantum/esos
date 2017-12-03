#! /bin/sh

# Write the SCST configuration to a file (we don't hide stderr)
/usr/sbin/scstadmin -force -nonkey -write_config /etc/scst.conf > /dev/null
# Commit any changed files to the local Git repo
cd /etc && /usr/bin/git add -A || exit 1
cd /etc && /usr/bin/git diff-index --quiet HEAD || \
    /usr/bin/git commit -q -m "configuration sync via conf_sync.sh" || exit 1
# Synchronize the local configuration with the USB flash drive
/usr/local/sbin/usb_sync.sh || exit 1

