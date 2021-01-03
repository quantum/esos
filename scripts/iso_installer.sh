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
    [ "${value}" != "" ] && echo "${value}"
}

{
    # Mount the CD-ROM
    mount $(findfs LABEL=ESOS-ISO) /mnt || bash

    # Grab the install device / install transport type (if any)
    install_dev="$(cmdline install_dev)"
    install_tran="$(cmdline install_tran)"
    install_model="$(cmdline install_model)"
    wipe_devs="$(cmdline wipe_devs)"
    no_prompt="$(cmdline no_prompt)"

    # Change to the mounted CD-ROM directory and run the installer
    cd /mnt
    WIPE_DEVS=${wipe_devs} NO_PROMPT=${no_prompt} ./install.sh \
        "${install_dev}" "${install_tran}" "${install_model}" || bash

    # Make sure the CD-ROM is still mounted
    cdrom_dev="$(findfs LABEL=ESOS-ISO)"
    if [ ${?} -ne 0 ]; then
        echo "ERROR: Can't resolve 'LABEL=ESOS-ISO'!"
        bash
    fi
    if ! grep -q "${cdrom_dev} /mnt" /proc/mounts; then
        echo "WARNING: It appears the CD-ROM is not mounted," \
            "attempting to remount..."
        mount ${cdrom_dev} /mnt || bash
    fi
    # Handle after-install customizations
    if [ -f "./extra_install.sh" ]; then
        echo " "
        echo "### Starting additional ESOS installation tasks..."
        WIPE_DEVS=${wipe_devs} NO_PROMPT=${no_prompt} \
            sh ./extra_install.sh || bash
    fi

    # Done with the CD-ROM
    cd
    umount /mnt || bash

    if [ "x${no_prompt}" != "x1" ]; then
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
    fi
    reboot
} | tee /tmp/iso_installer_$(date +%s).log

