#! /bin/sh

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem using rsync. The '-i'
# ('--initial') flag should only be used on boot to perform the initial
# configuration sync (from USB to root tmpfs).

CONF_MNT="/mnt/conf"
SYNC_DIRS="/etc /var/lib /opt" # These are absolute paths (leading '/' required)
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
    rsync --archive --exclude "System Volume Information" \
        --exclude lost+found ${CONF_MNT}/ ${ROOT_PATH} || exit 1
else
    rsync --archive --exclude /etc/rc.d --relative --delete \
        ${SYNC_DIRS} ${CONF_MNT} || exit 1
fi
umount ${CONF_MNT} || exit 1

