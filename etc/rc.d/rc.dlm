#! /bin/sh

# $Id$

source /etc/rc.d/common

DLM_CONTROLD="/usr/sbin/dlm_controld"
DLM_CONTROLD_LOCK="/var/lock/dlm_controld"

# Check arguments
if [ $# -ne 1 ] || [ "${1}" != "start" ] && [ "${1}" != "stop" ]; then
    /bin/echo "Usage: $0 {start | stop}"
    exit 1
fi

start() {
    /bin/echo "Setting up for DLM..."
    /bin/mount -t configfs none /sys/kernel/config
    /bin/echo "Starting dlm_controld..."
    ${DLM_CONTROLD} || exit 1
    /bin/touch ${DLM_CONTROLD_LOCK}
    /bin/sleep 1
}

stop() {
    /bin/echo "Stopping dlm_controld..."
    /bin/kill -TERM $(/bin/pidof ${DLM_CONTROLD}) || exit 1
    wait_for_stop ${DLM_CONTROLD} && /bin/rm -f ${DLM_CONTROLD_LOCK}
}

# Perform specified action
${1}
