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
opensm/per-module-logging.conf opensm/torus-2QoS.conf corosync/corosync.conf \
mhvtl/mhvtl.conf mhvtl/device.conf rc.conf xtra_hosts"
VAR_DIRS="lib/scst lib/drbd lib/pacemaker lib/corosync lib/heartbeat"
MKDIR="mkdir -m 0755 -p"
CP="cp -af"
CPIO="cpio -pdum --quiet"

mount ${CONF_MNT} || exit 1

# The mhVTL library files are dynamic, so we need to discover them
if [ -d "/etc/mhvtl" ]; then
	local_mhvtl_dir="/etc/mhvtl"
fi
if [ -d "${CONF_MNT}/etc/mhvtl" ]; then
	usb_mhvtl_dir="${CONF_MNT}/etc/mhvtl"
fi
mhvtl_lib_files="$(find ${local_mhvtl_dir} ${usb_mhvtl_dir} -name library_contents.* -type f -exec basename {} \; | sort -u | sed -e 's/^/mhvtl\//' | tr '\n' ' ')"
ETC_FILES="${ETC_FILES} ${mhvtl_lib_files}"

# Synchronize /etc
${MKDIR} ${CONF_MNT}/etc
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
		${MKDIR} `dirname ${local_file}`
		${CP} ${usb_file} ${local_file}
		continue
	fi
	# Case 3, local file exists, but USB does not
	if [ -f "${local_file}" ] && [ ! -f "${usb_file}" ]; then
		# Copy local file to USB file system
		${MKDIR} `dirname ${usb_file}`
		${CP} ${local_file} ${usb_file}
		continue
	fi
	# Case 4, the file exists locally and on USB drive
	if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
		# Check and see which version is the newest
		if [ "${local_file}" -nt "${usb_file}" ]; then
			# Update the USB file with the local copy
			${CP} ${local_file} ${usb_file}
		elif [ "${local_file}" -ot "${usb_file}" ]; then
			# Update the local file with the USB copy
			${CP} ${usb_file} ${local_file}
		else
			# The files are the same; do nothing
			continue
		fi
	fi
done

# Synchronize /var
${MKDIR} ${CONF_MNT}/var
for i in ${VAR_DIRS}; do
	# Make sure all of the local directories exist on USB
	local_var_base="/var/${i}"
	for j in `test -d ${local_var_base} && find ${local_var_base} -type d`; do
		local_dir=${j}
		usb_dir=${CONF_MNT}${local_dir}
		# The directory doesn't exist on the USB drive
		if [ ! -d "${usb_dir}" ]; then
			# Create the directory
			echo ${local_dir} | ${CPIO} ${CONF_MNT}
			continue
		fi
	done
	# Make sure all of the local files exist on USB
	for j in `test -d ${local_var_base} && find ${local_var_base} -type f`; do
		local_file=${j}
		usb_file=${CONF_MNT}${local_file}
		# The file doesn't exist on the USB drive
		if [ ! -f "${usb_file}" ]; then
			# Copy the local file to USB
			${CP} ${local_file} ${usb_file}
			continue
		fi
		# The file exists in both locations
		if [ -f "${usb_file}" ] && [ -f "${local_file}" ]; then
			# Check and see which version is the newest
			if [ "${local_file}" -nt "${usb_file}" ]; then
				# Update the USB file with the local copy
				${CP} ${local_file} ${usb_file}
			elif [ "${local_file}" -ot "${usb_file}" ]; then
				# Update the local file with the USB copy
				${CP} ${usb_file} ${local_file}
			else
				# The files are the same; do nothing
				continue
			fi
		fi
	done
	# Make sure all of the USB directories exist locally
	usb_var_base="${CONF_MNT}/var/${i}"
	for j in `test -d ${usb_var_base} && find ${usb_var_base} -type d`; do
		usb_dir=${j}
		local_dir=`echo "${usb_dir}" | sed -e s@${CONF_MNT}@@`
		# The directory doesn't exist on the local file system
		if [ ! -d "${local_dir}" ]; then
			# Create the directory
			cd ${CONF_MNT} && echo ${usb_dir} | sed -e s@${CONF_MNT}/@@ | ${CPIO} / && cd -
			continue
		fi
	done
	# Make sure all of the USB files exist locally
	for j in `test -d ${usb_var_base} && find ${usb_var_base} -type f`; do
		usb_file=${j}
		local_file=`echo "${usb_file}" | sed -e s@${CONF_MNT}@@`
		# The file doesn't exist on the local file system
		if [ ! -f "${local_file}" ]; then
			# Copy the USB file to the local FS
			${CP} ${usb_file} ${local_file}
			continue
		fi
		# The file exists in both locations
		if [ -f "${local_file}" ] && [ -f "${usb_file}" ]; then
			# Check and see which version is the newest
			if [ "${usb_file}" -nt "${local_file}" ]; then
				# Update the local file with the USB copy
				${CP} ${usb_file} ${local_file}
			elif [ "${usb_file}" -ot "${local_file}" ]; then
				# Update the USB file with the local copy
				${CP} ${local_file} ${usb_file}
			else
				# The files are the same; do nothing
				continue
			fi
		fi
	done
done

umount ${CONF_MNT} || exit 1
