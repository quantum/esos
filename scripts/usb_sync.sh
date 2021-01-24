#! /bin/sh

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem using Git and rsync.
# The '-i' ('--initial') flag should only be used on boot to perform the
# initial configuration sync (from USB to root tmpfs).

CONF_MNT="/mnt/conf"
USB_RSYNC="${CONF_MNT}/rsync_dirs"
ETCKEEPER_REPO="${CONF_MNT}/etckeeper.git"
RSYNC_DIRS="/var/lib /opt /root/.ssh" # Absolute paths (leading '/' required)
INITIAL_SYNC=0
ROOT_PATH="/"

# Make sure we always unmount (or try to)
function unmount_fs {
    umount ${CONF_MNT} || exit 1
}
if ! grep -q esos_persist /proc/cmdline; then
    trap unmount_fs EXIT
fi

# Read the options and extract
TEMP=$(getopt -o i --long initial -n 'usb_sync.sh' -- "$@")
eval set -- "$TEMP"
while true ; do
    case "$1" in
        -i|--initial) INITIAL_SYNC=1; shift ;;
        --) shift; break ;;
        *) echo "Error extracting options!"; exit 1 ;;
    esac
done

# Mount, sync, and unmount
if cat /proc/mounts | awk '{print $2}' | grep -q ${CONF_MNT}; then
    if ! grep -q esos_persist /proc/cmdline; then
        logger -s -t $(basename ${0}) -p "local4.warn" \
            "It appears '${CONF_MNT}' is already mounted! Continuing anyway..."
    fi
else
    mount ${CONF_MNT} || exit 1
fi
if [ ${INITIAL_SYNC} -eq 1 ]; then
    if git ls-remote ${ETCKEEPER_REPO} > /dev/null 2>&1; then
        # The Git repo exists, pull it down locally
        cd /etc && git init -q || exit 1
        cd /etc && git remote add origin ${ETCKEEPER_REPO} || exit 1
        cd /etc && git fetch -q origin master || exit 1
        cd /etc && git checkout -q -f -b master --track origin/master || exit 1
        cd /etc && git reset -q --hard origin/master || exit 1
        etckeeper init > /dev/null || exit 1
    else
        # Migrate any old configuration directories
        if [ -d "${CONF_MNT}/etc" ]; then
            echo "Sync'ing previous /etc configuration locally..."
            rsync --archive --exclude etc/esos-release \
                ${CONF_MNT}/etc ${ROOT_PATH} || exit 1
            rm -rf ${CONF_MNT:?}/etc || exit 1
        fi
        if [ -d "${CONF_MNT}/var" ]; then
            echo "Moving USB /var directory to new location..."
            mkdir -p ${USB_RSYNC} || exit 1
            mv ${CONF_MNT}/var ${USB_RSYNC}/ || exit 1
        fi
        if [ -d "${CONF_MNT}/opt" ]; then
            echo "Moving USB /opt directory to new location..."
            mkdir -p ${USB_RSYNC} || exit 1
            mv ${CONF_MNT}/opt ${USB_RSYNC}/ || exit 1
        fi
        # Initialize and configure the new Git repo
        git init -q --bare ${ETCKEEPER_REPO} || exit 1
	echo -en "# Specific to ESOS\n/rc.d/\n/esos-release\n/issue\n\n" > \
            /etc/.gitignore || exit 1
        etckeeper init > /dev/null || exit 1
        git config --system user.name "ESOS Superuser" || exit 1
	git config --system user.email "root@esos" || exit 1
        cd /etc && git commit -q -m "initial check-in via usb_sync.sh" || exit 1
        cd /etc && git remote add origin ${ETCKEEPER_REPO} || exit 1
	cd /etc && git push -q origin master || exit 1
    fi
    # Use rsync for the other directories/files
    mkdir -p ${USB_RSYNC} || exit 1
    rsync --archive --update --exclude "System Volume Information" \
        --exclude "lost+found" ${USB_RSYNC}/ ${ROOT_PATH} || exit 1
else
    # During an upgrade, the user may wipe esos_conf, so recreate if needed
    if ! git ls-remote ${ETCKEEPER_REPO} > /dev/null 2>&1; then
        git init -q --bare ${ETCKEEPER_REPO} || exit 1
    fi
    if [ ! -d "${USB_RSYNC}" ]; then
        mkdir -p ${USB_RSYNC} || exit 1
    fi
    # Push changes up to the USB Git repo / file system
    cd /etc && git push -q origin master || exit 1
    rsync --archive --relative --delete ${RSYNC_DIRS} ${USB_RSYNC} || exit 1
fi

