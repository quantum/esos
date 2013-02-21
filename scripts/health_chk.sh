#! /bin/sh

# $Id$

# This script should be run as a cron job on a regular interval. It will
# perform several system checks such as available disk space, free physical
# RAM check, and check logical drives' status/state.

MEGACLI="/opt/sbin/MegaCli64"
ARCCONF="/opt/sbin/arcconf"
MEM_PRCT_THRESH=0.75
DISK_PRCT_THRESH=0.80
CHK_FS_LABEL="esos_root"
EMAIL_TO="root"
EMAIL_FROM="root"
TMP_PATH="/tmp"

# Check MegaRAID logical drives (if any)
if [ -x "${MEGACLI}" ]; then
	echo "Starting MegaRAID logical drive checks..."
	# Get the number of adapters on the system
	# Adapter numbers start with 0 for MegaRAID
	adp_count=`${MEGACLI} -adpCount -NoLog | grep "Controller Count:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n'`
	echo "Number of adapters found: ${adp_count}"
	for adapter in `seq 0 $(expr ${adp_count} - 1)`; do
		# Get the number of logical drives for the adapter
		# Logical drive numbers start with 0 for MegaRAID
		ld_count=`${MEGACLI} -AdpAllInfo -a${adapter} -NoLog | \
			grep "Virtual Drives    :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
		echo "Adapter ${adapter} has ${ld_count} logical drive(s)."
		for logical_drv in `seq 0 $(expr ${ld_count} - 1)`; do
			ld_state=`${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} -NoLog | \
				grep "State               :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
			if [ "${ld_state}" != "Optimal" ]; then
				echo "** Warning! MegaRAID logical drive ${logical_drv} on adapter ${adapter} is not optimal!" 1>&2
				${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} -NoLog 1>&2
			fi
		done
	done
else
	echo "It appears the '${MEGACLI}' tool is not installed, or at least"
	echo "is not executable. Skipping MegaRAID logical drive checks..."
fi

# Check AACRAID logical drives (if any)
if [ -x "${ARCCONF}" ]; then
	echo "Starting AACRAID logical drive checks..."
	# Get the number of adapters on the system
	# Adapter numbers start with 1 for AACRAID
	adp_count=`${ARCCONF} GETVERSION nologs | grep "Controllers found:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '\n'`
	echo "Number of adapters found: ${adp_count}"
	for adapter in `seq 1 ${adp_count}`; do
		# Get the number of logical drives for the adapter
		# Logical drive numbers start with 0 for AACRAID
		ld_count=`${ARCCONF} GETCONFIG ${adapter} AD nologs | \
			grep "Logical devices/Failed/Degraded          :" | \
			cut -d: -f2 | cut -d/ -f1 | tr -d ' ' | tr -d '\n'`
		echo "Adapter ${adapter} has ${ld_count} logical drive(s)."
		for logical_drv in `seq 0 $(expr ${ld_count} - 1)`; do
			ld_state=`${ARCCONF} GETCONFIG ${adapter} LD ${logical_drv} nologs | \
				grep "Status of logical device                 :" | cut -d: -f2 | \
				tr -d ' ' | tr -d '\n'`
			if [ "${ld_state}" != "Optimal" ]; then
				echo "** Warning! AACRAID logical drive ${logical_drv} on adapter ${adapter} is not optimal!" 1>&2
				${ARCCONF} GETCONFIG ${adapter} LD ${logical_drv} nologs 1>&2
			fi
		done
	done
else
	echo "It appears the '${ARCCONF}' tool is not installed, or at least"
	echo "is not executable. Skipping AACRAID logical drive checks..."
fi

# Check physical RAM
mem_total=`cat /proc/meminfo | grep "^MemTotal:" | awk '{print $2}'`
mem_free=`cat /proc/meminfo | grep "^MemFree:" | awk '{print $2}'`
mem_cached=`cat /proc/meminfo | grep "^Cached:" | awk '{print $2}'`
mem_mapped=`cat /proc/meminfo | grep "^Mapped:" | awk '{print $2}'`
mem_avail=`expr ${mem_free} + ${mem_cached} - ${mem_mapped}`
mem_used=`expr ${mem_total} - ${mem_avail}`
echo "Physical RAM check..."
echo -e "Total Memory:\t\t${mem_total} kB\nUsed Memory:\t\t${mem_used} kB\nAvailable Memory:\t${mem_avail} kB"
prct_mem_used=`echo "${mem_used} ${mem_total}" | awk '{ printf("%.1g", $1 / $2) }'`
echo "Memory used percent: ${prct_mem_used}"
if expr ${prct_mem_used} '>' ${MEM_PRCT_THRESH} > /dev/null; then
	echo "** Warning! Maximum memory used threshold (${MEM_PRCT_THRESH}) has been exceeded..." 1>&2
	echo "Total Physical RAM: ${mem_total} kB" 1>&2
	echo "Available Physical RAM: ${mem_free} kB" 1>&2
fi

# Check disk space (well, tmpfs root FS space)
disk_total=`df -m / | grep tmpfs | awk '{print $2}'`
disk_used=`df -m / | grep tmpfs | awk '{print $3}'`
disk_avail=`df -m / | grep tmpfs | awk '{print $4}'`
echo "Disk (/ -> root tmpfs) space check..."
echo -e "Total Disk Space:\t${disk_total} MB\nUsed Disk Space:\t${disk_used} MB\nAvail. Disk Space:\t${disk_avail} MB"
prct_disk_used=`echo "${disk_used} ${disk_total}" | awk '{ printf("%.1g", $1 / $2) }'`
echo "Disk space used percent: ${prct_disk_used}"
if expr ${prct_disk_used} '>' ${DISK_PRCT_THRESH} > /dev/null; then
	echo "** Warning! Maximum disk space used threshold (${DISK_PRCT_THRESH}) has been exceeded..." 1>&2
	echo "Total Disk Space: ${disk_total} MB" 1>&2
	echo "Avail. Disk Space: ${disk_avail} MB" 1>&2
fi

# Check if the USB drive is available/working via one of the FS labels (no indentation for if statement)
if ! findfs LABEL=${CHK_FS_LABEL} > /dev/null 2>&1; then
# Create a archive of the configuration files
arch_pkg_file="esos_conf_pkg-`date +%s`.tgz"
arch_pkg_path="${TMP_PATH}/${arch_pkg_file}"
tar cpfz ${arch_pkg_path} --exclude='rc.d' --exclude='ssh_host_*' --exclude='shadow*' /etc > /dev/null 2>&1
# Send an email with the archive file attachment (uuencode'd)
sendmail -t << _EOF_
To: ${EMAIL_TO}
From: ${EMAIL_FROM}
Subject: ESOS USB Flash Drive Failure - `hostname` (`date`)
A possible USB flash drive failure has been detected on Enterprise Storage OS host "`hostname`".

The findfs utility exited non-zero when attempting to resolve file system label "${CHK_FS_LABEL}". This may be due to a failed ESOS USB flash drive, or because the device was removed, or some other reason.

We're attaching a tar ball archive of the ESOS configuration files for this host just incase.

`uuencode ${arch_pkg_path} ${arch_pkg_file}`
_EOF_
rm -f ${arch_pkg_path}
fi
