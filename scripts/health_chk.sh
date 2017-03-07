#! /bin/sh

# This script should be run as a cron job on a regular interval. It will
# perform several system checks such as available disk space, free physical
# RAM check, and check logical drives' status/state.

HW_RAID_CLI="/usr/local/sbin/hw_raid_cli.py"
MEGACLI="/opt/sbin/MegaCli64"
MDADM="/sbin/mdadm"
ZPOOL="/usr/sbin/zpool"
MEM_KB_MIN_THRESH=400000
MEM_PRCT_THRESH=0.90
DISK_PRCT_THRESH=0.80
CHK_FS_LABEL="esos_root"
EMAIL_TO="root"
EMAIL_FROM="root"
TMP_PATH="/tmp"


# Check logical drives (if any) for all controllers
echo "Checking hardware RAID logical drives..."
SAVED_IFS=${IFS}
IFS=$(echo -en "\n\b")
for i in $(${HW_RAID_CLI} --list-logical-drives); do
    ctrlr_type="$(echo ${i} | cut -d, -f1)"
    ctrlr_id="$(echo ${i} | cut -d, -f2)"
    ld_id="$(echo ${i} | cut -d, -f3)"
    ld_level="$(echo ${i} | cut -d, -f4)"
    ld_state="$(echo ${i} | cut -d, -f5)"
    ld_size="$(echo ${i} | cut -d, -f6)"
    ld_name="$(echo ${i} | cut -d, -f7)"
    if [[ ("${ctrlr_type}" = "MegaRAID" && "${ld_state}" != "Optl") || \
        ("${ctrlr_type}" = "PERC" && "${ld_state}" != "Optl") || \
        ("${ctrlr_type}" = "AACRAID" && "${ld_state}" != "Optimal") ]]; then
        echo "** Warning! ${ctrlr_type} logical drive # ${ld_id}" \
            "on controller # ${ctrlr_id} is not optimal!" 1>&2
        echo "** Logical drive state: ${ld_state}" 1>&2
    fi
done
IFS=${SAVED_IFS}
echo

# Check physical drives (if any) for all controllers
echo "Checking hardware RAID physical drives..."
SAVED_IFS=${IFS}
IFS=$(echo -en "\n\b")
for i in $(${HW_RAID_CLI} --list-physical-drives); do
    ctrlr_type="$(echo ${i} | cut -d, -f1)"
    ctrlr_id="$(echo ${i} | cut -d, -f2)"
    pd_encl_id="$(echo ${i} | cut -d, -f3)"
    pd_slot_num="$(echo ${i} | cut -d, -f4)"
    pd_state="$(echo ${i} | cut -d, -f5)"
    pd_size="$(echo ${i} | cut -d, -f6)"
    pd_model="$(echo ${i} | cut -d, -f7)"
    if [[ ("${ctrlr_type}" = "MegaRAID" && "${pd_state}" != "UGood" && \
        "${pd_state}" != "Onln" && "${pd_state}" != "GHS") || \
        ("${ctrlr_type}" = "PERC" && "${pd_state}" != "UGood" && \
        "${pd_state}" != "Onln" && "${pd_state}" != "GHS") || \
        ("${ctrlr_type}" = "AACRAID" && "${pd_state}" != "Ready" && \
        "${pd_state}" != "Online" && "${pd_state}" != "Hot Spare") ]]; then
        echo "** Warning! It appears a ${ctrlr_type} physical drive" \
            "(${pd_encl_id}:${pd_slot_num}) has failed on controller #" \
            "${ctrlr_id}!" 1>&2
        echo "** Physical drive state: ${pd_state}" 1>&2
    fi
done
IFS=${SAVED_IFS}
echo

# Start additional MegaRAID health checks if MegaCLI is installed
if [ -x "${MEGACLI}" ]; then
    echo "Starting additional MegaRAID health checks..."
    # Get the number of adapters on the system; adapter ID's start with 0
    adp_count=$(${MEGACLI} -adpCount -NoLog | grep "Controller Count:" | \
        cut -d: -f2 | tr -d ' ' | tr -d '.' | tr -d '\n')
    echo "Number of adapters found: ${adp_count}"
    for adapter in $(seq 0 $(expr ${adp_count} - 1)); do
        echo "Checking physical drives for media errors..."
        pd_count=0
        SAVED_IFS=${IFS}
        IFS=$(echo -en "\n\b")
        for i in $(${MEGACLI} -PDList -a${adapter} -NoLog); do
            if echo "${i}" | grep "Firmware state:" > /dev/null 2>&1; then
                pd_count=$(expr ${pd_count} + 1)
                drv_state=$(echo "${i}" | cut -d: -f2 | sed 's/^ *//' | \
                    tr -d '\n')
                # We check the firmware state above, don't check twice
            elif echo "${i}" | grep "Media Error Count:" > /dev/null 2>&1; then
                media_err=$(echo "${i}" | cut -d: -f2 | sed 's/^ *//' | \
                    tr -d '\n')
                if [ "${media_err}" != "0" ]; then
                    echo "** Warning! It appears a MegaRAID physical drive" \
                        "on adapter ${adapter} contains media errors!" 1>&2
                    echo "** Physical media error count: ${media_err}" 1>&2
                fi
            fi
        done
        IFS=${SAVED_IFS}
        echo "Checked ${pd_count} physical drive(s)."
        # Check the status of attached enclosures
        echo "Checking enclosure(s) health/state..."
        SAVED_IFS=${IFS}
        IFS=$(echo -en "\n\b")
        for i in $(${MEGACLI} -EncStatus -a${adapter} -NoLog); do
            if echo "${i}" | grep "Slot Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Power Supply Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Fan Status" > /dev/null 2>&1 ||
                echo "${i}" | grep "Temperature Sensor Status" > \
                /dev/null 2>&1 ||
                echo "${i}" | grep "SIM Module Status" > /dev/null 2>&1; then
                line_name=$(echo "${i}" | cut -d: -f1 | sed 's/ *$//' | \
                    tr -d '\n')
                line_status=$(echo "${i}" | cut -d: -f2 | sed 's/^ *//' | \
                    tr -d '\n')
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
    echo "is not executable. Skipping additional MegaRAID health checks..."
fi
echo

# Check MD arrays
if [ -x "${MDADM}" ]; then
    echo "Checking MD RAID arrays..."
    ${MDADM} --monitor --mail=root --scan --oneshot
fi
echo

# Check ZFS pools
if [ -x "${ZPOOL}" ]; then
    echo "Checking ZFS pool health..."
    SAVED_IFS=${IFS}
    IFS=$(echo -en "\n\b")
    for line in $(${ZPOOL} get -H health); do
        pool_name=$(echo "${line}" | awk '{print $1}')
        pool_state=$(echo "${line}" | awk '{print $3}')
        if [ "${pool_state}" != "ONLINE" ]; then
            echo "ZFS pool '${pool_name}' is offline or degraded!" 1>&2
        fi
    done
    IFS=${SAVED_IFS}
else
    echo "It appears the '${ZPOOL}' tool is not installed, or at least"
    echo "is not executable. Skipping ZFS pool checks..."
fi
echo

# Check physical RAM
mem_total=$(cat /proc/meminfo | grep "^MemTotal:" | awk '{print $2}')
mem_avail=$(cat /proc/meminfo | grep "^MemAvailable:" | awk '{print $2}')
mem_used=$(expr ${mem_total} - ${mem_avail})
echo "Physical RAM check..."
echo -e "Total Memory:\t\t${mem_total} kB\nUsed Memory:\t\t${mem_used}" \
    "kB\nAvailable Memory:\t${mem_avail} kB"
prct_mem_used=$(echo "${mem_used} ${mem_total}" | \
    awk '{ printf("%.1g", $1 / $2) }')
# Either use a percentage, or minimum value, whichever is lower
prct_mem_good=$(echo "1 ${MEM_PRCT_THRESH}" | awk '{ printf("%.1g", $1 - $2) }')
kb_mem_good=$(echo "${prct_mem_good} ${mem_total}" | \
    awk '{ printf("%d", $1 * $2) }')
if [ "${kb_mem_good}" -gt "${MEM_KB_MIN_THRESH}" ]; then
    # Check using the minimum kilobyte value
    if [ "${mem_avail}" -lt "${MEM_KB_MIN_THRESH}" ]; then
        echo "** Warning! Maximum memory used threshold kB value" \
            "(${MEM_KB_MIN_THRESH}) has been exceeded..." 1>&2
        echo "Total Physical RAM: ${mem_total} kB" 1>&2
        echo "Available Physical RAM: ${mem_avail} kB" 1>&2
    fi
else
    # Check using the percentage
    if [ x$(perl -e "print ${prct_mem_used} > ${MEM_PRCT_THRESH}") = "x1" ]; \
        then
        echo "** Warning! Maximum memory used threshold percent" \
            "($(echo ${MEM_PRCT_THRESH} | awk '{ printf("%d", $1 * 100) }')%)" \
            "has been exceeded..." 1>&2
        echo "Total Physical RAM: ${mem_total} kB" 1>&2
        echo "Available Physical RAM: ${mem_avail} kB" 1>&2
    fi
fi
echo -e "Memory Used Percent:\t$(echo ${prct_mem_used} | \
    awk '{ printf("%d", $1 * 100) }')%"
echo

# Check disk space (well, tmpfs root FS space)
disk_total=$(df -m / | grep tmpfs | awk '{print $2}')
disk_used=$(df -m / | grep tmpfs | awk '{print $3}')
disk_avail=$(df -m / | grep tmpfs | awk '{print $4}')
echo "Disk (/ -> root tmpfs) space check..."
echo -e "Total Disk Space:\t${disk_total} MB\nUsed Disk" \
    "Space:\t${disk_used} MB\nAvail. Disk Space:\t${disk_avail} MB"
prct_disk_used=$(echo "${disk_used} ${disk_total}" | \
    awk '{ printf("%.1g", $1 / $2) }')
echo -e "Disk Used Percent:\t$(echo ${prct_disk_used} | \
    awk '{ printf("%d", $1 * 100) }')%"
if [ x$(perl -e "print ${prct_disk_used} > ${DISK_PRCT_THRESH}") = "x1" ]; then
    echo "** Warning! Maximum disk space used threshold percent" \
        "($(echo ${DISK_PRCT_THRESH} | awk '{ printf("%d", $1 * 100) }')%)" \
        "has been exceeded..." 1>&2
    echo "Total Disk Space: ${disk_total} MB" 1>&2
    echo "Avail. Disk Space: ${disk_avail} MB" 1>&2
fi
echo

# Check if the USB drive is available/working via one of the FS
# labels (no indentation for if statement)
if ! findfs LABEL=${CHK_FS_LABEL} > /dev/null 2>&1; then
# Create a archive of the configuration files
arch_pkg_file="$(hostname)-esos_conf-$(date +%s).tgz"
arch_pkg_path="${TMP_PATH}/${arch_pkg_file}"
tar cpfz ${arch_pkg_path} --exclude='rc.d' --exclude='ssh' --exclude='shadow*' \
    /etc > /dev/null 2>&1
# Send an email with the archive file attachment (uuencode'd)
sendmail -t << _EOF_
To: ${EMAIL_TO}
From: ${EMAIL_FROM}
Subject: ESOS USB Flash Drive Failure - $(hostname) ($(date))
A possible USB flash drive failure has been detected on Enterprise Storage OS host "$(hostname)".

The findfs utility exited non-zero when attempting to resolve file system label "${CHK_FS_LABEL}". This may be due to a failed ESOS USB flash drive, or because the device was removed, or some other reason.

We're attaching a tar ball archive of the ESOS configuration files for this host just incase.

$(uuencode ${arch_pkg_path} ${arch_pkg_file})
_EOF_
rm -f ${arch_pkg_path}
fi

