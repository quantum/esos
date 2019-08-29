#! /bin/sh

# Try to acquire exclusive lock for up to 5 minutes (5 * 60 = 300 seconds)
SYNC_LOCK="/var/lock/conf_sync"
exec 200> "${SYNC_LOCK}"
flock --timeout 300 -E 200 -x 200
RC=${?}
if [ ${RC} -eq 200 ]; then
    echo "ERROR: Could not acquire lock file with-in 5 minutes," \
        "so we're not sync'ing!" 1>&2
    exit ${RC}
elif [ ${RC} -ne 0 ]; then
    echo "ERROR: Could not acquire lock, so we're not sync'ing!" 1>&2
    exit ${RC}
fi

# Commit any changed files to the local Git repo
cd /etc && /usr/bin/git add -A || exit 1
cd /etc && /usr/bin/git diff-index --quiet HEAD || \
    /usr/bin/git commit -q -m "configuration sync via conf_sync.sh" || exit 1
# Synchronize the local configuration with the USB flash drive
/usr/local/sbin/usb_sync.sh || exit 1

