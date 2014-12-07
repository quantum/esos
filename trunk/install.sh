#! /bin/bash
#
# $Id$

PKG_DIR="${PWD}"
TEMP_DIR="${PKG_DIR}/temp"
MNT_DIR="${TEMP_DIR}/mnt"
REQD_TOOLS="tar rpm2cpio cpio dd md5sum sha256sum grep blockdev unzip findfs"
MD5_CHECKSUM="dist_md5sum.txt"
SHA256_CHECKSUM="dist_sha256sum.txt"

source ./install_common

echo "*** Enterprise Storage OS Install Script ***" && echo

# Need root privileges for installer
if [ `whoami` != "root" ]; then
    echo "### This installer requires root privileges."
    exit 1
fi

# Make sure the required tools/utilities are available
echo "### Checking for required tools..."
for i in ${REQD_TOOLS}; do
    if ! which ${i} > /dev/null 2>&1; then
        echo "ERROR: The '${i}' utility is required to use this installation script."
        exit 1
    fi
done
echo

# Checksums
echo "### Verifying checksums..."
md5sum -w -c ${MD5_CHECKSUM} || exit 1
sha256sum -w -c ${SHA256_CHECKSUM} || exit 1
echo

# If the lsscsi program is available, list SCSI devices
if which lsscsi > /dev/null 2>&1; then
    echo "### Here is a list of SCSI devices on this machine:"
    lsscsi
    echo
fi

# Get desired USB drive device node and perform a few checks
echo "### Please type the full path of your USB drive device node (eg, /dev/sdz):" && read dev_node
echo
if [ "${dev_node}" = "" ] || [ ! -e ${dev_node} ]; then
    echo "ERROR: That device node doesn't seem to exist."
    exit 1
fi
if grep ${dev_node} /proc/mounts > /dev/null; then
    echo "ERROR: It looks like that device is mounted; unmount it and try again."
    exit 1
fi
dev_sectors=`blockdev --getsz ${dev_node}` || exit 1
dev_bytes=`expr ${dev_sectors} \\* 512`
if [ ${dev_bytes} -lt 4000000000 ]; then
    echo "ERROR: Your USB flash drive isn't large enough; it must be at least 4000 MiB."
    exit 1
fi

# Get a final confirmation before writing the image
echo "### Proceeding will completely wipe the '${dev_node}' device. Are you sure?" && read confirm
echo
if [ "${confirm}" = "yes" ] || [ "${confirm}" = "y" ]; then
    image_file=`ls esos-*.img`
    echo "### Writing ${image_file} to ${dev_node}; this may take a while..."
    dd if=./${image_file} of=${dev_node} bs=1M || exit 1
    blockdev --rereadpt ${dev_node} || exit 1
    echo
    echo "### It appears the image was successfully written to disk (no errors reported)!"
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
    esos_conf=$(findfs LABEL=esos_conf)
    if [ ${?} -ne 0 ]; then
        exit 1
    fi
    # Just in case this system has an auto-mounter
    if grep ${esos_conf} /proc/mounts > /dev/null 2>&1; then
        umount ${esos_conf} || exit 1
    fi
    # Mount it and inject all (if any) of the tools
    mount ${esos_conf} ${MNT_DIR} || exit 1
    mkdir -p ${MNT_DIR}/opt/{sbin,lib} || exit 1
    cd ${TEMP_DIR}
    for i in ${install_list}; do
        if [ "${i}" = "MegaCLI" ]; then
            unzip -o ${PKG_DIR}/*_MegaCLI.zip && rpm2cpio Linux/MegaCli-*.rpm | \
            cpio -idmv && cp opt/MegaRAID/MegaCli/MegaCli64 ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "StorCLI" ]; then
            unzip -o ${PKG_DIR}/*_StorCLI.zip && rpm2cpio storcli_all_os/Linux/storcli-*.rpm | \
            cpio -idmv && cp opt/MegaRAID/storcli/storcli64 ${MNT_DIR}/opt/sbin/ && \
            cp opt/MegaRAID/storcli/libstorelibir* ${MNT_DIR}/opt/lib/
        elif [ "${i}" = "arcconf" ]; then
            unzip -o ${PKG_DIR}/arcconf_*.zip && cp linux_x64/arcconf ${MNT_DIR}/opt/sbin/
        elif [ "${i}" = "hpacucli" ]; then
            rpm2cpio ${PKG_DIR}/hpacucli-*.x86_64.rpm | cpio -idmv && \
            cp opt/compaq/hpacucli/bld/.hpacucli ${MNT_DIR}/opt/sbin/hpacucli && \
            cp opt/compaq/hpacucli/bld/*.so ${MNT_DIR}/opt/lib/
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

