#! /bin/sh

# $Id$

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem.

CONF_MNT="/mnt/conf"
SYNC_DIRS="/etc /var/lib" # These are absolute paths (leading '/' required)
MKDIR="mkdir -m 0755 -p"
CP="cp -af"
CPIO="cpio -pdum --quiet"

mount ${CONF_MNT} || exit 1

# Synchronize each directory
for i in ${SYNC_DIRS}; do
    ${MKDIR} ${CONF_MNT}${i}
    # Make sure all of the local directories exist on USB
    local_dir_base="${i}"
    for j in `test -d ${local_dir_base} && find ${local_dir_base} -type d`; do
        local_dir=${j}
        usb_dir=${CONF_MNT}${local_dir}
        # The directory doesn't exist on the USB drive
        if [ ! -d "${usb_dir}" ]; then
            # Create the directory
            echo ${local_dir} | ${CPIO} ${CONF_MNT}
            continue
        fi
    done
    # Make sure all of the local files exist on USB
    for j in `test -d ${local_dir_base} && find ${local_dir_base} -type f`; do
        local_file=${j}
        usb_file=${CONF_MNT}${local_file}
        # The file doesn't exist on the USB drive
        if [ ! -f "${usb_file}" ]; then
            # Copy the local file to USB
            ${CP} ${local_file} ${usb_file}
            continue
        fi
        # The file exists in both locations
        if [ -f "${usb_file}" ] && [ -f "${local_file}" ]; then
            # Check and see which version is the newest
            if [ "${local_file}" -nt "${usb_file}" ]; then
                # Update the USB file with the local copy
                ${CP} ${local_file} ${usb_file}
            elif [ "${local_file}" -ot "${usb_file}" ]; then
                # Update the local file with the USB copy
                ${CP} ${usb_file} ${local_file}
            else
                # The files are the same; do nothing
                continue
            fi
        fi
    done
    # Make sure all of the USB directories exist locally
    usb_dir_base="${CONF_MNT}${i}"
    for j in `test -d ${usb_dir_base} && find ${usb_dir_base} -type d`; do
        usb_dir=${j}
        local_dir=`echo "${usb_dir}" | sed -e s@${CONF_MNT}@@`
        # The directory doesn't exist on the local file system
        if [ ! -d "${local_dir}" ]; then
            # Create the directory
            cd ${CONF_MNT} && echo ${usb_dir} | sed -e s@${CONF_MNT}/@@ | ${CPIO} / && cd -
            continue
        fi
    done
    # Make sure all of the USB files exist locally
    for j in `test -d ${usb_dir_base} && find ${usb_dir_base} -type f`; do
        usb_file=${j}
        local_file=`echo "${usb_file}" | sed -e s@${CONF_MNT}@@`
        # The file doesn't exist on the local file system
        if [ ! -f "${local_file}" ]; then
            # Copy the USB file to the local FS
            ${CP} ${usb_file} ${local_file}
            continue
        fi
        # The file exists in both locations
        if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
            # Check and see which version is the newest
            if [ "${usb_file}" -nt "${local_file}" ]; then
                # Update the local file with the USB copy
                ${CP} ${usb_file} ${local_file}
            elif [ "${usb_file}" -ot "${local_file}" ]; then
                # Update the USB file with the local copy
                ${CP} ${local_file} ${usb_file}
            else
                # The files are the same; do nothing
                continue
            fi
        fi
    done
done

umount ${CONF_MNT} || exit 1
