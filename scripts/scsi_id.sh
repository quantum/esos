#! /bin/sh

# Return a unique identifier for the SCSI device argument. We first try
# the device identification page (0x83) and if there isn't one for the given
# device, we use the unit serial number page (0x80).

if [ $# -ne 1 ]; then
    exit 1
fi

# First try the device identification page
sg_inq_result="$(sg_inq --page=0x83 --export ${1} 2>&1)"
if [ ${?} -eq 0 ]; then
    export ${sg_inq_result}
    if [ "x${SCSI_IDENT_LUN_NAA}" != "x" ]; then
        echo "LUN_NAA-${SCSI_IDENT_LUN_NAA}"
        exit 0
     elif [ "x${SCSI_IDENT_LUN_EUI64}" != "x" ]; then
        echo "LUN_EUI64-${SCSI_IDENT_LUN_EUI64}"
        exit 0
     fi
fi

# Finally try the unit serial number page if we fell through from above
sg_inq_result="$(sg_inq --page=0x80 --export ${1} 2>&1)"
if [ ${?} -eq 0 ]; then
    export ${sg_inq_result}
    if [ "x${SCSI_IDENT_SERIAL}" != "x" ]; then
        echo "SERIAL-${SCSI_IDENT_SERIAL}"
        exit 0
    else
        exit 1
    fi
else
    exit 1
fi
