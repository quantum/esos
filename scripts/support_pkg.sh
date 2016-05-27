#! /bin/sh

# This script is responsible for creating a support "bundle" (package/archive)
# from the ESOS server. System logs, kernel messages, configuration files, and
# other data is collected and packaged into an archive file. We attempt to
# avoid capturing sensitive data (eg, /etc/shadow) but care should be taken
# with the contents of the package file. The archive is created in the /tmp
# directory and the absolute path of the file name is returned.

BUNDLE_NAME="esos_support_pkg-$(date +%s)"
BUNDLE_DIR="/tmp/${BUNDLE_NAME}"
BUNDLE_FILE="${BUNDLE_DIR}.tgz"

# We use a temporary directory while collecting the data
mkdir -p ${BUNDLE_DIR} || exit 1

# Get everything we need
rsync -aq /etc ${BUNDLE_DIR} --exclude='rc.d' --exclude='ssh' \
    --exclude='shadow*' --exclude='ssmtp' || exit 1
rsync -aq /var/log ${BUNDLE_DIR} || exit 1
dmesg > ${BUNDLE_DIR}/dmesg.out || exit 1
lsmod > ${BUNDLE_DIR}/lsmod.out || exit 1
ps -aef > ${BUNDLE_DIR}/ps.out || exit 1
uptime > ${BUNDLE_DIR}/uptime.out || exit 1
last > ${BUNDLE_DIR}/last.out || exit 1

# Make the tarball
tar cpfz ${BUNDLE_FILE} --transform 's,^tmp/,,' \
    ${BUNDLE_DIR} 2> /dev/null || exit 1

# All done
rm -rf ${BUNDLE_DIR} || exit 1
echo "${BUNDLE_FILE}"

