#! /bin/sh

# Use this script to create a full backup/archive of the entire ESOS
# configuration that resides on the persistent boot media file system
# (LABEL=esos_conf). The archive is created in the /tmp directory and
# the absolute path of the file name is returned.

RELEASE="$(sed -r "s/ +/-/g" /etc/esos-release)"
HOSTNAME="$(hostname)"
ARCHIVE_NAME="esos_backup_conf-$(date +%s)@${HOSTNAME}@${RELEASE}"
ARCHIVE_DIR="/tmp/${ARCHIVE_NAME}"
ARCHIVE_FILE="${ARCHIVE_DIR}.tgz"
SYNC_LOCK="/var/lock/conf_sync"
CONF_MNT="/mnt/conf"

# We use a temporary directory while collecting the data
mkdir -p ${ARCHIVE_DIR} || exit 1

# Sync the config here for good measure
conf_sync.sh || exit 1

# Prevent conf_sync.sh from running
exec 200> "${SYNC_LOCK}"
flock --timeout 300 -E 200 -x 200
RC=${?}
if [ ${RC} -ne 0 ]; then
    echo "ERROR: Could not acquire conf_sync lock (RC=${RC})," \
        "so we're not backing up!" 1>&2
    exit ${RC}
fi

if ! grep -q esos_persist /proc/cmdline; then
    # Mount the ESOS config FS
    mount ${CONF_MNT} || exit 1
fi

# Grab all items from the config - exclude stuff under /opt
rsync -aq ${CONF_MNT}/* --exclude=rsync_dirs/opt ${ARCHIVE_DIR} || exit 1

# Make the tarball
tar cpfz ${ARCHIVE_FILE} --transform 's,^tmp/,,' \
    ${ARCHIVE_DIR} 2> /dev/null || exit 1

# All done
if ! grep -q esos_persist /proc/cmdline; then
    umount ${CONF_MNT} || exit 1
fi
rm -rf ${ARCHIVE_DIR} || exit 1
echo "${ARCHIVE_FILE}"
