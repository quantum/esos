#! /bin/sh

#shellcheck disable=SC3046,1091
source /etc/rc.d/common

WATCHER_LOG="/var/log/pwatcher.log"
WATCH_PROCESS_ARGS=""

stop() {
    /bin/echo "$(date "+%F %T %Z") Stopping ${WATCH_PROCESS}..." >> $WATCHER_LOG
    watch_pid=$(/usr/bin/pidof "${WATCH_PROCESS}")
    if [ -n "$watch_pid" ]; then
        /usr/bin/kill -TERM "${watch_pid}" || exit 1
        wait_for_stop "${WATCH_PROCESS}"
    fi
    exit
}

start() {
    /bin/echo "$(date "+%F %T %Z") Starting ${WATCH_PROCESS}..." >> $WATCHER_LOG
    # shellcheck disable=SC2086
    ${WATCH_PROCESS} ${WATCH_PROCESS_ARGS} > /dev/null 2>&1 &
}

status() {
    # Check for either a matching running daemon process or a subprocess
    # running the process
    /usr/bin/pidof "${WATCH_PROCESS}" > /dev/null 2>&1
    RC=$?
    /usr/bin/pidof -x "${WATCH_PROCESS}" > /dev/null 2>&1
    RC2=$?
    if [ $RC -ne 0 ] && [ $RC2 -ne 0 ]; then
        return 1
    else
        return 0
    fi
}

# stop the process that this watcher started on exit
trap "stop; exit" INT TERM QUIT

WATCH_PROCESS="${1}"
if [ -z "${WATCH_PROCESS}" ]; then
    echo "Usage: $0 process_name [process_args]"
    exit 1
fi

shift
#shellcheck disable=SC2124
WATCH_PROCESS_ARGS="${@}"

# Look at all pwatcher processes and see if we have one already
# running for the requested $WATCH_PROCESS
existing_pids=$(/usr/bin/pidof -x -o %PPID "$0")
# shellcheck disable=SC3011
if [ "$(wc -w <<< "$existing_pids")" -ge 1 ]; then
    for existing_pid in $existing_pids
    do
        if grep -q "$WATCH_PROCESS" "/proc/${existing_pid}/cmdline"; then
            echo "Already have a pwatcher process running for ${WATCH_PROCESS}, exiting"
            exit 0
        fi
    done
fi

while true
do
    if ! status; then
        /bin/echo "$(date "+%F %T %Z") ${WATCH_PROCESS} process not running, starting" >> $WATCHER_LOG
        start
        sleep 5
    else
        sleep 5
    fi
done
