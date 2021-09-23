#! /bin/sh

# Script that takes an uncompressed ESOS image (after 'image' target) and the
# path to an "isolinux" directory containing the ISO Linux components,
# including the "isolinux.cfg" config file, and any optional additional files
# that should be added to the root of the ISO image. The script then uses
# "genisoimage" to create the actual ESOS ISO. This ISO can then be booted to
# install ESOS on any machine that supports it.

usage() {
    echo "### Usage: $(basename ${0}) ISOLINUX_DIR [ADDITIONAL_FILES]"
    echo "  Creates a bootable ISO image from an uncompressed ESOS image."
    echo "  The path to the 'isolinux' directory must also be specified."
    echo "  Any additional files are placed in the root of the ISO image."
    exit 1
}

# This script requires root access (losetup, mount, etc.)
if [ "x$(whoami)" != "xroot" ]; then
    echo "### This tool requires root privileges."
    exit 1
fi

# Check arguments
if [ "x${1}" = "x-h" -o "x${1}" = "x--help" ]; then
    usage
fi
if [ ${#} -le 0 ]; then
    usage
fi

# Variable setup
ISOLINUX_DIR="${1}"
shift
ADDITIONAL_FILES="${@}"
SELF="$(readlink -f "${0}")"
ESOS_BUILD_DIR="$(readlink -f "$(dirname "${SELF}")")"
echo "### ESOS build directory: ${ESOS_BUILD_DIR}"
echo "### Checking for required ESOS components..."
ESOS_IMAGE="$(basename "$(echo ${ESOS_BUILD_DIR}/*-*.img)")"
BASE_FILES="${ESOS_IMAGE}.tar.bz2 ${ESOS_BUILD_DIR}/install.sh \
${ESOS_BUILD_DIR}/dist_*.txt ${ESOS_BUILD_DIR}/VERSION"
cd ${ESOS_BUILD_DIR} || exit 1
for i in ${BASE_FILES}; do
    if [ ! -r "${i}" ]; then
        echo "ERROR: Required file '${i}' not found or unreadable!"
        exit 1
    fi
done
ESOS_ISO="$(basename ${ESOS_IMAGE} .img).iso"

# Setup build tree
echo "### Setting up the ISO build tree..."
TEMP_DIR=$(mktemp -d -t esos2iso-XXXXXXXXX) || exit 1
rm -rf ${ESOS_ISO} || exit 1
cp -f -a ${ISOLINUX_DIR} ${TEMP_DIR}/isolinux || exit 1
mkdir -p ${TEMP_DIR}/isolinux || exit 1
cp -f -a ${ISOLINUX_DIR}/boot.cat ${TEMP_DIR}/isolinux/ || exit 1
cp -f -a ${ISOLINUX_DIR}/isolinux.bin ${TEMP_DIR}/isolinux/ || exit 1
cp -f -a ${ISOLINUX_DIR}/menu.c32 ${TEMP_DIR}/isolinux/ || exit 1
VER_STRING="$(cat ${ESOS_BUILD_DIR}/VERSION)"
sed "s/@@ver_string@@/${VER_STRING}/" ${ISOLINUX_DIR}/isolinux.cfg > \
    ${TEMP_DIR}/isolinux/isolinux.cfg || exit 1
echo

# Loop device setup
echo "### Creating loop device..."
LOOP_DEV="$(losetup -Pf --show ${ESOS_IMAGE})" || exit 1
if [ "x${LOOP_DEV}" = "x" ]; then
    echo "Unable to get a loop device!"
    exit 1
fi
echo

# Mount ESOS image and retrieve initramfs, kernel, and root archive
echo "### Fetching ESOS image components for ISO..."
MNT_DIR="/tmp/esos2iso_mnt"
mkdir -p ${MNT_DIR} || exit 1
mount ${LOOP_DEV}p1 ${MNT_DIR} || exit 1
cp ${MNT_DIR}/*initramfs* ${TEMP_DIR}/isolinux/initrd.img || exit 1
cp ${MNT_DIR}/*bzImage*prod ${TEMP_DIR}/isolinux/vmlinuz || exit 1
umount ${MNT_DIR} || exit 1
mount ${LOOP_DEV}p2 ${MNT_DIR} || exit 1
cp ${MNT_DIR}/*-root.sqsh ${TEMP_DIR}/ || exit 1
umount ${MNT_DIR} || exit 1
losetup -d ${LOOP_DEV} || exit 1
echo

# Get the list of files for the ISO
ISO_FILES="${BASE_FILES}"
if [ "x${ADDITIONAL_FILES}" != "x" ]; then
    # Put any optional files in root of ISO image
    ISO_FILES="${ISO_FILES} ${ADDITIONAL_FILES}"
fi
for i in ${ISO_FILES}; do
    ln -s $(readlink -f ${i}) ${TEMP_DIR}/$(basename ${i})
done

# Explanation of arguments used with 'genisoimage' below:
# -J                        # generate Windows compatible filenames
# -rock                     # generate Rock Ridge extension
# -rational-rock            # set file owner/mode
# -follow-links             # replace symlinks with actual contents
# -no-emul-boot             # do not do any disk emulation on bootable CD
# -boot-load-size n         # number of 512 sector to load in no-emulation mode
# -boot-info-table          # patch into boot image CD ROM layout (modified
#                           # boot image source file!)
# -eltorito-boot bootfile   # El Torito bootable CD image file
# -eltorito-catalog catfile # El Torito bootable CD catalog file
# -V string                 # 32-char volume ID
# -appid string             # 128 char Application ID
# -publisher string         # 128 char ID of the  publisher of the ISO
# -preparer string          # 128 char ID of preparer of the ISO
# -x lost+found             # do not include lost+found folder
# -o file                   # output file

# Create the ISO file
echo "### Creating the ISO file using 'genisoimage'..."
genisoimage -J \
    -rock \
    -rational-rock \
    -input-charset utf-8 \
    -follow-links \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -eltorito-boot isolinux/isolinux.bin \
    -eltorito-catalog isolinux/boot.cat \
    -boot-info-table \
    -V "ESOS-ISO" \
    -appid "ESOS" \
    -publisher "Quantum" \
    -preparer "Quantum" \
    -x lost+found \
    -o ${ESOS_ISO} \
    ${TEMP_DIR} || exit 1
echo

# Done
echo "### New bootable ESOS ISO has been created successfully: ${ESOS_ISO}"
rm -rf ${TEMP_DIR}
rmdir ${MNT_DIR}
exit 0

