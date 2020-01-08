#! /bin/bash

PKG_DIR="${PWD}"
TEMP_DIR="${PKG_DIR}/temp"
MNT_DIR="${TEMP_DIR}/mnt"
LINUX_REQD_TOOLS="dd md5sum sha256sum grep egrep blockdev lsblk findfs bunzip2"
MACOSX_REQD_TOOLS="dd md5 shasum cat cut sed diff grep egrep diskutil bunzip2"
MD5_CHECKSUM="dist_md5sum.txt"
SHA256_CHECKSUM="dist_sha256sum.txt"
LINUX="LINUX"
MACOSX="MACOSX"
SYNC_LOCK="/var/lock/conf_sync"

# Always bail on any error
set -e

# Optional installation block device path parameter
install_dev="${1}"
# Optional installation device transport type parameter
install_tran="${2}"

echo "*** $(cat VERSION) Install Script ***" && echo

# Need root privileges for installer
if [ "x$(whoami)" != "xroot" ]; then
    echo "### This installer requires root privileges."
    exit 1
fi

# What operating system is this
if [ "x$(uname -s)" = "xLinux" ]; then
    this_os="${LINUX}"
elif [ "x$(uname -s)" = "xDarwin" ]; then
    this_os="${MACOSX}"
else
    echo "ERROR: Only Linux and Mac OS X are supported with this script!"
    exit 1
fi

# Make sure the required tools/utilities are available
echo "### Checking for required tools..."
reqd_tools=${this_os}_REQD_TOOLS
for i in ${!reqd_tools}; do
    if ! which ${i} > /dev/null 2>&1; then
        echo "ERROR: The '${i}' utility is required to use this" \
            "installation script."
        exit 1
    fi
done
echo

# Checksums
echo "### Verifying checksums..."
if [ "${this_os}" = "${LINUX}" ]; then
    md5sum -w -c ${MD5_CHECKSUM} || exit 1
    sha256sum -w -c ${SHA256_CHECKSUM} || exit 1
elif [ "${this_os}" = "${MACOSX}" ]; then
    chksum_img_file="$(cat ${MD5_CHECKSUM} | cut -d' ' -f2 | sed -e 's/\*//')"
    if md5 -r ${chksum_img_file} | sed 's/ / */' | \
        diff ${MD5_CHECKSUM} - > /dev/null 2>&1; then
        echo "${chksum_img_file}: OK"
    else
        echo "${chksum_img_file}: The MD5 checksum doesn't match!"
        exit 1
    fi
    shasum -a 256 -c ${SHA256_CHECKSUM} || exit 1
fi
echo

# Locate the image file
image_file="$(ls *.img.bz2)" || exit 1

# Check if we're doing an upgrade
if test -f "/etc/esos-release" && test -z "${install_dev}" && \
    test -z "${install_tran}" && ! grep esos_iso /proc/cmdline > \
    /dev/null 2>&1; then
    # Prevent conf_sync.sh from running
    exec 200> "${SYNC_LOCK}"
    flock --timeout 300 -E 200 -x 200
    RC=${?}
    if [ ${RC} -ne 0 ]; then
        echo "ERROR: Could not acquire conf_sync lock (RC=${RC})!" 1>&2
        exit ${RC}
    fi
    while true; do
        # Look up the ESOS block device node
        esos_root="$(findfs LABEL=esos_root)"
        if [ ${?} -ne 0 ]; then
            echo "ERROR: We couldn't find the LABEL=esos_root file system!"
            exit 1
        fi
        # We only support upgrading if using a MD RAID boot drive
        if echo ${esos_root} | grep -q "/dev/md"; then
            using_md=1
        else
            using_md=0
        fi
        if [ ${using_md} -eq 1 ]; then
            # Set block device variables for below
            esos_blk_root="$(findfs LABEL=esos_root)"
            esos_blk_boot="$(findfs LABEL=ESOS_BOOT)"
            # Get confirmation for an upgrade (only option for MD boot)
            echo "### This installation script is running on a live ESOS" \
                "host. We've detected ESOS is using a MD RAID boot drive." \
                "Upgrading in-place is the only supported install option," \
                "please type 'yes' to continue the upgrade. If you decline," \
                "this installation script will exit." && read confirm
                echo
        else
            # Make sure the image disk label and the ESOS boot drive match
            esos_blk_dev="$(echo ${esos_root} | \
                sed -e '/\/dev\/sd/s/2//; /\/dev\/nvme/s/p2//')"
            image_mbr="$(mktemp -u -t mbr.XXXXXX)" || exit 1
            disk_parts="$(mktemp -u -t disk_parts.XXXXXX)" || exit 1
            image_parts="$(mktemp -u -t image_parts.XXXXXX)" || exit 1
            bunzip2 -d -c ${image_file} | dd of=${image_mbr} bs=512 count=1 > \
                /dev/null 2>&1 || exit 1
            fdisk -u -l ${esos_blk_dev} | egrep "^${esos_blk_dev}" | \
                sed -e 's/\*//' | awk '{ print $2 }' > ${disk_parts}
            fdisk -u -l ${image_mbr} | egrep "^${image_mbr}" | \
                sed -e 's/\*//' | awk '{ print $2 }' > ${image_parts}
            if ! diff ${disk_parts} ${image_parts} > /dev/null 2>&1; then
                echo "### The image file and current ESOS disk labels do not" \
                    "match! An in-place upgrade is not supported, continuing..."
                echo
                rm -f ${image_mbr} ${disk_parts} ${image_parts}
                break
            fi
            if echo ${esos_blk_dev} | grep -q "/dev/nvme"; then
                # For NVMe drives
                esos_blk_root="${esos_blk_dev}p2"
                esos_blk_boot="${esos_blk_dev}p1"
            else
                # For SCSI drives
                esos_blk_root="${esos_blk_dev}2"
                esos_blk_boot="${esos_blk_dev}1"
            fi
            # Get confirmation for an upgrade
            echo "### This installation script is running on a live ESOS" \
                "host. Okay to perform an in-place upgrade (yes/no)? Log" \
                "file system data will persist and you will not be" \
                "prompted to install propietary CLI tools. If you decline," \
                "a full installation will continue." && read confirm
                echo
        fi
        if [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
            echo "### Increasing the /tmp file system..."
            mount -o remount,size=6G /tmp || exit 1
            echo
            echo "### Mounting the ESOS boot drive file systems..."
            usb_esos_mnt="${TEMP_DIR}/old_esos"
            mkdir -p ${usb_esos_mnt} || exit 1
            mount ${esos_blk_root} ${usb_esos_mnt} || exit 1
            mount ${esos_blk_boot} ${usb_esos_mnt}/boot || exit 1
            echo
            echo "### Extracting the image file..."
            mkdir -p ${TEMP_DIR} || exit 1
            extracted_img="${TEMP_DIR}/$(basename ${image_file} .bz2)"
            bunzip2 -d -c ${image_file} > ${extracted_img} || exit 1
            echo
            echo "### Mounting the image file partitions..."
            img_esos_mnt="${TEMP_DIR}/new_esos"
            mkdir -p ${img_esos_mnt} || exit 1
            loop_dev="$(losetup -f)"
            losetup ${loop_dev} ${extracted_img} || exit 1
            kpartx -a -s ${loop_dev} || exit 1
            boot_dev="$(echo ${loop_dev} | sed 's/^\/dev/\/dev\/mapper/')p1"
            root_dev="$(echo ${loop_dev} | sed 's/^\/dev/\/dev\/mapper/')p2"
            mount ${root_dev} ${img_esos_mnt} || exit 1
            mount ${boot_dev} ${img_esos_mnt}/boot || exit 1
            echo
            echo "### Moving the current primary image to the secondary slot..."
            usb_ver="$(cat ${usb_esos_mnt}/boot/PRIMARY-version | cut -d= -f2)"
            mv -f ${usb_esos_mnt}/boot/PRIMARY-version \
                ${usb_esos_mnt}/boot/SECONDARY-version || exit 1
            mv -f ${usb_esos_mnt}/boot/PRIMARY-initramfs.cpio.gz \
                ${usb_esos_mnt}/boot/SECONDARY-initramfs.cpio.gz || exit 1
            mv -f ${usb_esos_mnt}/boot/PRIMARY-bzImage-esos.prod \
                ${usb_esos_mnt}/boot/SECONDARY-bzImage-esos.prod || exit 1
            mv -f ${usb_esos_mnt}/boot/PRIMARY-bzImage-esos.debug \
                ${usb_esos_mnt}/boot/SECONDARY-bzImage-esos.debug || exit 1
            mv -f ${usb_esos_mnt}/PRIMARY-root.cpio.bz2 \
                ${usb_esos_mnt}/SECONDARY-root.cpio.bz2 || exit 1
            echo
            echo "### Copying the new image to the primary slot..."
            img_ver="$(cat ${img_esos_mnt}/boot/PRIMARY-version | cut -d= -f2)"
            cp -fp ${img_esos_mnt}/boot/PRIMARY-version \
                ${usb_esos_mnt}/boot/PRIMARY-version || exit 1
            cp -fp ${img_esos_mnt}/boot/PRIMARY-initramfs.cpio.gz \
                ${usb_esos_mnt}/boot/PRIMARY-initramfs.cpio.gz || exit 1
            cp -fp ${img_esos_mnt}/boot/PRIMARY-bzImage-esos.prod \
                ${usb_esos_mnt}/boot/PRIMARY-bzImage-esos.prod || exit 1
            cp -fp ${img_esos_mnt}/boot/PRIMARY-bzImage-esos.debug \
                ${usb_esos_mnt}/boot/PRIMARY-bzImage-esos.debug || exit 1
            cp -fp ${img_esos_mnt}/PRIMARY-root.cpio.bz2 \
                ${usb_esos_mnt}/PRIMARY-root.cpio.bz2 || exit 1
            echo
            echo "### Cleaning up..."
            umount ${img_esos_mnt}/boot || exit 1
            umount ${img_esos_mnt} || exit 1
            umount ${usb_esos_mnt}/boot || exit 1
            umount ${usb_esos_mnt} || exit 1
            kpartx -d ${loop_dev} || exit 1
            rm -rf ${TEMP_DIR}
            echo
            echo "### The ESOS upgrade succeeded! Here are the details:"
            echo "Primary slot version: ${img_ver}"
            echo "Secondary slot version: ${usb_ver}"
            exit 0
        else
            if [ ${using_md} -eq 1 ]; then
                exit 1
            else
                break
            fi
        fi
    done
fi

if [ -z "${install_dev}" ] && [ -z "${install_tran}" ]; then
    # Print out a list of disk devices
    echo "### Here is a list of disk devices on this machine:"
    if [ "${this_os}" = "${LINUX}" ]; then
        lsblk --nodeps --paths --exclude 1,11,251,252 \
            --output NAME,VENDOR,MODEL,REV,SIZE,TRAN,SUBSYSTEMS
    elif [ "${this_os}" = "${MACOSX}" ]; then
        diskutil list
    fi
    echo
fi

# Get desired install target device node and perform a few checks
if [ -n "${install_dev}" ]; then
    dev_node="${install_dev}"
    echo "### Using block device '${dev_node}' given via argument..."
    echo
elif [ -n "${install_tran}" ]; then
    while : ; do
	echo "### Okay to wipe all block devices matching transport" \
            "type '${install_tran}' (yes/no)?" && read confirm
        echo
        if [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
            while read -r line; do
                blk_dev="$(echo ${line} | awk '{print $1}')"
                if [ -n "${blk_dev}" ]; then
                    device="/dev/${blk_dev}"
                    echo "### Attempting to wipe '${device}' via blkdiscard..."
                    blkdiscard ${device} || \
                        echo "WARNING: Failed to wipe the device!"
                fi
            done <<< "$(lsblk -o NAME,TYPE,TRAN | grep ${install_tran}\$)"
            echo
            break
        elif [[ ${confirm} =~ [Nn]|[Nn][Oo] ]]; then
            echo "WARNING: Not wiping block devices may result in first" \
                "boot issues!"
            echo
            break
        fi
    done
    tran_dev=$(lsblk -o NAME,TYPE,TRAN | grep "${install_tran}\$" | \
        head -1 | awk '{print $1}')
    if [ "x${tran_dev}" = "x" ]; then
        echo "ERROR: Unable to resolve any devices for transport" \
            "'${install_tran}'."
        exit 1
    fi
    dev_node="/dev/${tran_dev}"
    echo "### Using block device '${dev_node}' resolved via" \
        "transport '${install_tran}' argument..."
    echo
else
    while : ; do
        echo "### Please type the full path of your destination device node" \
            "(eg, /dev/sdz):" && read dev_node
        echo
        if [ -n "${dev_node}" ]; then
            break
        fi
    done
fi
if [ "x${dev_node}" = "x" ] || [ ! -e ${dev_node} ]; then
    echo "ERROR: That device node doesn't seem to exist."
    exit 1
fi
if mount | grep ${dev_node} > /dev/null; then
    echo "ERROR: It looks like that device is mounted; unmount it" \
        "and try again."
    exit 1
fi
if [ "${this_os}" = "${LINUX}" ]; then
    dev_sectors=$(blockdev --getsz ${dev_node}) || exit 1
    dev_sect_sz=$(blockdev --getss ${dev_node}) || exit 1
    dev_bytes=$(expr ${dev_sectors} \* ${dev_sect_sz})
elif [ "${this_os}" = "${MACOSX}" ]; then
    dev_bytes=$(diskutil info -plist ${dev_node} | \
        grep -A1 "<key>TotalSize</key>" | grep -o '<integer>'.*'</integer>' | \
        grep -o [^'<'integer'>'].*[^'<''/'integer'>']) || exit 1
fi
if [ ${dev_bytes} -lt 4000000000 ]; then
    echo "ERROR: Your target install drive isn't large enough;" \
        "it must be at least 4000 MiB."
    exit 1
fi

if [ "${this_os}" = "${LINUX}" ]; then
    suffix="M"
    real_dev_node=${dev_node}
elif [ "${this_os}" = "${MACOSX}" ]; then
    suffix="m"
    real_dev_node=${dev_node/\/dev\//\/dev\/r}
fi

# Get a final confirmation before writing the image
while : ; do
    echo "### Proceeding will completely wipe the '${real_dev_node}' device." \
        "Are you sure (yes/no)?" && read confirm
    echo
    if [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
        echo "### Writing '${image_file}' to '${real_dev_node}'; this may" \
            "take a while..."
        bunzip2 -d -c ${image_file} | dd of=${real_dev_node} bs=1${suffix} || \
            exit 1
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            exit 1
        fi
        if [ "${this_os}" = "${LINUX}" ]; then
            # Re-read the partition table (not fatal)
            blockdev --rereadpt ${dev_node}
        fi
        echo
        echo "### It appears the image was successfully written to disk" \
            "(no errors reported)!"
        break
    elif [[ ${confirm} =~ [Nn]|[Nn][Oo] ]]; then
        exit 1
    fi
done

# We're all done, users can install RAID tools in the ESOS instance
echo && echo
echo "*** RAID controller management utilities are now installed using" \
    "the 'raid_tools.py' script in a running ESOS instance. ***"
echo
echo "### ESOS boot device installation complete!"
echo "### You may now remove and use your ESOS bootable drive."

# Done
rm -rf ${TEMP_DIR}
exit 0

