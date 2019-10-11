#! /bin/sh

# Start cluster services (if not already started) in the proper sequence. If
# SBD is enabled, we attempt to start that first.

source /etc/rc.d/common

if check_enabled "rc.sbd"; then
    if ! /etc/rc.d/rc.sbd status; then
        /etc/rc.d/rc.sbd start
    fi
fi
if ! /etc/rc.d/rc.corosync status; then
    /etc/rc.d/rc.corosync start
fi
if ! /etc/rc.d/rc.pacemaker status; then
    /etc/rc.d/rc.pacemaker start
fi

