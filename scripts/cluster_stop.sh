#! /bin/sh

# Stop cluster services (if not already stopped) in the proper sequence. If
# SBD is enabled, we attempt to stop that last.

source /etc/rc.d/common

if /etc/rc.d/rc.pacemaker status; then
    /etc/rc.d/rc.pacemaker stop
fi
if /etc/rc.d/rc.corosync status; then
    /etc/rc.d/rc.corosync stop
fi
if check_enabled "rc.sbd"; then
    if /etc/rc.d/rc.sbd status; then
        /etc/rc.d/rc.sbd stop
    fi
fi

