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
MD_DATA="esos_data"
BOOT_LABEL="ESOS_BOOT"
ROOT_LABEL="esos_root"
CONF_LABEL="esos_conf"
LOGS_LABEL="esos_logs"
DATA_LABEL="esos_data"
ORIG_MNT="/tmp/orig_mnt"
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

if ! grep -q esos_iso /proc/cmdline; then
    # Make sure the ESOS file systems are not mounted
    for i in /boot /mnt/root /mnt/conf /mnt/logs /mnt/data; do
        if mountpoint -q ${i}; then
            echo "ERROR: It appears that the '${i}' file system is mounted!"
            exit 1
        fi
    done
fi

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

# Copy the partition table from the current boot drive to the additional device
sfdisk -d ${esos_blk_dev} > /tmp/sfdisk_pt || exit 1
sfdisk ${addtl_blk_dev} < /tmp/sfdisk_pt || exit 1

# Re-read the partition table and set variables
blockdev --rereadpt ${addtl_blk_dev}
if echo ${addtl_blk_dev} | grep -q "/dev/nvme"; then
    # For NVMe drives
    part_sep="p"
else
    # For SCSI drives
    part_sep=""
fi
if [ -b "${esos_blk_dev}${part_sep}5" ] && \
    [ -b "${esos_blk_dev}${part_sep}6" ]; then
    # Extended partitions
    addtl_blk_boot="${addtl_blk_dev}${part_sep}1"
    addtl_blk_root="${addtl_blk_dev}${part_sep}2"
    addtl_blk_conf="${addtl_blk_dev}${part_sep}3"
    addtl_blk_logs="${addtl_blk_dev}${part_sep}5"
    addtl_blk_data="${addtl_blk_dev}${part_sep}6"
else
    # Standard partitions
    addtl_blk_boot="${addtl_blk_dev}${part_sep}1"
    addtl_blk_root="${addtl_blk_dev}${part_sep}2"
    addtl_blk_conf="${addtl_blk_dev}${part_sep}3"
    addtl_blk_logs="${addtl_blk_dev}${part_sep}4"
    addtl_blk_data=""
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
if [ -n "${addtl_blk_data}" ]; then
    mdadm --create --verbose --run /dev/md/${MD_DATA} --name=${MD_DATA} \
        --level=1 --raid-devices=2 --bitmap=internal --metadata=${MD_META_VER} \
        missing ${addtl_blk_data} || exit 1
fi

# Create file systems on the new MD arrays (no FS labels for now)
mkfs.vfat -F 32 /dev/md/${MD_BOOT} || exit 1
mkfs.ext2 /dev/md/${MD_ROOT} || exit 1
mkfs.ext2 /dev/md/${MD_CONF} || exit 1
mkfs.ext2 /dev/md/${MD_LOGS} || exit 1
if [ -n "${addtl_blk_data}" ]; then
    mkfs.ext2 /dev/md/${MD_DATA} || exit 1
fi

# Mount the new file systems
mkdir -p ${TEMP_MNT}/{boot,root,conf,logs,data} || exit 1
mount /dev/md/${MD_BOOT} ${TEMP_MNT}/boot || exit 1
mount /dev/md/${MD_ROOT} ${TEMP_MNT}/root || exit 1
mount /dev/md/${MD_CONF} ${TEMP_MNT}/conf || exit 1
mount /dev/md/${MD_LOGS} ${TEMP_MNT}/logs || exit 1
if [ -n "${addtl_blk_data}" ]; then
    mount /dev/md/${MD_DATA} ${TEMP_MNT}/data || exit 1
fi

# Mount the original file systems
mkdir -p ${ORIG_MNT}/{boot,root,conf,logs,data} || exit 1
mount LABEL=${BOOT_LABEL} ${ORIG_MNT}/boot || exit 1
mount LABEL=${ROOT_LABEL} ${ORIG_MNT}/root || exit 1
mount LABEL=${CONF_LABEL} ${ORIG_MNT}/conf || exit 1
mount LABEL=${LOGS_LABEL} ${ORIG_MNT}/logs || exit 1
if [ -n "${addtl_blk_data}" ]; then
    mount LABEL=${DATA_LABEL} ${ORIG_MNT}/data || exit 1
fi

# Copy contents of the orignals to the new file systems
rsync ${RSYNC_OPTS} ${ORIG_MNT}/boot/ ${TEMP_MNT}/boot/ || exit 1
rsync ${RSYNC_OPTS} ${ORIG_MNT}/root/ ${TEMP_MNT}/root/ || exit 1
rsync ${RSYNC_OPTS} ${ORIG_MNT}/conf/ ${TEMP_MNT}/conf/ || exit 1
rsync ${RSYNC_OPTS} ${ORIG_MNT}/logs/ ${TEMP_MNT}/logs/ || exit 1
if [ -n "${addtl_blk_data}" ]; then
    rsync ${RSYNC_OPTS} ${ORIG_MNT}/data/ ${TEMP_MNT}/data/ || exit 1
fi

# Unmount the original file systems
umount ${ORIG_MNT}/boot || exit 1
umount ${ORIG_MNT}/root || exit 1
umount ${ORIG_MNT}/conf || exit 1
umount ${ORIG_MNT}/logs || exit 1
if [ -n "${addtl_blk_data}" ]; then
    umount ${ORIG_MNT}/data || exit 1
fi

# Unmount the new file systems
umount ${TEMP_MNT}/boot || exit 1
umount ${TEMP_MNT}/root || exit 1
umount ${TEMP_MNT}/conf || exit 1
umount ${TEMP_MNT}/logs || exit 1
if [ -n "${addtl_blk_data}" ]; then
    umount ${TEMP_MNT}/data || exit 1
fi

# The variables make it clearer for the remaining steps
if echo ${esos_blk_dev} | grep -q "/dev/nvme"; then
    # For NVMe drives
    part_sep="p"
else
    # For SCSI drives
    part_sep=""
fi
if [ -n "${addtl_blk_data}" ]; then
    # Extended partitions
    esos_blk_boot="${esos_blk_dev}${part_sep}1"
    esos_blk_root="${esos_blk_dev}${part_sep}2"
    esos_blk_conf="${esos_blk_dev}${part_sep}3"
    esos_blk_logs="${esos_blk_dev}${part_sep}5"
    esos_blk_data="${esos_blk_dev}${part_sep}6"
else
    # Standard partitions
    esos_blk_boot="${esos_blk_dev}${part_sep}1"
    esos_blk_root="${esos_blk_dev}${part_sep}2"
    esos_blk_conf="${esos_blk_dev}${part_sep}3"
    esos_blk_logs="${esos_blk_dev}${part_sep}4"
    esos_blk_data=""
fi

# Remove labels from the original file systems
fatlabel ${esos_blk_boot} OLD_BOOT || exit 1
e2label ${esos_blk_root} old_root || exit 1
e2label ${esos_blk_conf} old_conf || exit 1
e2label ${esos_blk_logs} old_logs || exit 1
if [ -n "${esos_blk_data}" ]; then
    e2label ${esos_blk_data} old_data || exit 1
fi

# Wipe the original file system signatures (don't exit on error)
wipefs --all ${esos_blk_boot}
wipefs --all ${esos_blk_root}
wipefs --all ${esos_blk_conf}
wipefs --all ${esos_blk_logs}
if [ -n "${esos_blk_data}" ]; then
    wipefs --all ${esos_blk_data}
fi

# Add the original block devices into the new MD RAID1 arrays
mdadm /dev/md/${MD_BOOT} --add ${esos_blk_boot} || exit 1
mdadm /dev/md/${MD_ROOT} --add ${esos_blk_root} || exit 1
mdadm /dev/md/${MD_CONF} --add ${esos_blk_conf} || exit 1
mdadm /dev/md/${MD_LOGS} --add ${esos_blk_logs} || exit 1
if [ -n "${esos_blk_data}" ]; then
    mdadm /dev/md/${MD_DATA} --add ${esos_blk_data} || exit 1
fi

# Create the file system labels on the new devices
fatlabel /dev/md/${MD_BOOT} ${BOOT_LABEL} || exit 1
e2label /dev/md/${MD_ROOT} ${ROOT_LABEL} || exit 1
e2label /dev/md/${MD_CONF} ${CONF_LABEL} || exit 1
e2label /dev/md/${MD_LOGS} ${LOGS_LABEL} || exit 1
if [ -n "${esos_blk_data}" ]; then
    e2label /dev/md/${MD_DATA} ${DATA_LABEL} || exit 1
fi

# GRUB setup
mount /boot || exit 1
grub-bios-setup --directory=/boot/grub/i386-pc ${addtl_blk_dev} || exit 1
umount /boot || exit 1

# Done
echo "We made it this far, so RAID1 boot conversion is likely successful!"

