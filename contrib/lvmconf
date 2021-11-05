#!/bin/bash
#
# Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
#
# This file is part of the lvm2 package.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#
# Edit an lvm.conf file to adjust various properties
#

# cluster with clvmd and/or locking lib?
HANDLE_CLUSTER=0

# cluster without clvmd?
HANDLE_HALVM=0

# also enable services appropriately (lvmetad, clvmd)?
HANDLE_SERVICES=0

# also enable cmirrord service in addition?
HANDLE_MIRROR_SERVICE=0

# also start/start services in addition to enabling/disabling them?
START_STOP_SERVICES=0

function usage
{
    echo "Usage: $0 <command>"
    echo ""
    echo "Commands:"
    echo "Enable clvm:  --enable-cluster [--lockinglibdir <dir>] [--lockinglib <lib>]"
    echo "Disable clvm: --disable-cluster"
    echo "Enable halvm: --enable-halvm"
    echo "Disable halvm: --disable-halvm"
    echo "Set locking library: --lockinglibdir <dir> [--lockinglib <lib>]"
    echo ""
    echo "Global options:"
    echo "Config file location: --file <configfile>"
    echo "Set services: --services [--mirrorservice] [--startstopservices]"
    echo ""
    echo "Use the separate command 'lvmconfig' to display configuration information"
}

function set_default_use_lvmetad_var
{
	eval "$(lvm dumpconfig --type default global/use_lvmetad 2>/dev/null)"
	if [ "$?" != 0 ]; then
		USE_LVMETAD=0
	else
                USE_LVMETAD=$use_lvmetad
	fi
}

function parse_args
{
    while [ -n "$1" ]; do
        case "$1" in
            --enable-cluster)
                LOCKING_TYPE=3
                USE_LVMETAD=0
                HANDLE_CLUSTER=1
                shift
                ;;
            --disable-cluster)
                LOCKING_TYPE=1
                set_default_use_lvmetad_var
                HANDLE_CLUSTER=1
                shift
                ;;
            --enable-halvm)
                LOCKING_TYPE=1
		USE_LVMETAD=0
                HANDLE_HALVM=1
		shift
                ;;
            --disable-halvm)
                LOCKING_TYPE=1
                set_default_use_lvmetad_var
                HANDLE_HALVM=1
		shift
                ;;
            --lockinglibdir)
                if [ -n "$2" ]; then
                    LOCKINGLIBDIR=$2
                    shift 2
                else
                    usage
                    exit 1
                fi
                HANDLE_CLUSTER=1
                ;;
            --lockinglib)
                if [ -n "$2" ]; then
                    LOCKINGLIB=$2
                    shift 2
                else
                    usage
                    exit 1
                fi
                HANDLE_CLUSTER=1
                ;;
            --file)
                if [ -n "$2" ]; then
                    CONFIGFILE=$2
                    shift 2
                else
                    usage
                    exit 1
                fi
                ;;
            --services)
                HANDLE_SERVICES=1
                shift
                ;;
            --mirrorservice)
                HANDLE_MIRROR_SERVICE=1
                shift
                ;;
            --startstopservices)
                START_STOP_SERVICES=1
                shift
                ;;
            *)
                usage
                exit 1
        esac
    done

    if [ -n "$LOCKINGLIBDIR" ] || [ -n "$LOCKINGLIB" ]; then
        LOCKING_TYPE=2
        USE_LVMETAD=0
    fi
}

function validate_args
{
    [ -z "$CONFIGFILE" ] && CONFIGFILE="/etc/lvm/lvm.conf"

    if [ ! -f "$CONFIGFILE" ]
            then
            echo "$CONFIGFILE does not exist"
            exit 10
    fi

    if [ "$HANDLE_CLUSTER" = 1 ] && [ "$HANDLE_HALVM" = 1 ]; then
        echo "Either HA LVM or cluster method may be used at one time"
	    exit 18
    fi

    if [ "$HANDLE_SERVICES" = 0 ]; then
        if [ "$HANDLE_MIRROR_SERVICE" = 1 ]; then
            echo "--mirrorservice may be used only with --services"
            exit 19
        fi
        if [ "$START_STOP_SERVICES" = 1 ]; then
            echo "--startstopservices may be used only with --services"
            exit 19
        fi
    fi

    if [ -z "$LOCKING_TYPE" ] && [ -z "$LOCKINGLIBDIR" ]; then
        usage
        exit 1
    fi

    if [ -n "$LOCKINGLIBDIR" ]; then

        if [ "${LOCKINGLIBDIR:0:1}" != "/" ]
            then
            echo "Prefix must be an absolute path name (starting with a /)"
            exit 12
        fi

        if [ -n "$LOCKINGLIB" ] && [ ! -f "$LOCKINGLIBDIR/$LOCKINGLIB" ]
            then
            echo "$LOCKINGLIBDIR/$LOCKINGLIB does not exist, did you do a \"make install\" ?"
            exit 11
        fi

    fi

    if [ "$LOCKING_TYPE" = 1 ] ; then
	if [ -n "$LOCKINGLIBDIR" ] || [ -n "$LOCKINGLIB" ]; then
		echo "Superfluous locking lib parameter, ignoring"
	fi
    fi
}

umask 0077

parse_args "$@"

validate_args


SCRIPTFILE=/etc/lvm/.lvmconf-script.tmp
TMPFILE=/etc/lvm/.lvmconf-tmp.tmp


# Flags so we know which parts of the file we can replace and which need
# adding. These are return codes from grep, so zero means it IS present!
have_type=1
have_dir=1
have_library=1
have_use_lvmetad=1
have_global=1

grep -q '^[[:blank:]]*locking_type[[:blank:]]*=' "$CONFIGFILE"
have_type=$?

grep -q '^[[:blank:]]*library_dir[[:blank:]]*=' "$CONFIGFILE"
have_dir=$?

grep -q '^[[:blank:]]*locking_library[[:blank:]]*=' "$CONFIGFILE"
have_library=$?

grep -q '^[[:blank:]]*use_lvmetad[[:blank:]]*=' "$CONFIGFILE"
have_use_lvmetad=$?

# Those options are in section "global {" so we must have one if any are present.
if [ "$have_type" = 0 ] || [ "$have_dir" = 0 ] || [ "$have_library" = 0 ] || [ "$have_use_lvmetad" = 0 ]
then

    # See if we can find it...
    grep -q '^[[:blank:]]*global[[:blank:]]*{' $CONFIGFILE
    have_global=$?

    if [ "$have_global" = 1 ] 
	then
	echo "global keys but no 'global {' found, can't edit file"
	exit 13
    fi
fi

if [ "$LOCKING_TYPE" = 2 ] && [ -z "$LOCKINGLIBDIR" ] && [ "$have_dir" = 1 ]; then
	echo "no library_dir specified in $CONFIGFILE"
	exit 16
fi

# So if we don't have "global {" we need to create one and 
# populate it

if [ "$have_global" = 1 ]
then
    if [ -z "$LOCKING_TYPE" ]; then
	LOCKING_TYPE=1
    fi
    if [ "$LOCKING_TYPE" = 3 ] || [ "$LOCKING_TYPE" = 2 ]; then
        cat "$CONFIGFILE" - <<EOF > "$TMPFILE"
global {
    # Enable locking for cluster LVM
    locking_type = $LOCKING_TYPE
    library_dir = "$LOCKINGLIBDIR"
    # Disable lvmetad in cluster
    use_lvmetad = 0
EOF
        if [ $? != 0 ]
        then
    	    echo "failed to create temporary config file, $CONFIGFILE not updated"
	    exit 14
        fi
	if [ -n "$LOCKINGLIB" ]; then
	    cat - <<EOF >> "$TMPFILE"
    locking_library = "$LOCKINGLIB"
EOF
            if [ $? != 0 ]
            then
	        echo "failed to create temporary config file, $CONFIGFILE not updated"
	        exit 16
            fi
	fi
	cat - <<EOF >> "$TMPFILE"
}
EOF
    fi # if we aren't setting cluster locking, we don't need to create a global section

    if [ $? != 0 ]
    then
	echo "failed to create temporary config file, $CONFIGFILE not updated"
	exit 17
    fi
else
    #
    # We have a "global {" section, so add or replace the
    # locking entries as appropriate
    #

    if [ -n "$LOCKING_TYPE" ]; then
	if [ "$have_type" = 0 ] 
	then
	    SEDCMD=" s/^[[:blank:]]*locking_type[[:blank:]]*=.*/\ \ \ \ locking_type = $LOCKING_TYPE/g"
	else
	    SEDCMD=" /global[[:blank:]]*{/a\ \ \ \ locking_type = $LOCKING_TYPE"
	fi
    fi

    if [ -n "$LOCKINGLIBDIR" ]; then
        if [ "$have_dir" = 0 ] 
            then
            SEDCMD="${SEDCMD}\ns'^[[:blank:]]*library_dir[[:blank:]]*=.*'\ \ \ \ library_dir = \"$LOCKINGLIBDIR\"'g"
        else
            SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ library_dir = \"$LOCKINGLIBDIR\""
        fi
    fi

    if [ -n "$LOCKINGLIB" ]; then
        if [ "$have_library" = 0 ]
            then
            SEDCMD="${SEDCMD}\ns/^[[:blank:]]*locking_library[[:blank:]]*=.*/\ \ \ \ locking_library = \"$LOCKINGLIB\"/g"
        else
            SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ locking_library = \"$LOCKINGLIB\""
        fi
    fi

    if [ "$have_use_lvmetad" = 0 ]
    then
        SEDCMD="${SEDCMD}\ns'^[[:blank:]]*use_lvmetad[[:blank:]]*=.*'\ \ \ \ use_lvmetad = $USE_LVMETAD'g"
    else
        SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ use_lvmetad = $USE_LVMETAD"
    fi

    echo -e "$SEDCMD" > "$SCRIPTFILE"
    sed  <"$CONFIGFILE" >"$TMPFILE" -f "$SCRIPTFILE"
    if [ $? != 0 ]
    then
	echo "sed failed, $CONFIGFILE not updated"
	exit 15
    fi
fi

# Now we have a suitably editted config file in a temp place,
# backup the original and copy our new one into place.

cp "$CONFIGFILE" "$CONFIGFILE.lvmconfold"
if [ $? != 0 ]
    then
    echo "failed to backup old config file, $CONFIGFILE not updated"
    exit 2
fi

cp "$TMPFILE" "$CONFIGFILE"
if [ $? != 0 ]
    then
    echo "failed to copy new config file into place, check $CONFIGFILE is still OK"
    exit 3
fi

rm -f "$SCRIPTFILE" "$TMPFILE"

function set_service {
    local type=$1
    local action=$2
    shift 2

    if [ "$type" = "systemd" ]; then
        if [ "$action" = "activate" ]; then
            for i in "$@"; do
                unset LoadState
                eval "$($SYSTEMCTL_BIN show "$i" -p LoadState 2>/dev/null)"
                test  "$LoadState" = "loaded" || continue
                $SYSTEMCTL_BIN enable "$i"
                if [ "$START_STOP_SERVICES" = 1 ]; then
                    $SYSTEMCTL_BIN start "$i"
                fi
            done
        elif [ "$action" = "deactivate" ]; then
            for i in "$@"; do
                unset LoadState
                eval "$($SYSTEMCTL_BIN show "$i" -p LoadState 2>/dev/null)"
                test  "$LoadState" = "loaded" || continue
                "$SYSTEMCTL_BIN" disable "$i"
                if [ "$START_STOP_SERVICES" = 1 ]; then
                    "$SYSTEMCTL_BIN" stop "$i"
                fi
            done
        fi
    elif [ "$type" = "sysv" ]; then
        if [ "$action" = "activate" ]; then
            for i in "$@"; do
                "$CHKCONFIG_BIN" --list "$i" > /dev/null || continue
                "$CHKCONFIG_BIN" "$i" on
                if [ "$START_STOP_SERVICES" = 1 ]; then
                    "$SERVICE_BIN" "$i" start
                fi
            done
        elif [ "$action" = "deactivate" ]; then
            for i in "$@"; do
                "$CHKCONFIG_BIN" --list "$i" > /dev/null  || continue
                if [ "$START_STOP_SERVICES" = 1 ]; then
                    "$SERVICE_BIN" "$i" stop
                fi
                "$CHKCONFIG_BIN" "$i" off
            done
        fi
    fi
}

# Start/stop and enable/disable services if needed.

if [ "$HANDLE_SERVICES" = 1 ]; then

    SYSTEMCTL_BIN=$(which systemctl 2>/dev/null)
    CHKCONFIG_BIN=$(which chkconfig 2>/dev/null)
    SERVICE_BIN=$(which service 2>/dev/null)

    # Systemd services
    if [ -n "$SYSTEMCTL_BIN" ]; then
        if [ "$USE_LVMETAD" = 0 ]; then
            set_service systemd deactivate lvm2-lvmetad.service lvm2-lvmetad.socket
        else
            set_service systemd activate lvm2-lvmetad.socket
        fi

        if [ "$LOCKING_TYPE" = 3 ]; then
            set_service systemd activate lvm2-cluster-activation.service
            if [ "$HANDLE_MIRROR_SERVICE" = 1 ]; then
                set_service activate lvm2-cmirrord.service
            fi
        else
            set_service systemd deactivate lvm2-cluster-activation.service
            if [ "$HANDLE_MIRROR_SERVICE" = 1 ]; then
                set_service systemd deactivate lvm2-cmirrord.service
            fi
        fi

    # System V init scripts
    elif [ -n "$SERVICE_BIN" ] && [ -n "$CHKCONFIG_BIN" ]; then
        if [ "$USE_LVMETAD" = 0 ]; then
            set_service sysv deactivate lvm2-lvmetad
        else
            set_service sysv activate lvm2-lvmetad
        fi

        if [ "$LOCKING_TYPE" = 3 ]; then
            set_service sysv activate clvmd
            if [ "$HANDLE_MIRROR_SERVICE" = 1 ]; then
                set_service sysv activate cmirrord
            fi
        else
            set_service sysv deactivate clvmd
            if [ "$HANDLE_MIRROR_SERVICE" = 1 ]; then
                set_service sysv deactivate cmirrord
            fi
        fi

    # None of the service tools found, error out
    else
        echo "Missing tools to handle services"
        exit 20
    fi
fi
