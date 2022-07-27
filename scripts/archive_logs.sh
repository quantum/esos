#! /bin/sh

# Dump logs files to the ESOS USB device (esos_logs). Check for free space on
# the esos_logs file system, and if needed, delete log archives starting with
# oldest until we have enough free space.

# Settings
LOGS_MNT="/mnt/logs"
LOG_DIR="/var/log"
TMP_DIR="/tmp"

# Make sure we always unmount (or try to)
function unmount_fs {
    umount ${LOGS_MNT} || exit 1
}
if ! grep -q esos_persist /proc/cmdline; then
    trap unmount_fs EXIT
fi

# Check free space; this is in KiB (1 KiB blocks), not kilobytes
check_free() {
    avail_space=$(df -k ${LOGS_MNT} | grep -v '^Filesystem' | \
        awk '{print $4}')
    echo -n ${avail_space}
}

# Mount the file system and set a file prefix
if cat /proc/mounts | awk '{print $2}' | grep -q ${LOGS_MNT}; then
    if ! grep -q esos_persist /proc/cmdline; then
        logger -s -t $(basename ${0}) -p "local4.warn" \
            "It appears '${LOGS_MNT}' is already mounted! Continuing anyway..."
    fi
else
    mount ${LOGS_MNT} || exit 1
fi
archive_prefix="`hostname`_`date +%F`_`date +%s`"

# Archive the logs -- we should only fail if there is no room in /tmp (tmpfs)
mkdir -m 0755 -p ${TMP_DIR}/${archive_prefix} || exit 1
find ${LOG_DIR} -type f ! -path "*/boot" ! -path "*/pacemaker.log" \
    ! -path "*/abyss.log" ! -path "*/node_exporter.log" \
    ! -path "*/libvirtd_exporter.log" ! -path "*/prometheus.log" \
    -exec mv -f {} ${TMP_DIR}/${archive_prefix}/ \; || exit 1
for i in ${LOG_DIR}/boot ${LOG_DIR}/pacemaker.log ${LOG_DIR}/abyss.log \
    ${LOG_DIR}/node_exporter.log ${LOG_DIR}/libvirtd_exporter.log \
    ${LOG_DIR}/prometheus.log; do
    # TODO: Need a better solution, we're losing log lines using cp + truncate!
    if [ -f "${i}" ]; then
        cp -p ${i} ${TMP_DIR}/${archive_prefix}/ || exit 1
        truncate -s 0 ${i} || exit 1
    fi
done
file_path="${TMP_DIR}/${archive_prefix}.tar.gz"
tar cpfz ${file_path} -C ${TMP_DIR} ${archive_prefix} || exit 1
new_arch_size=$(du -k ${file_path} | awk '{print $1}')

# Make sure this is even feasible
total_space=$(df -k ${LOGS_MNT} | grep -v '^Filesystem' | awk '{print $2}')
if [ ${new_arch_size} -gt ${total_space} ]; then
    echo "The new log archive (${file_path}) will not fit on" \
        "the ${LOGS_MNT} file system." 1>&2
    exit 1
fi

# Get rid of old files (if needed) until we have enough space free
while [ ${new_arch_size} -gt $(check_free) ]; do
    find ${LOGS_MNT} -type f ! -path "${LOGS_MNT}/var_log/*" -print0 | \
        sort -z | xargs -0 ls | head -n 1 | xargs rm
done

# Move the new archive to USB
mv -f ${file_path} ${LOGS_MNT}/ || exit 1

# Make syslogd happy again
killall -q -SIGHUP syslogd

# Re-create some files
touch ${LOG_DIR}/wtmp
touch ${LOG_DIR}/lastlog

# Done
rm -rf ${TMP_DIR:?}/${archive_prefix}

