#! /bin/sh

# $Id$

# System startup script; this script will check for crash dump files, grab some
# information from the ESOS system, and then emails the root user everything.

EMAIL_TO="root"
EMAIL_FROM="root"
LOGS_MNT="/mnt/logs"

# Check for kernel crash dump files on the esos_logs filesystem
mount ${LOGS_MNT}
if ls ${LOGS_MNT}/*dumpfile* > /dev/null 2>&1; then
	dump_files=""
	for i in `ls ${LOGS_MNT}/*dumpfile*`; do
		dump_files="${dump_files}${i}\n"
	done
fi
umount ${LOGS_MNT}

# Send the email message
sendmail -t << _EOF_
To: ${EMAIL_TO}
From: ${EMAIL_FROM}
Subject: ESOS System Startup - `hostname` (`date`)
Enterprise Storage OS on host "`hostname`" has started. If this system startup is expected, you can probably ignore this.

`test "${dump_files}" != "" && echo "** Warning! Kernel crash dump file(s) detected:"; echo -e "${dump_files}"`

`test -d "/sys/kernel/scst_tgt" && scstadmin -list_target`
`test -d "/sys/kernel/scst_tgt" && scstadmin -list_device`
_EOF_
