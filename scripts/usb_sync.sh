#! /bin/sh

# $Id$

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem.

CONF_MNT="/mnt/conf"
SYNC_DIRS="/etc /var/lib /opt" # These are absolute paths (leading '/' required)
MKDIR="mkdir -m 0755 -p"
CP="cp -af"
CPIO="cpio -pdum --quiet"
LOOP_IFS=$(echo -en "\n\b")
ORIG_IFS=${IFS}
CTIME="stat -c %Z"
MTIME="stat -c %Y"

mount ${CONF_MNT} || exit 1

# Synchronize each directory
for i in ${SYNC_DIRS}; do
    ${MKDIR} ${CONF_MNT}${i}
    # Make sure all of the local directories exist on USB
    local_dir_base="${i}"
    IFS=${LOOP_IFS}
    for j in `test -d ${local_dir_base} && find ${local_dir_base} -type d \( ! -name rc.d \)`; do
        IFS=${ORIG_IFS}
        local_dir=${j}
        usb_dir=${CONF_MNT}${local_dir}
        # The directory doesn't exist on the USB drive
        if [ ! -d "${usb_dir}" ]; then
            # Create the directory
            echo ${local_dir} | ${CPIO} ${CONF_MNT}
            continue
        fi
        IFS=${LOOP_IFS}
    done
    # Make sure all of the local files exist on USB
    IFS=${LOOP_IFS}
    for j in `test -d ${local_dir_base} && find ${local_dir_base} -path /etc/rc.d -prune -o -type f -print`; do
        IFS=${ORIG_IFS}
        local_file=${j}
        usb_file=${CONF_MNT}${local_file}
        # The file doesn't exist on the USB drive
        if [ ! -f "${usb_file}" ]; then
            # Copy the local file to USB
            ${CP} "${local_file}" "${usb_file}" && touch "${local_file}" "${usb_file}"
            continue
        fi
        # The file exists in both locations
        if [ -f "${usb_file}" ] && [ -f "${local_file}" ]; then
            # Check and see which version is the newest (first check mtime, then ctime)
            if [ $(${MTIME} "${local_file}") -gt $(${MTIME} "${usb_file}") ]; then
                # Update the USB file with the local copy (mtime check)
                ${CP} "${local_file}" "${usb_file}" && touch "${local_file}" "${usb_file}"
            elif [ $(${MTIME} "${local_file}") -lt $(${MTIME} "${usb_file}") ]; then
                # Update the local file with the USB copy (mtime check)
                ${CP} "${usb_file}" "${local_file}" && touch "${usb_file}" "${local_file}"
            elif [ $(${CTIME} "${local_file}") -gt $(${CTIME} "${usb_file}") ]; then
                # Update the USB file with the local copy (ctime check)
                ${CP} "${local_file}" "${usb_file}" && touch "${local_file}" "${usb_file}"
            elif [ $(${CTIME} "${local_file}") -lt $(${CTIME} "${usb_file}") ]; then
                # Update the local file with the USB copy (ctime check)
                ${CP} "${usb_file}" "${local_file}" && touch "${usb_file}" "${local_file}"
            else
                # The files are the same; do nothing
                continue
            fi
        fi
        IFS=${LOOP_IFS}
    done
    # Make sure all of the USB directories exist locally
    usb_dir_base="${CONF_MNT}${i}"
    IFS=${LOOP_IFS}
    for j in `test -d ${usb_dir_base} && find ${usb_dir_base} -type d`; do
        IFS=${ORIG_IFS}
        usb_dir=${j}
        local_dir=`echo "${usb_dir}" | sed -e s@${CONF_MNT}@@`
        # The directory doesn't exist on the local file system
        if [ ! -d "${local_dir}" ]; then
            # Create the directory
            cd ${CONF_MNT} && echo ${usb_dir} | sed -e s@${CONF_MNT}/@@ | ${CPIO} / && cd - > /dev/null
            continue
        fi
        IFS=${LOOP_IFS}
    done
    # Make sure all of the USB files exist locally
    IFS=${LOOP_IFS}
    for j in `test -d ${usb_dir_base} && find ${usb_dir_base} -type f`; do
        IFS=${ORIG_IFS}
        usb_file=${j}
        local_file=`echo "${usb_file}" | sed -e s@${CONF_MNT}@@`
        # The file doesn't exist on the local file system
        if [ ! -f "${local_file}" ]; then
            # Copy the USB file to the local FS
            ${CP} "${usb_file}" "${local_file}" && touch "${usb_file}" "${local_file}"
            continue
        fi
        # The file exists in both locations
        if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
            # Check and see which version is the newest (first check mtime, then ctime)
            if [ $(${MTIME} "${usb_file}") -gt $(${MTIME} "${local_file}") ]; then
                # Update the local file with the USB copy (mtime)
                ${CP} "${usb_file}" "${local_file}" && touch "${usb_file}" "${local_file}"
            elif [ $(${MTIME} "${usb_file}") -lt $(${MTIME} "${local_file}") ]; then
                # Update the USB file with the local copy (mtime)
                ${CP} "${local_file}" "${usb_file}" && touch "${local_file}" "${usb_file}"
            elif [ $(${CTIME} "${usb_file}") -gt $(${CTIME} "${local_file}") ]; then
                # Update the local file with the USB copy (ctime)
                ${CP} "${usb_file}" "${local_file}" && touch "${usb_file}" "${local_file}"
            elif [ $(${CTIME} "${usb_file}") -lt $(${CTIME} "${local_file}") ]; then
                # Update the USB file with the local copy (ctime)
                ${CP} "${local_file}" "${usb_file}" && touch "${local_file}" "${usb_file}"
            else
                # The files are the same; do nothing
                continue
            fi
        fi
        IFS=${LOOP_IFS}
    done
done

# Make sure our sole symbolic link for the time zone is up to date
local_tz_link="/etc/localtime"
usb_tz_link="${CONF_MNT}${local_tz_link}"
if [ ! -L "${usb_tz_link}" ] && [ -L "${local_tz_link}" ]; then
    # The link doesn't exist on the USB drive
    ${CP} ${local_tz_link} ${usb_tz_link}
elif [ ! -L "${local_tz_link}" ] && [ -L "${usb_tz_link}" ]; then
    # The link doesn't exist on the local file system
    ${CP} ${usb_tz_link} ${local_tz_link}
elif [ -L "${usb_tz_link}" ] && [ -L "${local_tz_link}" ]; then
    # The link exists in both locations
    if [ $(${MTIME} "${local_tz_link}") -gt $(${MTIME} "${usb_tz_link}") ]; then
        # Update the USB link with the local copy
        ln -sf $(readlink -n "${local_tz_link}") "${usb_tz_link}"
        ln -sf $(readlink -n "${local_tz_link}") "${local_tz_link}"
    elif [ $(${MTIME} "${local_tz_link}") -lt $(${MTIME} "${usb_tz_link}") ]; then
        # Update the local link with the USB copy
        ln -sf $(readlink -n "${usb_tz_link}") "${local_tz_link}"
        ln -sf $(readlink -n "${usb_tz_link}") "${usb_tz_link}"
    else
        # The links are the same; do nothing
        :
    fi
fi

umount ${CONF_MNT} || exit 1
