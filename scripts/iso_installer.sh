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

# Run the normal installation procedure
run_install() {
    if ! grep -q nfs_iso_device /proc/cmdline; then
        # Mount the CD-ROM (if it's not already)
        mount_cd_iso || return 1
    fi

    # Grab the install device / install transport type (if any)
    install_dev="$(cmdline install_dev)"
    install_tran="$(cmdline install_tran)"
    install_model="$(cmdline install_model)"
    wipe_devs="$(cmdline wipe_devs)"
    no_prompt="$(cmdline no_prompt)"
    image_server="$(cmdline imageserver)"

    # Make sure we don't have any SED-capable drives locked/enabled
    echo "### Checking all SED-capable drives..."
    sed_locked=0
    auth_failed=0
    # shellcheck disable=SC2010
    for i in $(ls /dev/ | grep -E '(sd[a-z]+|nvme[0-9]+)$'); do
        device="/dev/${i}"
        sed_cap="$(sedutil-cli --isValidSED "${device}" 2> /dev/null | \
            cut -d' ' -f2)"
        if [ "${sed_cap}" = "SED" ]; then
            if [[ ${device} == *"/dev/nvme"* ]]; then
                # NVMe Opal2 SED check
                locking="$(sedutil-cli --query "${device}" | \
                    awk '/Locking function \(0x0002\)/{getline; print}' | \
                    tr -d ' ' | tr ',' ' ')"
                eval "${locking}"
                # shellcheck disable=SC2154
                echo "NVMe SED '${device}' -> Locked: ${Locked}," \
                    "LockingEnabled: ${LockingEnabled}"
                if [ "${Locked}" = "Y" ] || [ "${LockingEnabled}" = "Y" ]; then
                    sed_locked=1
                fi
            else
                # SAS Enterprise SED check
                auth="$(sedutil-cli --listLockingRange 0 "" "${device}" 2>&1)"
                auth_status="${?}"
                echo "SAS SED '${device}' -> Default authentication status:" \
                    "${auth_status}"
                if [ "${auth_status}" -ne 0 ] && echo "${auth}" | \
                    grep -q -E '.*(Authenticate\s+failed)'; then
                    auth_failed=1
                fi
            fi
        fi
    done
    if [ "${sed_locked}" -eq 1 ] || [ "${auth_failed}" -eq 1 ]; then
        echo "ERROR: One or more SED-capable devices has SED locking" \
            "enabled and/or is currently locked, or default passphrase" \
            "authentication failed (see output above)! You must unlock" \
            "and disable SED on all devices before continuing!"
        return 1
    fi
    echo " "

    # Change to the mounted CD-ROM directory and run the installer
    cd /mnt/root || return 1
    WIPE_DEVS="${wipe_devs}" NO_PROMPT="${no_prompt}" ./install.sh \
        "${install_dev}" "${install_tran}" "${install_model}" || return 1

    if ! grep -q nfs_iso_device /proc/cmdline; then
        # Make sure the CD-ROM is still mounted (may get disconnected)
        mount_cd_iso || return 1
    fi

    # Handle after-install customizations
    if [ -f "./extra_install.sh" ]; then
        echo " "
        echo "### Starting additional ESOS installation tasks..."
        WIPE_DEVS="${wipe_devs}" NO_PROMPT="${no_prompt}" \
            IMAGE_SERVER="${image_server}" \
            sh ./extra_install.sh || return 1
    fi

    # Done with the CD-ROM
    cd || return 1

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
    reboot || return 1
}

# Bail to a shell if anything goes wrong
run_install 2>&1 | tee "/tmp/iso_installer_$(date +%s).log"
if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    bash
fi

