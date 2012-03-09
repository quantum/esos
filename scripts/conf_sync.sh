#! /bin/sh

# $Id$

# This script will synchronize configuration files between the ESOS USB device (esos_conf) and the root tmpfs filesystem.

CONF_MNT="/mnt/conf"
ETC_FILES="passwd group shadow network hostname hosts ssh_host_rsa_key.pub ssh_host_rsa_key ssh_host_ecdsa_key.pub ssh_host_ecdsa_key ssh_host_dsa_key.pub ssh_host_dsa_key random-seed resolv.conf revaliases ssmtp.conf scst.conf modprobe.conf pre-scst_xtra_conf post-scst_xtra_conf"

mount ${CONF_MNT} || exit 1
mkdir -p ${CONF_MNT}/etc

# Synchronize /etc
for i in ${ETC_FILES}; do
	local_file=/etc/${i}
	usb_file=${CONF_MNT}/etc/${i}
	# Case 1, neither local or USB exist
	if [ ! -f "${local_file}" ] && [ ! -f "${usb_file}" ]; then
		# Do nothing
		continue
	fi
	# Case 2, local file does not exist, but USB does
	if [ ! -f "${local_file}" ] && [ -f "${usb_file}" ]; then
		# Copy USB file to local filesystem
		cp -a ${usb_file} /etc/
		continue
	fi
	# Case 3, local file exists, but USB does not
	if [ -f "${local_file}" ] && [ ! -f "${usb_file}" ]; then
		# Copy local file to USB filesystem
		cp -a ${local_file} ${CONF_MNT}/etc/
		continue
	fi
	# Case 4, the file exists locally and on USB drive
	if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
		# Check and see which version is the newest
		if [ "${local_file}" -nt "${usb_file}" ]; then
			# Update the USB file with the local copy
			cp -a ${local_file} ${CONF_MNT}/etc/
			continue
		else
			# Update the local file with the USB copy
			cp -a ${usb_file} /etc/
			continue
		fi
	fi
done

umount ${CONF_MNT} || exit 1
