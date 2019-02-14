#! /bin/sh

# Helper script that gets run by init (/etc/inittab) when booting an ESOS ISO
# using installation mode (esos_iso=install). This script will mount the CD-ROM
# device and run the standard ESOS installer script; if an installation device
# kernel parameter is also provided (eg, install_dev=/dev/sda), this value is
# passed to the install.sh script as an argument.

# Always bail on any error
set -e

# From: https://wiki.gentoo.org/wiki/Custom_Initramfs
cmdline() {
    local value
    value=" $(cat /proc/cmdline) "
    value="${value##* $1=}"
    value="${value%% *}"
    [ "$value" != "" ] && echo "$value"
}

{
    # Mount the CD-ROM
    mount /dev/sr0 /mnt || bash

    # Grab the install device (if any)
    install_dev="$(cmdline install_dev)"

    # Change to the mounted CD-ROM directory and run the installer
    cd /mnt
    ./install.sh ${install_dev} || bash

    # Handle after-install customizations
    if [ -f "./extra_install.sh" ]; then
        echo " "
        echo "### Starting additional ESOS installation tasks..."
        sh ./extra_install.sh || bash
    fi

    # Done with the CD-ROM
    cd
    umount /mnt || bash

    # Pause until the user continues, then reboot
    echo " "
    while : ; do
        echo "### ESOS ISO installer complete; type 'yes' to reboot:" && \
            read confirm
        echo " "
        if [ "x${confirm}" = "xyes" ]; then
            break
        fi
    done
    reboot
} | tee /tmp/iso_installer_$(date +%s).log

