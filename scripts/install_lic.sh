#! /bin/sh

# Helper script used by the ESOS Web UI to handle installing the license file
# using sudo. If the given destination file already exists, rename it using a
# suffix of ".old" and install the source file to the destination.

if [ ${#} -ne 2 ]; then
    echo "This script takes two arguments:"
    echo "SRC_LIC_FILE DST_LIC_FILE"
    exit 1
else
    src_lic=${1}
    dst_lic=${2}
    if [ -f "${dst_lic}" ]; then
        mv -f ${dst_lic} ${dst_lic}.old || exit 1
    fi
    install -m 0644 ${src_lic} ${dst_lic} || exit 1
fi

