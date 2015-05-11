#! /bin/sh

source /etc/rc.d/common

DLM_CONTROLD="/usr/sbin/dlm_controld"
DLM_CONTROLD_LOCK="/var/lock/dlm_controld"

check_args ${@}

start() {
    /bin/echo "Setting up for DLM..."
    /bin/mount -t configfs none /sys/kernel/config > /dev/null 2>&1
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

status() {
    /bin/pidof ${DLM_CONTROLD} > /dev/null 2>&1
    exit ${?}
}

# Perform specified action
${1}
