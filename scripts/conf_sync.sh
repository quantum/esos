#! /bin/sh

# Don't do anything if lock exists
if [ -f "/tmp/conf_sync_lock" ]; then
    echo "Lock file exists, so we're not sync'ing!" 1>&2
    exit 1
fi
# Commit any changed files to the local Git repo
cd /etc && /usr/bin/git add -A || exit 1
cd /etc && /usr/bin/git diff-index --quiet HEAD || \
    /usr/bin/git commit -q -m "configuration sync via conf_sync.sh" || exit 1
# Synchronize the local configuration with the USB flash drive
/usr/local/sbin/usb_sync.sh || exit 1

