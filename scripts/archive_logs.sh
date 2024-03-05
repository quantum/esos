#! /bin/bash

# Dump logs files to the ESOS USB device (esos_logs). Check for free space on
# the esos_logs file system, and if needed, delete log archives starting with
# oldest until we have enough free space.

# Settings
LOGS_MNT="/mnt/logs"
LOG_DIR="/var/log"
TMP_DIR="/tmp"
NO_MV_FILES="boot pacemaker.log abyss.log node_exporter.log \
libvirtd_exporter.log prometheus.log"

# Make sure we always unmount (or try to)
function unmount_fs {
    umount "${LOGS_MNT}" || exit 1
}
if ! grep -q esos_persist /proc/cmdline; then
    trap unmount_fs EXIT
fi

# Check free space; this is in KiB (1 KiB blocks), not kilobytes
check_free() {
    avail_space="$(df -k "${LOGS_MNT}" | grep -v '^Filesystem' | \
        awk '{print $4}')"
    # Reserve 128 MiB so things aren't too tight
    avail_space=$((avail_space - 131072))
    echo -n "${avail_space}"
}

# Mount the file system and set a file prefix
if awk '{print $2}' /proc/mounts | grep -q "${LOGS_MNT}"; then
    if ! grep -q esos_persist /proc/cmdline; then
        logger -s -t "$(basename "${0}")" -p "local4.warn" \
            "It appears '${LOGS_MNT}' is already mounted! Continuing anyway..."
    fi
else
    mount "${LOGS_MNT}" || exit 1
fi
archive_prefix="$(hostname)_$(date +%F)_$(date +%s)"

# Archive the logs -- we should only fail if there is no room in /tmp (tmpfs)
# shellcheck disable=SC2174
mkdir -m 0755 -p "${TMP_DIR}/${archive_prefix}" || exit 1
ignore_files=""
for i in ${NO_MV_FILES}; do
    # shellcheck disable=SC2125
    ignore_files="${ignore_files} ! -path "*/${i}""
done
# shellcheck disable=SC2086
while read -r i; do
    [[ -n "${i}" ]] || break
    base_dir="$(dirname "${i}")"
    base_dir="${base_dir//"${LOG_DIR}"/}"
    mkdir -p "${TMP_DIR}/${archive_prefix}/${base_dir}" || exit 1
    mv -f "${i}" "${TMP_DIR}/${archive_prefix}/${base_dir}/" || exit 1
done <<< "$(find ${LOG_DIR} -type f ${ignore_files})"
for i in ${NO_MV_FILES}; do
    # TODO: Need a better solution, we're losing log lines using cp + truncate!
    file_path="${LOG_DIR}/${i}"
    if [ -f "${file_path}" ]; then
        base_dir="$(dirname "${file_path}")"
        base_dir="${base_dir//"${LOG_DIR}"/}"
        mkdir -p "${TMP_DIR}/${archive_prefix}/${base_dir}" || exit 1
        cp -p "${file_path}" "${TMP_DIR}/${archive_prefix}/${base_dir}/" \
            || exit 1
        truncate -s 0 "${file_path}" || exit 1
    fi
done
file_path="${TMP_DIR}/${archive_prefix}.tar.gz"
tar cpfz "${file_path}" -C "${TMP_DIR}" "${archive_prefix}" || exit 1
new_arch_size="$(du -k "${file_path}" | awk '{print $1}')"

# Make sure this is even feasible
total_space=$(df -k ${LOGS_MNT} | grep -v '^Filesystem' | awk '{print $2}')
if [ "${new_arch_size}" -gt "${total_space}" ]; then
    echo "The new log archive (${file_path}) will not fit on" \
        "the ${LOGS_MNT} file system." 1>&2
    exit 1
fi

# Get rid of old files (if needed) until we have enough space free
while [ "${new_arch_size}" -gt "$(check_free)" ]; do
    find ${LOGS_MNT} -type f ! -path "${LOGS_MNT}/var_log/*" -print0 | \
        sort -z | xargs -0 ls | head -n 1 | xargs rm
done

# Move the new archive to USB
mv -f "${file_path}" "${LOGS_MNT}/" || exit 1

# Make syslogd happy again
killall -q -SIGHUP syslogd

# Re-create some files
touch ${LOG_DIR}/wtmp
touch ${LOG_DIR}/lastlog

# Done
rm -rf "${TMP_DIR:?}/${archive_prefix}"

