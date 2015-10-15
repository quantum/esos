#!/usr/bin/env bash
#########################################################################
# Script:       check_zpools.sh
# Purpose:      Nagios plugin to monitor status of zfs pool
# Authors:      Aldo Fabi             First version (2006-09-01)
#               Vitaliy Gladkevitch   Forked (2013-02-04)
#               Claudio Kuenzler      Complete redo, perfdata, etc (2013-2014)
# Doc:          http://www.claudiokuenzler.com/nagios-plugins/check_zpools.php
# History:
# 2006-09-01    Original first version
# 2006-10-04    Updated (no change history known)
# 2013-02-04    Forked and released
# 2013-05-08    Make plugin work on different OS, pepp up plugin
# 2013-05-09    Bugfix in exit code handling
# 2013-05-10    Removed old exit vars (not used anymore)
# 2013-05-21    Added performance data (percentage used)
# 2013-07-11    Bugfix in zpool health check
# 2014-02-10    Bugfix in threshold comparison
# 2014-03-11    Allow plugin to run without enforced thresholds
#########################################################################
### Begin vars
STATE_OK=0 # define the exit code if status is OK
STATE_WARNING=1 # define the exit code if status is Warning
STATE_CRITICAL=2 # define the exit code if status is Critical
STATE_UNKNOWN=3 # define the exit code if status is Unknown
# Set path
PATH=$PATH:/usr/sbin:/sbin
export PATH
### End vars
#########################################################################
help="check_zpools.sh (c) 2006-2014 several authors\n
Usage: $0 -p (poolname|ALL) [-w warnpercent] [-c critpercent]\n
Example: $0 -p ALL -w 80 -c 90"
#########################################################################
# Check necessary commands are available
for cmd in zpool awk [
do
 if ! `which ${cmd} 1>/dev/null`
 then
 echo "UNKNOWN: ${cmd} does not exist, please check if command exists and PATH is correct"
 exit ${STATE_UNKNOWN}
 fi
done
#########################################################################
# Check for people who need help - aren't we all nice ;-)
if [ "${1}" = "--help" -o "${#}" = "0" ];
       then
       echo -e "${help}";
       exit ${STATE_UNKNOWN};
fi
#########################################################################
# Get user-given variables
while getopts "p:w:c:" Input;
do
       case ${Input} in
       p)      pool=${OPTARG};;
       w)      warn=${OPTARG};;
       c)      crit=${OPTARG};;
       *)      echo -e $help
               exit $STATE_UNKNOWN
               ;;
       esac
done
#########################################################################
# Did user obey to usage?
if [ -z $pool ]; then echo -e $help; exit ${STATE_UNKNOWN}; fi
#########################################################################
# Verify threshold sense
if [[ -n $warn ]] && [[ -z $crit ]]; then echo "Both warning and critical thresholds must be set"; exit $STATE_UNKNOWN; fi
if [[ -z $warn ]] && [[ -n $crit ]]; then echo "Both warning and critical thresholds must be set"; exit $STATE_UNKNOWN; fi
if [[ $warn -gt $crit ]]; then echo "Warning threshold cannot be greater than critical"; exit $STATE_UNKNOWN; fi
#########################################################################
# What needs to be checked?
## Check all pools
if [ $pool = "ALL" ]
then
  POOLS=($(zpool list -Ho name))
  p=0
  for POOL in ${POOLS[*]}
  do 
    CAPACITY=$(zpool list -Ho capacity $POOL | awk -F"%" '{print $1}')
    HEALTH=$(zpool list -Ho health $POOL)
    # Check with thresholds
    if [[ -n $warn ]] && [[ -n $crit ]]
    then
      if [[ $CAPACITY -ge $crit ]]
      then error[${p}]="POOL $POOL usage is CRITICAL (${CAPACITY}%)"; fcrit=1
      elif [[ $CAPACITY -ge $warn && $CAPACITY -lt $crit ]]
      then error[$p]="POOL $POOL usage is WARNING (${CAPACITY}%)"
      elif [ $HEALTH != "ONLINE" ]
      then error[${p}]="$POOL health is $HEALTH"; fcrit=1
      fi
    # Check without thresholds
    else 
      if [ $HEALTH != "ONLINE" ]
      then error[${p}]="$POOL health is $HEALTH"; fcrit=1
      fi
    fi
    perfdata[$p]="$POOL=${CAPACITY}% "
    let p++
  done

  if [[ ${#error[*]} -gt 0 ]]
  then 
    if [[ $fcrit -eq 1 ]]; then exit_code=2; else exit_code=1; fi
    echo "ZFS POOL ALARM: ${error[*]}|${perfdata[*]}"; exit ${exit_code}
  else echo "ALL ZFS POOLS OK (${POOLS[*]})|${perfdata[*]}"; exit 0
  fi
  
## Check single pool
else 
  CAPACITY=$(zpool list -Ho capacity $pool | awk -F"%" '{print $1}')
  HEALTH=$(zpool list -Ho health $pool)

  if [[ -n $warn ]] && [[ -n $crit ]]
  then 
    # Check with thresholds
    if [ $HEALTH != "ONLINE" ]; then echo "ZFS POOL $pool health is $HEALTH|$pool=${CAPACITY}%"; exit ${STATE_CRITICAL}
    elif [[ $CAPACITY -gt $crit ]]; then echo "ZFS POOL $pool usage is CRITICAL (${CAPACITY}%|$pool=${CAPACITY}%)"; exit ${STATE_CRITICAL}
    elif [[ $CAPACITY -gt $warn && $CAPACITY -lt $crit ]]; then echo "ZFS POOL $pool usage is WARNING (${CAPACITY}%)|$pool=${CAPACITY}%"; exit ${STATE_WARNING}
    else echo "ALL ZFS POOLS OK ($pool)|$pool=${CAPACITY}%"; exit ${STATE_OK}
    fi
  else
    # Check without thresholds
    if [ $HEALTH != "ONLINE" ]
    then echo "ZFS POOL $pool health is $HEALTH|$pool=${CAPACITY}%"; exit ${STATE_CRITICAL}
    else echo "ALL ZFS POOLS OK ($pool)|$pool=${CAPACITY}%"; exit ${STATE_OK}
    fi
  fi 

fi

echo "UKNOWN - Should never reach this part"
exit ${STATE_UNKNOWN}
