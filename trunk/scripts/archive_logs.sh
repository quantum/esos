#! /bin/sh

# $Id$

# Dump logs files unconditionally to the ESOS USB device (esos_logs).
# Not sure if moving log files and sending a SIGHUP to syslogd is sufficient (or ideal)...

LOGS_MNT="/mnt/logs"
TMP_DIR="/tmp"

mount ${LOGS_MNT} || exit 1
archive_prefix="`hostname`_`date +%F`_`date +%s`"
mkdir -m 0755 -p ${TMP_DIR}/${archive_prefix} || exit 1

# Move log files to temp. location and archive to USB
mv -f /var/log/* ${TMP_DIR}/${archive_prefix}/
tar cz -f ${LOGS_MNT}/${archive_prefix}.tar.gz -C ${TMP_DIR} ${archive_prefix}

# Make syslogd happy again
killall -q -SIGHUP syslogd

rm -rf ${TMP_DIR}/${archive_prefix}
umount ${LOGS_MNT} || exit 1
