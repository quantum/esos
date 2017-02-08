#! /bin/sh

# This simple script is used to mass-update all package distribution files (eg,
# tarballs). It is intended to be used only by ESOS developers, and will not
# update any software on a running ESOS system. The script takes two arguments,
# the first is the data file, and the second is the directory containing all
# of the new package files.

# The data file contains two columns delimited by a single space character. The
# first column contains the old/current file name (eg, lzo-2.06.tar.gz) and the # second column contains the new package file name (eg, lzo-2.09.tar.gz).


# Make sure we got what we need
if [ ${#} -ne 2 ]; then
    echo "This script takes two arguments:"
    echo "PKG_DATA_FILE PATH_TO_NEW_FILES"
    exit 1
fi

# Loop over the package data file
while read line; do
    old_pkg=$(echo ${line} | cut -d' ' -f1)
    new_pkg=$(echo ${line} | cut -d' ' -f2)
    # Check that we actually have something to do
    if [ "x${old_pkg}" = "x${new_pkg}" ] || [ "x${new_pkg}" = "xNONE" ] ||
        [ -z "${old_pkg}" ] || [ -z "${new_pkg}" ]; then
        echo "Skipping '${line}'..."
        continue
    fi
    # Get the checksum values for the new files
    cd ${2} || exit 1
    md5_line=$(md5sum ${new_pkg}) || exit 1
    sha256_line=$(sha256sum ${new_pkg}) || exit 1
    cd - > /dev/null
    # Replace the old checksum lines with the new
    sed -i "/${old_pkg}/c\\${md5_line}" ./CHECKSUM.MD5 || exit 1
    sed -i "/${old_pkg}/c\\${sha256_line}" ./CHECKSUM.SHA256 || exit 1
    # Do the Makefile.in file too
    sed -i "s/${old_pkg}/${new_pkg}/" ./Makefile.in || exit 1
done < ${1}

