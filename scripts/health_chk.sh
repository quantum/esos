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
	adp_count=`${MEGACLI} -adpCount | grep "Controller Count:" | \
		cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n'`
	for adapter in `seq 0 $(expr ${adp_count} - 1)`; do
		# Get the number of logical drives for the adapter
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
