#! /bin/sh

# $Id$

# This script should be run as a cron job on a regular interval. It will
# perform several system checks such as available disk space, free physical
# RAM check, and check logical drives' status/state.

MEGACLI="/opt/sbin/MegaCli64"
ARCCONF="/opt/sbin/arcconf"
MEM_PRCT_THRESH=0.75
DISK_PRCT_THRESH=0.80

# Check MegaRAID logical drives (if any)
if [ -x "${MEGACLI}" ]; then
	echo "Starting MegaRAID logical drive checks..."
	# Get the number of adapters on the system
	# Adapter numbers start with 0 for MegaRAID
	adp_count=`${MEGACLI} -adpCount | grep "Controller Count:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n'`
	echo "Number of adapters found: ${adp_count}"
	for adapter in `seq 0 $(expr ${adp_count} - 1)`; do
		# Get the number of logical drives for the adapter
		# Logical drive numbers start with 0 for MegaRAID
		ld_count=`${MEGACLI} -AdpAllInfo -a${adapter} | \
			grep "Virtual Drives    :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
		echo "Adapter ${adapter} has ${ld_count} logical drive(s)."
		for logical_drv in `seq 0 $(expr ${ld_count} - 1)`; do
			ld_state=`${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} | \
				grep "State               :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
			if [ "${ld_state}" != "Optimal" ]; then
				echo "** Warning! MegaRAID logical drive ${logical_drv} on adapter ${adapter} is not optimal!" 1>&2
				${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} 1>&2
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
				grep "Status of logical device                 :" |  -f2 | \
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
mem_total=`free -m | grep "Mem:" | awk '{print $2}'`
mem_used=`free -m | grep "Mem:" | awk '{print $3}'`
mem_free=`free -m | grep "Mem:" | awk '{print $4}'`
echo "Physical RAM check..."
echo -e "Total Memory:\t${mem_total} MB\nUsed Memory:\t${mem_used} MB\nFree Memory:\t${mem_free} MB"
prct_mem_used=`echo "${mem_used} ${mem_total}" | awk 'BEGIN { printf("%.1g", $1 / $2) }'`
echo "Memory used percent: ${prct_mem_used}"
if expr ${prct_mem_used} '>' ${MEM_PRCT_THRESH} > /dev/null; then
	echo "** Warning! Maximum memory used threshold (${MEM_PRCT_THRESH}) has been exceeded..." 1>&2
	echo "Total Physical RAM: ${mem_total} MB" 1>&2
	echo "Free Physical RAM: ${mem_free} MB" 1>&2
fi

# Check disk space (well, tmpfs root FS space)
disk_total=`df -m / | grep tmpfs | awk '{print $2}'`
disk_used=`df -m / | grep tmpfs | awk '{print $3}'`
disk_avail=`df -m / | grep tmpfs | awk '{print $4}'`
echo "Disk (/ -> root tmpfs) space check..."
echo -e "Total Disk Space:\t${disk_total} MB\nUsed Disk Space:\t${disk_used} MB\nAvail. Disk Space:\t${disk_avail} MB"
prct_disk_used=`echo "${disk_used} ${disk_total}" | awk 'BEGIN { printf("%.1g", $1 / $2) }'`
echo "Disk space used percent: ${prct_disk_used}"
if expr ${prct_disk_used} '>' ${DISK_PRCT_THRESH} > /dev/null; then
	echo "** Warning! Maximum disk space used threshold (${DISK_PRCT_THRESH}) has been exceeded..." 1>&2
	echo "Total Disk Space: ${disk_total} MB" 1>&2
	echo "Avail. Disk Space: ${disk_avail} MB" 1>&2
fi

