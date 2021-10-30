#! /bin/bash

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

# Helper for mounting the CD-ROM / ISO (used below)
mount_cd_iso() {
    cdrom_dev="$(findfs LABEL=ESOS-ISO)"
    # shellcheck disable=SC2181
    if [ ${?} -ne 0 ]; then
        echo "ERROR: Can't resolve 'LABEL=ESOS-ISO'!"
        return 1
    fi
    if ! grep -q "${cdrom_dev} /mnt/root" /proc/mounts; then
        echo "### Mounting the CD-ROM / ISO..."
        mount "${cdrom_dev}" /mnt/root || return 1
        echo " "
    fi
    return 0
}

{
    if ! grep -q nfs_iso_device /proc/cmdline; then
        # Mount the CD-ROM (if it's not already)
        mount_cd_iso || bash
    fi

    # Grab the install device / install transport type (if any)
    install_dev="$(cmdline install_dev)"
    install_tran="$(cmdline install_tran)"
    install_model="$(cmdline install_model)"
    wipe_devs="$(cmdline wipe_devs)"
    no_prompt="$(cmdline no_prompt)"
    image_server="$(cmdline imageserver)"

    # Change to the mounted CD-ROM directory and run the installer
    cd /mnt/root || bash
    WIPE_DEVS="${wipe_devs}" NO_PROMPT="${no_prompt}" ./install.sh \
        "${install_dev}" "${install_tran}" "${install_model}" || bash

    if ! grep -q nfs_iso_device /proc/cmdline; then
        # Make sure the CD-ROM is still mounted (may get disconnected)
        mount_cd_iso || bash
    fi

    # Handle after-install customizations
    if [ -f "./extra_install.sh" ]; then
        echo " "
        echo "### Starting additional ESOS installation tasks..."
        WIPE_DEVS="${wipe_devs}" NO_PROMPT="${no_prompt}" \
            IMAGE_SERVER="${image_server}" \
            sh ./extra_install.sh || bash
    fi

    # Done with the CD-ROM
    cd || exit 1

    if [ "x${no_prompt}" != "x1" ]; then
        # Pause until the user continues, then reboot
        echo " "
        while : ; do
            echo "### ESOS ISO installer complete; type 'yes' to reboot:" && \
                read -r confirm
            if [ "${confirm}" = "yes" ]; then
                break
            fi
        done
    fi
    echo " "
    echo "### Rebooting..."
    echo " "
    reboot
} | tee "/tmp/iso_installer_$(date +%s).log"

