#! /bin/bash

PKG_DIR="${PWD}"
TEMP_DIR="${PKG_DIR}/temp"
MNT_DIR="${TEMP_DIR}/mnt"
LOCAL_RPM2CPIO="${PKG_DIR}/rpm2cpio.sh"
LINUX_REQD_TOOLS="tar cpio dd md5sum sha256sum grep blockdev lsscsi unzip findfs bunzip2 rpm2cpio"
MACOSX_REQD_TOOLS="tar cpio dd md5 shasum cat cut sed diff grep diskutil unzip bunzip2 fuse-ext2"
MD5_CHECKSUM="dist_md5sum.txt"
SHA256_CHECKSUM="dist_sha256sum.txt"
LINUX="LINUX"
MACOSX="MACOSX"

source ./install_common

echo "*** Enterprise Storage OS Install Script ***" && echo

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

# Set the rpm2cpio that we're using
if [ "${this_os}" = "${LINUX}" ]; then
    rpm2cpio=$(which rpm2cpio)
elif [ "${this_os}" = "${MACOSX}" ]; then
    rpm2cpio=${LOCAL_RPM2CPIO}
fi

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
image_file="$(ls esos-*.img.bz2)" || exit 1

# Check if we're doing an upgrade
if [ -f "/etc/esos-release" ]; then
    while true; do
        # Look up the ESOS block device node
        esos_root="$(findfs LABEL=esos_root)"
        if [ ${?} -ne 0 ]; then
            echo "ERROR: We couldn't find the LABEL=esos_root file system!"
            exit 1
        fi
        esos_blk_dev="$(echo ${esos_root} | sed -e 's/2//')"
        # Make sure the image disk label and the ESOS USB drive match
        image_mbr="$(mktemp -u -t mbr.XXXXXX)" || exit 1
        disk_parts="$(mktemp -u -t disk_parts.XXXXXX)" || exit 1
        image_parts="$(mktemp -u -t image_parts.XXXXXX)" || exit 1
        bunzip2 -d -c ${image_file} | dd of=${image_mbr} bs=512 count=1 > \
        /dev/null 2>&1 || exit 1
        fdisk -u -l ${esos_blk_dev} | egrep "^${esos_blk_dev}" | \
            sed -e 's/\*//' | awk '{ print $2 }' > ${disk_parts}
        fdisk -u -l ${image_mbr} | egrep "^${image_mbr}" | sed -e 's/\*//' | \
            awk '{ print $2 }' > ${image_parts}
        if ! diff ${disk_parts} ${image_parts} > /dev/null 2>&1; then
            echo "### The image file and current ESOS disk labels do not" \
                "match! An in-place upgrade is not supported, continuing..."
            echo
            rm -f ${image_mbr} ${disk_parts} ${image_parts}
            break
        fi
        # Get confirmation for an upgrade
        echo "### This installation script is running on a live ESOS host." \
            "Okay to perform an in-place upgrade (yes/no)? Log file system" \
            "data will persist and you will not be prompted to install" \
            "propietary CLI tools. If you decline, a full installation will" \
            "continue." && read confirm
            echo
        if [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
            echo "### Increasing the /tmp file system..."
            mount -o remount,size=5G /tmp || exit 1
            echo
            echo "### Mounting the ESOS USB flash drive file systems..."
            usb_esos_mnt="${TEMP_DIR}/old_esos"
            mkdir -p ${usb_esos_mnt} || exit 1
            mount ${esos_blk_dev}2 ${usb_esos_mnt} || exit 1
            mount ${esos_blk_dev}1 ${usb_esos_mnt}/boot || exit 1
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
            #losetup -d ${loop_dev} || exit 1
            rm -rf ${TEMP_DIR}
            echo
            echo "### The ESOS upgrade succeeded! Here are the details:"
            echo "Primary slot version: ${img_ver}"
            echo "Secondary slot version: ${usb_ver}"
            exit 0
        else
            break
        fi
    done
fi

# Print out a list of disk devices
echo "### Here is a list of disk devices on this machine:"
if [ "${this_os}" = "${LINUX}" ]; then
    lsscsi
elif [ "${this_os}" = "${MACOSX}" ]; then
    diskutil list
fi
echo

# Get desired USB drive device node and perform a few checks
echo "### Please type the full path of your USB drive device node" \
    "(eg, /dev/sdz):" && read dev_node
echo
if [ "${dev_node}" = "" ] || [ ! -e ${dev_node} ]; then
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
    echo "ERROR: Your USB flash drive isn't large enough;" \
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
echo "### Proceeding will completely wipe the '${real_dev_node}' device." \
    "Are you sure (yes/no)?" && read confirm
echo
if [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
    echo "### Writing ${image_file} to ${real_dev_node}; this may" \
        "take a while..."
    bunzip2 -d -c ${image_file} | dd of=${real_dev_node} bs=1${suffix} || exit 1
    if [ "${this_os}" = "${LINUX}" ]; then
        blockdev --rereadpt ${dev_node} || exit 1
    fi
    echo
    echo "### It appears the image was successfully written to disk" \
        "(no errors reported)!"
else
    exit 0
fi

# Continue on to installing proprietary CLI tools, or finished
echo && echo
read -p "*** If you would like to add any RAID controller management utilities, press ENTER to continue; otherwise press CTRL-C to quit, your ESOS USB drive install is complete. ***"

# Display proprietary tool information and download instructions
echo && echo
mkdir -p ${TEMP_DIR} || exit 1
for i in ${PROP_TOOLS}; do
    tool_desc=TOOL_DESC_${i}
    echo "### ${!tool_desc}"
    tool_file=TOOL_FILE_${i}
    echo "### Required file: ${!tool_file}"
    tool_url=TOOL_URL_${i}
    echo "### Download URL: ${!tool_url}"
    echo "### Place downloaded file in this directory: ${PKG_DIR}"
    echo
done

# Prompt user to continue
echo
read -p "*** Once the file(s) have been loaded and placed in the '${PKG_DIR}' directory, press ENTER to install the RAID controller CLI tools on your new ESOS USB drive. ***"

# Check downloaded packages
echo && echo
echo "### Checking downloaded packages..."
for i in ${PROP_TOOLS}; do
    tool_file=TOOL_FILE_${i}
    if [ -e "${PKG_DIR}/${!tool_file}" ]; then
        echo "${PKG_DIR}/${!tool_file}: Adding to the install list."
        install_list="${install_list} ${i}"
    else
        echo "${PKG_DIR}/${!tool_file}: File not found."
    fi
done

# Install the proprietary CLI tools to the ESOS USB drive (if any)
echo
if [ -z "${install_list}" ]; then
    echo "### Nothing to do."
    echo "### Your ESOS USB drive is complete, however, no RAID controller CLI tools were installed."
else
    echo "### Installing proprietary CLI tools..."
    mkdir -p ${MNT_DIR} || exit 1
    if [ "${this_os}" = "${LINUX}" ]; then
        esos_conf=$(findfs LABEL=esos_conf)
        if [ ${?} -ne 0 ]; then
            exit 1
        fi
        # Just in case this system has an auto-mounter
        if grep ${esos_conf} /proc/mounts > /dev/null 2>&1; then
            umount ${esos_conf} || exit 1
        fi
    fi
    # Mount it and inject all (if any) of the tools
    if [ "${this_os}" = "${LINUX}" ]; then
        mount ${esos_conf} ${MNT_DIR} || exit 1
    elif [ "${this_os}" = "${MACOSX}" ]; then
        fuse-ext2 -o force ${dev_node}s3 ${MNT_DIR} || exit 1
        sleep 5
    fi
    mkdir -p ${MNT_DIR}/opt/{sbin,lib} || exit 1
    cd ${TEMP_DIR}
    for i in ${install_list}; do
        if [ "${i}" = "MegaCLI" ]; then
            unzip -o ${PKG_DIR}/*_MegaCLI.zip && ${rpm2cpio} Linux/MegaCli-*.rpm | \
            cpio -idmv && cp opt/MegaRAID/MegaCli/MegaCli64 ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "StorCLI" ]; then
            unzip -o ${PKG_DIR}/*_StorCLI*.zip && unzip -o versionChangeSet/univ_viva_cli_rel/storcli_all_os.zip && ${rpm2cpio} storcli_all_os/Linux/storcli-*.rpm | \
            cpio -idmv && cp opt/MegaRAID/storcli/storcli64 ${MNT_DIR}/opt/sbin/ && \
            cp opt/MegaRAID/storcli/libstorelibir* ${MNT_DIR}/opt/lib/
        elif [ "${i}" = "arcconf" ]; then
            unzip -o ${PKG_DIR}/arcconf_*.zip && cp linux_x64/cmdline/arcconf ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "hpacucli" ]; then
            ${rpm2cpio} ${PKG_DIR}/hpacucli-*.x86_64.rpm | cpio -idmv && \
            cp opt/compaq/hpacucli/bld/.hpacucli ${MNT_DIR}/opt/sbin/hpacucli && \
            cp opt/compaq/hpacucli/bld/*.so ${MNT_DIR}/opt/lib/
        elif [ "${i}" = "hpssacli" ]; then
            ${rpm2cpio} ${PKG_DIR}/hpssacli-*.x86_64.rpm | cpio -idmv && \
            cp opt/hp/hpssacli/bld/hpssacli ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "linuxcli" ]; then
            unzip -o ${PKG_DIR}/linuxcli_*.zip && \
            cp linuxcli_*/x86_64/cli64 ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "3DM2_CLI" ]; then
            unzip -o ${PKG_DIR}/3DM2_CLI-*.zip && tar xvfz tdmCliLnx.tgz && \
            cp tw_cli.x86_64 ${MNT_DIR}/opt/sbin/
        fi
    done

    cd -
    umount ${MNT_DIR}
    echo
    echo "### ESOS USB drive installation complete!"
    echo "### You may now remove and use your ESOS USB drive."
fi

# Done
rm -rf ${TEMP_DIR}
exit 0

