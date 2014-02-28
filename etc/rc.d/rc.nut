#! /bin/sh

# $Id$

source /etc/rc.d/common

UPSD="/usr/sbin/upsd"
UPSMON="/usr/sbin/upsmon"
UPSDRVCTL="/usr/bin/upsdrvctl"
UPSD_LOCK="/var/lock/upsd"
UPSMON_LOCK="/var/lock/upsmon"
UPSDRVCTL_LOCK="/var/lock/upsdrvctl"
NUT_USER="nutmon"

# Check arguments
if [ $# -ne 1 ] || [ "${1}" != "start" ] && [ "${1}" != "stop" ]; then
    /bin/echo "Usage: $0 {start | stop}"
    exit 1
fi

start() {
    /bin/echo "Starting NUT UPS drivers..."
    ${UPSDRVCTL} -u ${NUT_USER} || exit 1
    /bin/touch ${UPSDRVCTL_LOCK}
    /bin/echo "Starting NUT UPS daemon..."
    ${UPSD} -u ${NUT_USER} || exit 1
    /bin/touch ${UPSD_LOCK}
    /bin/echo "Starting NUT UPS monitor..."
    ${UPSMON} -u ${NUT_USER} || exit 1
    /bin/touch ${UPSMON_LOCK}
}

stop() {
    /bin/echo "Stopping NUT UPS monitor..."
    ${UPSMON} -c stop && /bin/rm -f ${UPSMON_LOCK}
    /bin/echo "Stopping NUT UPS daemon..."
    ${UPSD} -c stop && /bin/rm -f ${UPSD_LOCK}
    /bin/echo "Stopping NUT UPS drivers..."
    ${UPSDRVCTL} stop && /bin/rm -f ${UPSDRVCTL_LOCK}
}

# Perform specified action
${1}
