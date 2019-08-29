#! /bin/sh

# This script converts an ESOS boot drive (USB, physical disk, etc.) into a
# RAID1 (mirror) boot drive. The single argument given to this tool is the
# full path to a new block device that will be the RAID1 second member device.

SYNC_LOCK="/var/lock/conf_sync"
MD_META_VER="1.0"
MD_BOOT="esos_boot"
MD_ROOT="esos_root"
MD_CONF="esos_conf"
MD_LOGS="esos_logs"
TEMP_MNT="/tmp/raid1_mnt"
RSYNC_OPTS="-axHX --numeric-ids --info=progress2"

# Check the given block device
if [ ${#} -ne 1 ]; then
    echo "This script takes one argument:"
    echo "ADDTL_BLK_DEV"
    exit 1
fi
addtl_blk_dev="${1}"
if [ ! -b "${addtl_blk_dev}" ]; then
    echo "The given argument '${addtl_blk_dev}' does not appear" \
        "to be a block device!"
    exit 1
fi
wipefs_out="$(wipefs ${addtl_blk_dev})"
if [ -n "${wipefs_out}" ]; then
    echo "A disk label / signature exists on the block device:"
    wipefs ${addtl_blk_dev}
    echo "Please wipe the given block device before using this tool."
    exit 1
fi

# Make sure the ESOS file systems are not mounted
for i in /boot /mnt/root /mnt/conf /mnt/logs; do
    if mountpoint -q ${i}; then
        echo "ERROR: It appears that the '${i}' file system is mounted!"
        exit 1
    fi
done

# Look up the ESOS block device node
esos_root="$(findfs LABEL=esos_root)"
if [ ${?} -ne 0 ]; then
    echo "ERROR: We couldn't find the LABEL=esos_root file system!"
    exit 1
fi
esos_blk_dev="$(echo ${esos_root} | \
    sed -e '/\/dev\/sd/s/2//; /\/dev\/nvme/s/p2//')"

# Confirm the additonal block device is a suitable size
esos_dev_size="$(blockdev --getsize64 ${esos_blk_dev})"
addtl_dev_size="$(blockdev --getsize64 ${addtl_blk_dev})"
if [ ! ${addtl_dev_size} -ge ${esos_dev_size} ]; then
    echo "Current ESOS boot device size: ${esos_dev_size}"
    echo "Additional block device size: ${addtl_dev_size}"
    echo "The new/additional block device size must be greater" \
        "than or equal to the original ESOS boot drive!"
    exit 1
fi

if [ -n "${NO_PROMPT}" ] && [ ${NO_PROMPT} -eq 1 ]; then
    # Don't get confirmation, print a warning and continue
    echo "NO_PROMPT=1 is set, continuing..."
else
    # Get final approval from the user before continuing
    echo "Proceeding will destroy any data on the '${addtl_blk_dev}' device," \
        "and if the operation does not complete successfully, this may ruin" \
        "your running ESOS instance. Be sure you have a backup of the ESOS" \
        "configuration. Are you sure (yes/no) that you want to continue?" && \
        read confirm
    if ! [[ ${confirm} =~ [Yy]|[Yy][Ee][Ss] ]]; then
        exit 0
    fi
fi

# Prevent conf_sync.sh from running
exec 200> "${SYNC_LOCK}"
flock --timeout 300 -E 200 -x 200
RC=${?}
if [ ${RC} -ne 0 ]; then
    echo "ERROR: Could not acquire conf_sync lock (RC=${RC})!" 1>&2
    exit ${RC}
fi

# Copy the MBR from the current boot drive to the additional device
dd if=${esos_blk_dev} of=${addtl_blk_dev} bs=512 count=1 || exit 1

# Re-read the partition table and set variables
blockdev --rereadpt ${addtl_blk_dev}
if echo ${addtl_blk_dev} | grep -q "/dev/nvme"; then
    # For NVMe drives
    addtl_blk_boot="${addtl_blk_dev}p1"
    addtl_blk_root="${addtl_blk_dev}p2"
    addtl_blk_conf="${addtl_blk_dev}p3"
    addtl_blk_logs="${addtl_blk_dev}p4"
else
    # For SCSI drives
    addtl_blk_boot="${addtl_blk_dev}1"
    addtl_blk_root="${addtl_blk_dev}2"
    addtl_blk_conf="${addtl_blk_dev}3"
    addtl_blk_logs="${addtl_blk_dev}4"
fi

# Create the new MD RAID1 arrays
mdadm --create --verbose --run /dev/md/${MD_BOOT} --name=${MD_BOOT} \
    --level=1 --raid-devices=2 --bitmap=internal --metadata=${MD_META_VER} \
    missing ${addtl_blk_boot} || exit 1
mdadm --create --verbose --run /dev/md/${MD_ROOT} --name=${MD_ROOT} \
    --level=1 --raid-devices=2 --bitmap=internal --metadata=${MD_META_VER} \
    missing ${addtl_blk_root} || exit 1
mdadm --create --verbose --run /dev/md/${MD_CONF} --name=${MD_CONF} \
    --level=1 --raid-devices=2 --bitmap=internal --metadata=${MD_META_VER} \
    missing ${addtl_blk_conf} || exit 1
mdadm --create --verbose --run /dev/md/${MD_LOGS} --name=${MD_LOGS} \
    --level=1 --raid-devices=2 --bitmap=internal --metadata=${MD_META_VER} \
    missing ${addtl_blk_logs} || exit 1

# Create file systems on the new MD arrays (no FS labels for now)
mkfs.vfat -F 32 /dev/md/${MD_BOOT} || exit 1
mkfs.ext2 /dev/md/${MD_ROOT} || exit 1
mkfs.ext2 /dev/md/${MD_CONF} || exit 1
mkfs.ext2 /dev/md/${MD_LOGS} || exit 1

# Mount the new file systems
mkdir -p ${TEMP_MNT}/{boot,root,conf,logs} || exit 1
mount /dev/md/${MD_BOOT} ${TEMP_MNT}/boot || exit 1
mount /dev/md/${MD_ROOT} ${TEMP_MNT}/root || exit 1
mount /dev/md/${MD_CONF} ${TEMP_MNT}/conf || exit 1
mount /dev/md/${MD_LOGS} ${TEMP_MNT}/logs || exit 1

# Mount the original file systems
mount /boot || exit 1
mount /mnt/root || exit 1
mount /mnt/conf || exit 1
mount /mnt/logs || exit 1

# Copy contents of the orignals to the new file systems
rsync ${RSYNC_OPTS} /boot/ ${TEMP_MNT}/boot/ || exit 1
rsync ${RSYNC_OPTS} /mnt/root/ ${TEMP_MNT}/root/ || exit 1
rsync ${RSYNC_OPTS} /mnt/conf/ ${TEMP_MNT}/conf/ || exit 1
rsync ${RSYNC_OPTS} /mnt/logs/ ${TEMP_MNT}/logs/ || exit 1

# Unmount the original file systems
umount /boot || exit 1
umount /mnt/root || exit 1
umount /mnt/conf || exit 1
umount /mnt/logs || exit 1

# Unmount the new file systems
umount ${TEMP_MNT}/boot || exit 1
umount ${TEMP_MNT}/root || exit 1
umount ${TEMP_MNT}/conf || exit 1
umount ${TEMP_MNT}/logs || exit 1

# The variables make it clearer for the remaining steps
if echo ${esos_blk_dev} | grep -q "/dev/nvme"; then
    # For NVMe drives
    esos_blk_boot="${esos_blk_dev}p1"
    esos_blk_root="${esos_blk_dev}p2"
    esos_blk_conf="${esos_blk_dev}p3"
    esos_blk_logs="${esos_blk_dev}p4"
else
    # For SCSI drives
    esos_blk_boot="${esos_blk_dev}1"
    esos_blk_root="${esos_blk_dev}2"
    esos_blk_conf="${esos_blk_dev}3"
    esos_blk_logs="${esos_blk_dev}4"
fi

# Remove labels from the original file systems
fatlabel ${esos_blk_boot} OLD_BOOT || exit 1
e2label ${esos_blk_root} old_root || exit 1
e2label ${esos_blk_conf} old_conf || exit 1
e2label ${esos_blk_logs} old_logs || exit 1

# Wipe the original file system signatures (don't exit on error)
wipefs --all ${esos_blk_boot}
wipefs --all ${esos_blk_root}
wipefs --all ${esos_blk_conf}
wipefs --all ${esos_blk_logs}

# Add the original block devices into the new MD RAID1 arrays
mdadm /dev/md/${MD_BOOT} --add ${esos_blk_boot} || exit 1
mdadm /dev/md/${MD_ROOT} --add ${esos_blk_root} || exit 1
mdadm /dev/md/${MD_CONF} --add ${esos_blk_conf} || exit 1
mdadm /dev/md/${MD_LOGS} --add ${esos_blk_logs} || exit 1

# Create the file system labels on the new devices
fatlabel /dev/md/${MD_BOOT} ESOS_BOOT || exit 1
e2label /dev/md/${MD_ROOT} esos_root || exit 1
e2label /dev/md/${MD_CONF} esos_conf || exit 1
e2label /dev/md/${MD_LOGS} esos_logs || exit 1

# GRUB setup
mount /boot || exit 1
grub-bios-setup --directory=/boot/grub/i386-pc ${addtl_blk_dev} || exit 1
umount /boot || exit 1

# Done
echo "We made it this far, so RAID1 boot conversion is likely successful!"

