#! /bin/bash
#
# $Id$

TEMP_DIR=`mktemp -d /tmp/esos_install.XXXXX` || exit 1
MNT_DIR="${TEMP_DIR}/mnt"
REQD_TOOLS="tar rpm2cpio cpio dd md5sum sha256sum grep blockdev unzip"
PROP_TOOLS="MegaCLI asm_linux hpacucli linuxcli 3DM2_CLI"
MD5_CHECKSUM="dist_md5sum.txt"
SHA256_CHECKSUM="dist_sha256sum.txt"

TOOL_DESC_MegaCLI="LSI Logic MegaRAID Controllers"
TOOL_FILE_MegaCLI="8.02.16_MegaCLI.zip"
TOOL_URL_MegaCLI="http://www.lsi.com/downloads/Public/MegaRAID%20Common%20Files/8.02.16_MegaCLI.zip"
TOOL_INSTALL_CMD_MegaCLI="unzip -o *_MegaCLI.zip && unzip -o LINUX/MegaCliLin.zip && rpm2cpio MegaCli-*.rpm | cpio -idmv && rpm2cpio Lib_Utils-*.rpm | cpio -idmv && cp opt/MegaRAID/MegaCli/MegaCli64 ${MNT_DIR}/opt/sbin/ && chmod 755 ${MNT_DIR}/opt/sbin/MegaCli64 && cp opt/lsi/3rdpartylibs/libsysfs.so.2.0.2 ${MNT_DIR}/opt/lib/ && mkdir -p ${MNT_DIR}/opt/lsi/3rdpartylibs/x86_64 && ln -s /opt/lib/libsysfs.so.2.0.2 ${MNT_DIR}/opt/lsi/3rdpartylibs/x86_64/libsysfs.so.2.0.2"

TOOL_DESC_asm_linux="Adaptec AACRAID Controllers"
TOOL_FILE_asm_linux="asm_linux_x64_v7_30_18837.tgz"
TOOL_URL_asm_linux="http://www.adaptec.com/en-us/speed/raid/storage_manager/asm_linux_x64_v7_30_18837_tgz.htm"
TOOL_INSTALL_CMD_asm_linux="tar xvfz asm_linux_*.tgz && rpm2cpio manager/StorMan-*.x86_64.rpm | cpio -idmv && cp cmdline/arcconf ${MNT_DIR}/opt/sbin/ && chmod 755 ${MNT_DIR}/opt/sbin/arcconf && cp usr/StorMan/libstdc++.so.5 ${MNT_DIR}/opt/lib/"

TOOL_DESC_hpacucli="HP Smart Array Controllers"
TOOL_FILE_hpacucli="hpacucli-9.30-15.0.x86_64.rpm"
TOOL_URL_hpacucli="http://h20000.www2.hp.com/bizsupport/TechSupport/SoftwareDescription.jsp?lang=en&cc=us&prodTypeId=18964&prodSeriesId=468780&prodNameId=468781&swEnvOID=4103&swLang=8&mode=2&taskId=135&swItem=MTX-d21212ba8dbe4147b43fd70a55"
TOOL_INSTALL_CMD_hpacucli="rpm2cpio hpacucli-*.x86_64.rpm | cpio -idmv && cp opt/compaq/hpacucli/bld/.hpacucli ${MNT_DIR}/opt/sbin/hpacucli && chmod 755 ${MNT_DIR}/opt/sbin/hpacucli"

TOOL_DESC_linuxcli="Areca RAID Controllers"
TOOL_FILE_linuxcli="linuxcli_V1.10.0_120815.zip"
TOOL_URL_linuxcli="http://www.areca.us/support/s_linux/cli/linuxcli_V1.10.0_120815.zip"
TOOL_INSTALL_CMD_linuxcli="unzip linuxcli_*.zip && cp linuxcli_*/x86_64/cli64 ${MNT_DIR}/opt/sbin/ && chmod 755 ${MNT_DIR}/opt/sbin/cli64"

TOOL_DESC_3DM2_CLI="3ware SATA/SAS RAID Controllers"
TOOL_FILE_3DM2_CLI="3DM2_CLI-Linux_10.2.1_9.5.4.zip"
TOOL_URL_3DM2_CLI="http://www.lsi.com/downloads/Public/SATA/SATA%20Common%20Files/3DM2_CLI-Linux_10.2.1_9.5.4.zip"
TOOL_INSTALL_CMD_3DM2_CLI="unzip 3DM2_CLI-*.zip && tar xvfz tdmCliLnx.tgz && cp tw_cli.x86_64 ${MNT_DIR}/opt/sbin/ && chmod 755 ${MNT_DIR}/opt/sbin/tw_cli.x86_64"

echo "*** Enterprise Storage OS Install Script ***" && echo

# Need root privileges for installer
if [ `whoami` != "root" ]; then
    echo "### This installer requires root privileges."
    rm -rf ${TEMP_DIR}
    exit 1
fi

# Make sure the required tools/utilities are available
echo "### Checking for required tools..."
for i in ${REQD_TOOLS}; do
    if ! which ${i} > /dev/null 2>&1; then
        echo "The '${i}' utility is required to use this installation script."
        rm -rf ${TEMP_DIR}
        exit 1
    fi
done
echo

# Checksums
echo "### Verifying checksums..."
md5sum -w -c ${MD5_CHECKSUM}
sha256sum -w -c ${SHA256_CHECKSUM}
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
    echo "### That device node doesn't seem to exist."
    rm -rf ${TEMP_DIR}
    exit 1
fi
if grep ${dev_node} /proc/mounts > /dev/null; then
    echo "### It looks like that device is mounted..."
    rm -rf ${TEMP_DIR}
    exit 1
fi
dev_sectors=`blockdev --getsz ${dev_node}` || exit 1
dev_bytes=`expr ${dev_sectors} \\* 512`
if [ ${dev_bytes} -lt 4000000000 ]; then
    echo "### Your USB flash drive isn't large enough; it must be at least 4000 MiB."
    rm -rf ${TEMP_DIR}
    exit 1
fi

# Get a final confirmation before writing the image
echo "### Proceeding will completely wipe the '${dev_node}' device. Are you sure?" && read confirm
echo
if [ "${confirm}" = "yes" ] || [ "${confirm}" = "y" ]; then
    image_file=`ls esos-*.img`
    echo "### Writing ${image_file} to ${dev_node}; this may take a while..."
    dd if=./${image_file} of=${dev_node} bs=1M || exit 1
    echo
    echo "### It appears the image was successfully written to disk (no errors reported)!"
else
    rm -rf ${TEMP_DIR}
    exit 0
fi

# Continue on to installing proprietary CLI tools, or finished
echo && echo
read -p "*** If you would like to add any RAID controller management utilities, press ENTER to continue; otherwise press CTRL-C to quit, your ESOS USB drive install is complete. ***"

# Display proprietary tool information and download instructions
echo && echo
for i in ${PROP_TOOLS}; do
    tool_desc=TOOL_DESC_${i}
    echo "### ${!tool_desc}"
    tool_file=TOOL_FILE_${i}
    echo "### Required file: ${!tool_file}"
    tool_url=TOOL_URL_${i}
    echo "### Download URL: ${!tool_url}"
    echo "### Place downloaded file in this directory: ${TEMP_DIR}"
    echo
done

# Prompt user to continue
echo
read -p "*** Once the file(s) have been loaded and placed in the '${TEMP_DIR}' directory, press ENTER to install the RAID controller CLI tools on your new ESOS USB drive. ***"

# Check downloaded packages
echo && echo
echo "### Checking downloaded packages..."
for i in ${PROP_TOOLS}; do
    tool_file=TOOL_FILE_${i}
    if [ -e "${TEMP_DIR}/${!tool_file}" ]; then
        install_list="${install_list} ${i}"
    else
        echo "${TEMP_DIR}/${!tool_file}: File not found."
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
    blockdev --rereadpt ${dev_node} || exit 1
    mount -L esos_root ${MNT_DIR} || exit 1
    cd ${TEMP_DIR}
    for i in ${install_list}; do
        tool_install_cmd=TOOL_INSTALL_CMD_${i}
        eval ${!tool_install_cmd}
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
