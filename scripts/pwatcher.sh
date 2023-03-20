#! /bin/bash

# shellcheck disable=SC1091
source /etc/rc.d/common

WATCHER_LOG="/var/log/pwatcher.log"
WATCH_PROCESS_ARGS=""

log_it() {
    echo "$(date "+%F %T %Z") ${*}" >> "${WATCHER_LOG}"
}

start() {
    log_it "Starting '${WATCH_PROCESS}'..."
    log_it "STATUS_TEST='${STATUS_TEST}'"
    log_it "LOCK_FILE='${LOCK_FILE}'"
    log_it "WATCH_PROCESS='${WATCH_PROCESS}'"
    log_it "WATCH_PROCESS_ARGS='${WATCH_PROCESS_ARGS}'"
    # shellcheck disable=SC2086
    ${WATCH_PROCESS} ${WATCH_PROCESS_ARGS} > /dev/null 2>&1 &
    touch "${LOCK_FILE}"
}

stop() {
    log_it "Stopping '${WATCH_PROCESS}'..."
    watch_pid=$(pidof -x "${WATCH_PROCESS}")
    if [ -n "${watch_pid}" ]; then
        log_it "Sending TERM signal to PID '${watch_pid}'..."
        kill -TERM "${watch_pid}" || exit 1
        if ! wait_for_stop "${WATCH_PROCESS}" 30; then
            log_it "TERM didn't seem to stop the process;" \
                "sending ABRT signal to PID '${watch_pid}'..."
            kill -ABRT "${watch_pid}" || exit 1
            if ! wait_for_stop "${WATCH_PROCESS}" 20; then
                log_it "ABRT didn't seem to stop the process;" \
                    "sending KILL signal to PID '${watch_pid}'..."
                kill -KILL "${watch_pid}" || exit 1
                if ! wait_for_stop "${WATCH_PROCESS}" 10; then
                    log_it "KILL didn't seem to stop the process;" \
                        "we're giving up and exiting now..."
                    exit 1
                fi
            fi
        fi
    fi
}

status() {
    # Check for either a matching running daemon process or a subprocess
    # running the process
    pidof "${WATCH_PROCESS}" > /dev/null 2>&1
    rc=${?}
    pidof -x "${WATCH_PROCESS}" > /dev/null 2>&1
    rc2=${?}
    if [ ${rc} -ne 0 ] && [ ${rc2} -ne 0 ]; then
        # Only log the message if we have a lock file
        if [ -e "${LOCK_FILE}" ]; then
            log_it "No PID detected for the '${WATCH_PROCESS}' process!" \
                "It must have died unexpectedly..."
        fi
        return 1
    elif [ -n "${STATUS_TEST}" ]; then
        if ! eval "${STATUS_TEST}"; then
            log_it "The additional status test '${STATUS_TEST}' exited" \
                "non-zero for the '${WATCH_PROCESS}' process!" \
                "Attempting to stop it now..."
            stop
            return 1
        fi
    else
        return 0
    fi
}

# Stop the process that this watcher started on exit
trap "stop; exit" INT TERM QUIT

# Parse the arguments
STATUS_TEST="${1}"
shift
LOCK_FILE="${1}"
shift
WATCH_PROCESS="${1}"
shift
if [ -z "${LOCK_FILE}" ] || [ -z "${WATCH_PROCESS}" ]; then
    echo "Usage: ${0} STATUS_TEST LOCK_FILE PROCESS_NAME [PROCESS_ARGS]"
    exit 1
fi
# shellcheck disable=SC2124
WATCH_PROCESS_ARGS="${@}"

# Look at all pwatcher processes and see if we have one already
# running for the requested WATCH_PROCESS
existing_pids="$(pidof -x -o %PPID "${0}")"
if [ "$(wc -w <<< "${existing_pids}")" -ge 1 ]; then
    for existing_pid in ${existing_pids}; do
        if grep -q "${WATCH_PROCESS}" "/proc/${existing_pid}/cmdline"; then
            echo "An existing process watcher running for '${WATCH_PROCESS}'" \
                "was detected, exiting..."
            exit 0
        fi
    done
fi

# Start, and allow 10 seconds for initialization
start
sleep 10
# We loop here indefinitely, monitoring the watched process
while true; do
    if ! status; then
        log_it "The '${WATCH_PROCESS}' process is not running," \
            "attempting to start it..."
        start
        sleep 5
    else
        sleep 5
    fi
done

