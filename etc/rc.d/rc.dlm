#! /bin/sh

source /etc/rc.d/common

DLM_CONTROLD="/usr/sbin/dlm_controld"
DLM_CONTROLD_LOCK="/var/lock/dlm_controld"
DFLT_OPTS=""

check_args ${@}

SCRIPT="$(/usr/bin/basename ${0})"
if check_opts_set ${SCRIPT}; then
    USER_OPTS="$(get_rc_opts ${SCRIPT})"
    if [ ${?} -ne 0 ]; then
        /bin/echo ${USER_OPTS}
        exit 1
    fi
else
    USER_OPTS="${DFLT_OPTS}"
fi

start() {
    /bin/echo "Starting dlm_controld..."
    eval ${DLM_CONTROLD} ${USER_OPTS} || exit 1
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
