#! /bin/sh

# $Id$

# This script will synchronize configuration files between the ESOS USB
# device (esos_conf) and the root tmpfs filesystem.

CONF_MNT="/mnt/conf"
ETC_FILES="passwd group shadow network.conf hosts resolv.conf \
ssh/ssh_host_rsa_key.pub ssh/ssh_host_rsa_key ssh/ssh_host_ecdsa_key.pub \
ssh/ssh_host_ecdsa_key ssh/ssh_host_dsa_key.pub ssh/ssh_host_dsa_key \
random-seed ssmtp/revaliases ssmtp/ssmtp.conf scst.conf modprobe.conf \
pre-scst_xtra_conf post-scst_xtra_conf drbd.conf lvm/lvm.conf mdadm.conf \
localtime ntp_server fstab opensm/opensm.conf opensm/ib-node-name-map \
opensm/partitions.conf opensm/qos-policy.conf opensm/prefix-routes.conf \
opensm/per-module-logging.conf opensm/torus-2QoS.conf"

mount ${CONF_MNT} || exit 1
mkdir -m 0755 -p ${CONF_MNT}/etc

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
		# Copy USB file to local file system
		mkdir -p `dirname ${local_file}`
		cp -a ${usb_file} ${local_file}
		continue
	fi
	# Case 3, local file exists, but USB does not
	if [ -f "${local_file}" ] && [ ! -f "${usb_file}" ]; then
		# Copy local file to USB file system
		mkdir -p `dirname ${usb_file}`
		cp -a ${local_file} ${usb_file}
		continue
	fi
	# Case 4, the file exists locally and on USB drive
	if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
		# Check and see which version is the newest
		if [ "${local_file}" -nt "${usb_file}" ]; then
			# Update the USB file with the local copy
			cp -af ${local_file} ${usb_file}
			continue
		else
			# Update the local file with the USB copy
			cp -af ${usb_file} ${local_file}
			continue
		fi
	fi
done

umount ${CONF_MNT} || exit 1
