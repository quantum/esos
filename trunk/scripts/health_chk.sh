#! /bin/sh

# $Id$

# This script should be run as a cron job on a regular interval. It will
# perform several system checks such as available disk space, free physical
# RAM check, and check logical drives' status/state.

MEGACLI="/opt/sbin/MegaCli64"
ARCCONF="/opt/sbin/arcconf"

# Check MegaRAID logical drives (if any)
if [ -x "${MEGACLI}" ]; then
	# Get the number of adapters on the system
	# Adapter numbers start with 0 for MegaRAID
	adp_count=`${MEGACLI} -adpCount | grep "Controller Count:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n'`
	for adapter in `seq 0 $(expr ${adp_count} - 1)`; do
		# Get the number of logical drives for the adapter
		# Logical drive numbers start with 0 for MegaRAID
		ld_count=`${MEGACLI} -AdpAllInfo -a${adapter} | \
			grep "Virtual Drives    :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
		for logical_drv in `seq 0 $(expr ${ld_count} - 1)`; do
			ld_state=`${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} | \
				grep "State               :" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
			if [ "${ld_state}" != "Optimal" ]; then
				echo "** Warning! MegaRAID logical drive ${logical_drv} on adapter ${adapter} is not optimal!" 1>&2
				${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} 1>&2
			fi
		done
	done
fi

# Check AACRAID logical drives (if any)
if [ -x "${ARCCONF}" ]; then
	# Get the number of adapters on the system
	# Adapter numbers start with 1 for AACRAID
	adp_count=`${ARCCONF} GETVERSION nologs | grep "Controllers found:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '\n'`
	for adapter in `seq 1 ${adp_count}`; do
		# Get the number of logical drives for the adapter
		# Logical drive numbers start with 0 for AACRAID
		ld_count=`${ARCCONF} GETCONFIG ${adapter} AD nologs | \
			grep "Logical devices/Failed/Degraded          :" | \
			cut -d: -f2 | cut -d/ -f1 | tr -d ' ' | tr -d '\n'`
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
fi


