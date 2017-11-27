#! /bin/sh

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem using rsync. The '-i'
# ('--initial') flag should only be used on boot to perform the initial
# configuration sync (from USB to root tmpfs).

CONF_MNT="/mnt/conf"
USB_RSYNC="${CONF_MNT}/rsync_dirs"
ETCKEEPER_REPO="${CONF_MNT}/etckeeper.git"
RSYNC_DIRS="/var/lib /opt" # These are absolute paths (leading '/' required)
INITIAL_SYNC=0
ROOT_PATH="/"

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
mount ${CONF_MNT} || exit 1
if [ ${INITIAL_SYNC} -eq 1 ]; then
    if git ls-remote ${ETCKEEPER_REPO} > /dev/null 2>&1; then
        cd /etc && git init -q || exit 1
        cd /etc && git remote add origin ${ETCKEEPER_REPO} || exit 1
        cd /etc && git fetch -q origin master || exit 1
        cd /etc && git checkout -q -f -b master --track origin/master || exit 1
        cd /etc && git reset -q --hard origin/master || exit 1
    else
        git init -q --bare ${ETCKEEPER_REPO} || exit 1
	echo -en "# Specific to ESOS\n/rc.d/\n/esos-release\n/issue\n\n" > \
            /etc/.gitignore || exit 1
        etckeeper init > /dev/null || exit 1
        git config --system user.name "ESOS Superuser" || exit 1
	git config --system user.email "root@esos" || exit 1
        cd /etc && git commit -q -m "initial check-in" > /dev/null || exit 1
        cd /etc && git remote add origin ${ETCKEEPER_REPO} > /dev/null || exit 1
	cd /etc && git push -q origin master > /dev/null || exit 1
    fi
    mkdir -p ${USB_RSYNC} || exit 1
    rsync --archive --exclude "System Volume Information" \
        --exclude "lost+found" ${USB_RSYNC}/ ${ROOT_PATH} || exit 1
else
    cd /etc && git push -q origin master || exit 1
    rsync --archive --relative --delete ${RSYNC_DIRS} ${USB_RSYNC} || exit 1
fi
umount ${CONF_MNT} || exit 1

