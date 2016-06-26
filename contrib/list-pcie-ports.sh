#!/bin/bash
#
# Copyright (c) 2013, Dell Inc.
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# Author: Jose De la Rosa <jose_de_la_rosa@dell.com>
#
# List devices attached to PCIe ports. Shows output as:
# PCI1: Empty
# PCI2: LSI Logic / Symbios Logic MegaRAID SAS 2208 [Thunderbolt] (rev 01)
# PCI3: Broadcom Corporation NetXtreme II BCM5709 Gigabit Ethernet (rev 20) 

bold="\033[1m"
end="\033[0m"
declare -a Port
DMIDECODE=/usr/sbin/dmidecode

# Minimum dmidecode version required
majreq=2
minreq=10

usage() {
   echo "Usage: `basename $0` [options]"
   echo
   echo "Options are mutually exclusive:"
   echo " -l    list devices"
   echo " -t    print type of port, i.e. x4 PCI Express"
   echo " -h    print this menu"
   exit 1
}

areweroot() {
   [ `id | sed 's/uid=\([0-9]*\)(.*/\1/'` -eq 0 ] && return
   echo "You must be root to run this script."; exit 1
}

checkvers() {
   # scripts don't handle floating numbers, split into major and minor versions
   maj=`$DMIDECODE -V | cut -d"." -f1`
   min=`$DMIDECODE -V | cut -d"." -f2`
   if [ $maj -ge $majreq ] ; then	# should be true, but check
      [ $min -ge $minreq ] && return 0	# ok
   fi
   echo "dmidecode version requirement not met!"
   echo -e "minimum version required is ${bold}${majreq}.${minreq}${end} --> found ${bold}${maj}.${min}${end}"
   echo "you can find latest at http://download.savannah.gnu.org/releases/dmidecode/"
   echo "exiting"
   exit 1
}

get_slot() {
   line=$*
   slot=`echo $line | sed 's/Designation: //g'`
   Port=("${Port[@]}" slot=$slot)
}

get_type() {
   line=$*
   desc=`echo $line | sed 's/Type: //g' | sed 's/ /-/g'` # replace blanks with -
   Port=("${Port[@]}" type=$desc "addr=Empty")
}

get_addr() {
   line=$*
   addr=`echo $line | sed 's/Bus Address: //g'`
   # remove last element ("Empty") and replace it with valid address
   pos=${#Port[@]}
   unset Port[$pos-1]
   Port=("${Port[@]}" addr=$addr)
}

getdmidecode() {
   # Get PCIe port information, store in temporary file
   output=$(mktemp)
   $DMIDECODE -t slot > $output
   exec 3<$output
   while read line 0<&3 ; do
      f=`echo $line | grep Designation`
      [ ! X"$f" = "X" ] && get_slot $line
      f=`echo $line | grep "Type"`
      [ ! X"$f" = "X" ] && get_type $line
      f=`echo $line | grep "Bus Address"`
      [ ! X"$f" = "X" ] && get_addr $line
   done
   exec 3<&-
   rm -f $output			# remove tmp file
}

printtype() {
   for el in ${Port[@]} ; do
      if [ `echo $el | cut -d"=" -f1` = "slot" ] ; then
         slot=`echo $el | cut -d"=" -f2`
      elif [ `echo $el | cut -d"=" -f1` = "type" ] ; then
         type=`echo $el | cut -d"=" -f2- | sed 's/-/ /g'`
         echo -e "${bold}${slot}:${end} $type"
      fi 
   done
}

printlist() {
   for el in ${Port[@]} ; do
      if [ `echo $el | cut -d"=" -f1` = "slot" ] ; then
         slot=`echo $el | cut -d"=" -f2`
      elif [ `echo $el | cut -d"=" -f1` = "addr" ] ; then
         addr=`echo $el | cut -d"=" -f2`
         echo -ne "${bold}${slot}:${end}"
         if [ ! $addr = "Empty" ] ; then
            addrs=`echo $addr | cut -d":" -f2-`	# strip PCI domain from address
            dev=`lspci -s $addrs`		# clean up first line
            dev=`echo $dev | cut -d" " -f2- | cut -d":" -f2- | sed -e 's/([^()]*)//g'`
            echo -e "${bold}${dev}${end}"
            lspci -s $addrs -v | sed 1d	# print details except first line (doing above)
         else
            echo " $addr"
         fi
      fi 
   done
}

areweroot
checkvers	# check for version as early releases don't provide all data

# Process args
[ $# -eq 0 ] && usage
argCnt=0
listf=0
typef=0
while [ $# -ge 1 ] ; do
   argList[$argCnt]=$1
   shift 1
   ((argCnt++))
done

for arg in ${argList[@]} ; do
   [ "$arg" = "-h" ] && usage
   [ "$arg" = "-t" ] && typef=1
   [ "$arg" = "-l" ] && listf=1
done

getdmidecode
if [ $typef = 1 ] ; then
   printtype
elif [ $listf = 1 ] ; then
   printlist
fi
