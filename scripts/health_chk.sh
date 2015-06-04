#! /bin/sh

# This script should be run as a cron job on a regular interval. It will
# perform several system checks such as available disk space, free physical
# RAM check, and check logical drives' status/state.

MEGACLI="/opt/sbin/MegaCli64"
ARCCONF="/opt/sbin/arcconf"
ZPOOL="/usr/sbin/zpool"
MEM_PRCT_THRESH=0.90
DISK_PRCT_THRESH=0.80
CHK_FS_LABEL="esos_root"
EMAIL_TO="root"
EMAIL_FROM="root"
TMP_PATH="/tmp"

# Check MegaRAID logical drives (if any)
if [ -x "${MEGACLI}" ]; then
    echo "Starting MegaRAID health checks..."
    # Get the number of adapters on the system
    # Adapter numbers start with 0 for MegaRAID
    adp_count=`${MEGACLI} -adpCount -NoLog | grep "Controller Count:" | \
        cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n'`
    echo "Number of adapters found: ${adp_count}"
    for adapter in `seq 0 $(expr ${adp_count} - 1)`; do
        # Get the number of logical drives for the adapter
        # Logical drive numbers start with 0 for MegaRAID
        echo "Checking logical drives..."
        ld_count=`${MEGACLI} -AdpAllInfo -a${adapter} -NoLog | \
            grep "Virtual Drives" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
        if [ ${ld_count} -gt 0 ]; then
            echo "Adapter ${adapter} has ${ld_count} logical drive(s)."
            ld_list=`${MEGACLI} -LDInfo -Lall -a${adapter} -NoLog | \
                grep "Virtual Drive:" | sed 's/CacheCade //g' | cut -d" " -f3`
            for logical_drv in ${ld_list}; do
                ld_state=`${MEGACLI} -LDInfo -L${logical_drv} -a${adapter} -NoLog | \
                    grep "State" | cut -d: -f2 | tr -d ' ' | tr -d '\n'`
                if [ "${ld_state}" != "Optimal" ]; then
                    echo "** Warning! MegaRAID logical drive ${logical_drv} on" \
                        "adapter ${adapter} is not optimal!" 1>&2
                    echo "** Logical drive state: ${ld_state}" 1>&2
                fi
            done
        fi
        # Check the physical drives for the adapter
        echo "Checking physical drives..."
        pd_count=0
        SAVED_IFS=${IFS}
        IFS=$(echo -en "\n\b")
        for i in `${MEGACLI} -PDList -a${adapter} -NoLog`; do
            if echo "${i}" | grep "Firmware state:" > /dev/null 2>&1; then
                pd_count=$(expr ${pd_count} + 1)
                drv_state=`echo "${i}" | cut -d: -f2 | sed 's/^ *//' | tr -d '\n'`
                if [ "${drv_state}" != "Unconfigured(good), Spun Up" ] &&
                    [ "${drv_state}" != "Online, Spun Up" ] &&
                    [ "${drv_state}" != "Hotspare, Spun Up" ]; then
                    echo "** Warning! It appears a MegaRAID physical drive has" \
                        "failed on adapter ${adapter}!" 1>&2
                    echo "** Physical drive state: ${drv_state}" 1>&2
                fi
            fi
        done
        IFS=${SAVED_IFS}
        echo "Checked ${pd_count} physical drive(s)."
        # Check the status of attached enclosures
        echo "Checking enclosures..."
        SAVED_IFS=${IFS}
        IFS=$(echo -en "\n\b")
        for i in `${MEGACLI} -EncStatus -a${adapter} -NoLog`; do
            if echo "${i}" | grep "Slot Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Power Supply Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Fan Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Temperature Sensor Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "SIM Module Status" > /dev/null 2>&1; then
                line_name=`echo "${i}" | cut -d: -f1 | sed 's/ *$//' | tr -d '\n'`
                line_status=`echo "${i}" | cut -d: -f2 | sed 's/^ *//' | tr -d '\n'`
                if [ "${line_status}" != "OK" ] &&
                    [ "${line_status}" != "Not Installed" ] &&
                    [ "${line_status}" != "Unknown" ] &&
                    [ "${line_status}" != "Unsupported" ] &&
                    [ "${line_status}" != "Not Available" ]; then
                    echo "** Warning! An enclosure health/status issue" \
                        "has been detected on adapter ${adapter}!" 1>&2
                    echo "** ${line_name} -> ${line_status}" 1>&2
                fi
            fi
        done
        IFS=${SAVED_IFS}
    done
else
    echo "It appears the '${MEGACLI}' tool is not installed, or at least"
    echo "is not executable. Skipping MegaRAID logical drive checks..."
fi

# Check AACRAID logical drives (if any)
if [ -x "${ARCCONF}" ]; then
    echo "Starting AACRAID health checks..."
    # Get the number of adapters on the system
    # Adapter numbers start with 1 for AACRAID
    adp_count=`${ARCCONF} GETVERSION nologs | grep "Controllers found:" | \
        cut -d: -f2 | tr -d ' ' | tr -d '\n'`
    echo "Number of adapters found: ${adp_count}"
    for adapter in `seq 1 ${adp_count}`; do
        # Get the number of logical drives for the adapter
        # Logical drive numbers start with 0 for AACRAID
        ld_count=`${ARCCONF} GETCONFIG ${adapter} AD nologs | \
            grep "Logical devices/Failed/Degraded" | \
            cut -d: -f2 | cut -d/ -f1 | tr -d ' ' | tr -d '\n'`
        if [ ${ld_count} -gt 0 ]; then
            echo "Adapter ${adapter} has ${ld_count} logical drive(s)."
            ld_list=`${ARCCONF} GETCONFIG ${adapter} LD nologs | \
                grep "Logical device number" | cut -d" " -f4`
            for logical_drv in ${ld_list}; do
                ld_state=`${ARCCONF} GETCONFIG ${adapter} LD ${logical_drv} nologs | \
                    grep "Status of logical device" | cut -d: -f2 | \
                    tr -d ' ' | tr -d '\n'`
                if [ "${ld_state}" != "Optimal" ]; then
                    echo "** Warning! AACRAID logical drive ${logical_drv} on" \
                        "adapter ${adapter} is not optimal!" 1>&2
                    echo "** Logical drive state: ${ld_state}" 1>&2
                fi
            done
        fi
        # Check the physical drives for the adapter
        echo "Checking physical drives..."
        pd_count=0
        SAVED_IFS=${IFS}
        IFS=$(echo -en "\n\b")
        for i in `${ARCCONF} GETCONFIG ${adapter} PD nologs`; do
            if echo "${i}" | grep "State                              :" > /dev/null 2>&1; then
                pd_count=$(expr ${pd_count} + 1)
                drv_state=`echo "${i}" | cut -d: -f2 | sed 's/^ *//' | tr -d '\n'`
                if [ "${drv_state}" != "Online" ] &&
                    [ "${drv_state}" != "Ready" ] &&
                    [ "${drv_state}" != "Hot Spare" ]; then
                    echo "** Warning! It appears a AACRAID physical drive has" \
                        "failed on adapter ${adapter}!" 1>&2
                    echo "** Physical drive state: ${drv_state}" 1>&2
                fi
            fi
        done
        IFS=${SAVED_IFS}
        echo "Checked ${pd_count} physical drive(s)."
        # TODO: Need to implement enclosure status check.
    done
else
    echo "It appears the '${ARCCONF}' tool is not installed, or at least"
    echo "is not executable. Skipping AACRAID logical drive checks..."
fi

# Check ZFS pools
if [ -x "${ZPOOL}" ]; then
    echo "Checking ZFS pool health..."
    SAVED_IFS=${IFS}
    IFS=$(echo -en "\n\b")
    for line in `${ZPOOL} get -H health`; do
        pool_name=`echo "${line}" | awk '{print $1}'`
        pool_state=`echo "${line}" | awk '{print $3}'`
        if [ "${pool_state}" != "ONLINE" ]; then
            echo "ZFS pool '${pool_name}' is offline or degraded!" 1>&2
        fi
    done
    IFS=${SAVED_IFS}
else
    echo "It appears the '${ZPOOL}' tool is not installed, or at least"
    echo "is not executable. Skipping ZFS pool checks..."
fi

# Check physical RAM
mem_total=`cat /proc/meminfo | grep "^MemTotal:" | awk '{print $2}'`
mem_avail=`cat /proc/meminfo | grep "^MemAvailable:" | awk '{print $2}'`
mem_used=`expr ${mem_total} - ${mem_avail}`
echo "Physical RAM check..."
echo -e "Total Memory:\t\t${mem_total} kB\nUsed Memory:\t\t${mem_used} kB\nAvailable Memory:\t${mem_avail} kB"
prct_mem_used=`echo "${mem_used} ${mem_total}" | awk '{ printf("%.1g", $1 / $2) }'`
echo -e "Memory used percent:\t${prct_mem_used}"
if expr ${prct_mem_used} '>' ${MEM_PRCT_THRESH} > /dev/null; then
    echo "** Warning! Maximum memory used threshold (${MEM_PRCT_THRESH}) has been exceeded..." 1>&2
    echo "Total Physical RAM: ${mem_total} kB" 1>&2
    echo "Available Physical RAM: ${mem_avail} kB" 1>&2
fi

# Check disk space (well, tmpfs root FS space)
disk_total=`df -m / | grep tmpfs | awk '{print $2}'`
disk_used=`df -m / | grep tmpfs | awk '{print $3}'`
disk_avail=`df -m / | grep tmpfs | awk '{print $4}'`
echo "Disk (/ -> root tmpfs) space check..."
echo -e "Total Disk Space:\t${disk_total} MB\nUsed Disk Space:\t${disk_used} MB\nAvail. Disk Space:\t${disk_avail} MB"
prct_disk_used=`echo "${disk_used} ${disk_total}" | awk '{ printf("%.1g", $1 / $2) }'`
echo -e "Disk used percent:\t${prct_disk_used}"
if expr ${prct_disk_used} '>' ${DISK_PRCT_THRESH} > /dev/null; then
    echo "** Warning! Maximum disk space used threshold (${DISK_PRCT_THRESH}) has been exceeded..." 1>&2
    echo "Total Disk Space: ${disk_total} MB" 1>&2
    echo "Avail. Disk Space: ${disk_avail} MB" 1>&2
fi

# Check if the USB drive is available/working via one of the FS labels (no indentation for if statement)
if ! findfs LABEL=${CHK_FS_LABEL} > /dev/null 2>&1; then
# Create a archive of the configuration files
arch_pkg_file="`hostname`-esos_conf-`date +%s`.tgz"
arch_pkg_path="${TMP_PATH}/${arch_pkg_file}"
tar cpfz ${arch_pkg_path} --exclude='rc.d' --exclude='ssh' --exclude='shadow*' /etc > /dev/null 2>&1
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
