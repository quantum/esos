#! /bin/sh

# Helper script that gets run by init (/etc/inittab) when booting an ESOS ISO
# using installation mode (esos_iso=install). This script will mount the CD-ROM
# device and run the standard ESOS installer script; if an installation device
# kernel parameter is also provided (eg, install_dev=/dev/sda), this value is
# passed to the install.sh script as an argument.

# From: https://wiki.gentoo.org/wiki/Custom_Initramfs
cmdline() {
    local value
    value=" $(cat /proc/cmdline) "
    value="${value##* $1=}"
    value="${value%% *}"
    [ "$value" != "" ] && echo "$value"
}

# Mount the CD-ROM
mount /dev/sr0 /mnt

# Grab the install device (if any)
install_dev="$(cmdline install_dev)"

# Change to the mounted CD-ROM directory and run the installer
cd /mnt
./install.sh ${install_dev}

# Done with the CD-ROM
cd
umount /mnt

# Pause to print a message, then reboot
echo
read -p "*** Press the ENTER key to reboot... ***"
reboot

