#! /usr/bin/python3

# This script is used to install propietary CLI RAID tools in ESOS. With no
# arguments given, the script will attempt to read a JSON file hosted by the
# ESOS project and display a list of RAID tools and a download location for
# each. The user is instructed to download the packages needed, then copy them
# up to the ESOS host (eg, via SCP) and run this script with the path to the
# package as the argument; this will extract and install the RAID tool.

import urllib.request
import json
import sys
import subprocess
import tempfile
import shutil
import os


# Script settings
TOOL_JSON_URL = 'http://download.esos-project.com/raid_cli_tools.json'


if len(sys.argv) == 2:
    # Run the shell script recipe for a matching package
    pkg_file = os.path.abspath(sys.argv[1])
    print('Attempting to install "%s" on this host...\n' % pkg_file)
    temp_dir = tempfile.mkdtemp()
    if 'StorCLI' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && unzip -o ' + pkg_file + ' && unzip -o '
            'versionChangeSet/univ_viva_cli_rel/storcli_all_os.zip && '
            'rpm2cpio storcli_all_os/Linux/storcli-*.rpm | cpio -idmv && '
            'install -m 0755 opt/MegaRAID/storcli/storcli64 /opt/sbin/ && '
            'cp opt/MegaRAID/storcli/libstorelibir* /opt/lib/', shell=True)
    elif 'perccli' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && tar xvfz ' + pkg_file + ' && rpm2cpio '
            'perccli-*.noarch.rpm | cpio -idmv && install -m 0755 '
            'opt/MegaRAID/perccli/perccli64 /opt/sbin/ && cp '
            'opt/MegaRAID/perccli/libstorelibir* /opt/lib/', shell=True)
    elif 'arcconf' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && unzip -o ' + pkg_file + ' && install '
            '-m 0755 linux_x64/cmdline/arcconf /opt/sbin/', shell=True)
    elif 'hpacucli' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && rpm2cpio ' + pkg_file + ' | cpio -idmv '
            '&& install -m 0755 opt/compaq/hpacucli/bld/.hpacucli '
            '/opt/sbin/hpacucli && cp opt/compaq/hpacucli/bld/*.so '
            '/opt/lib/', shell=True)
    elif 'hpssacli' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && rpm2cpio ' + pkg_file + ' | cpio -idmv '
            '&& install -m 0755 opt/hp/hpssacli/bld/hpssacli '
            '/opt/sbin/', shell=True)
    elif 'linuxcli' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && unzip -o ' + pkg_file + ' && install '
            '-m 0755 linuxcli_*/x86_64/cli64 /opt/sbin/', shell=True)
    elif '3DM2_CLI' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && unzip -o ' + pkg_file + ' && tar xvfz '
            'tdmCliLnx.tgz && install -m 0755 tw_cli.x86_64 '
            '/opt/sbin/', shell=True)
    elif 'MegaCLI' in pkg_file:
        result = subprocess.run(
            'cd ' + temp_dir + ' && unzip -o ' + pkg_file + ' && rpm2cpio '
            'Linux/MegaCli-*.rpm | cpio -idmv && install -m 0755 '
            'opt/MegaRAID/MegaCli/MegaCli64 /opt/sbin/', shell=True)
    else:
        print('We don\'t know how to extract and install this file!')
        sys.exit(1)
    shutil.rmtree(temp_dir)
    if result.returncode == 0:
        print('\nThe RAID CLI tool was installed successfully.')
    else:
        print('\nAn error occurred while installing this package!')
        sys.exit(1)

else:
    # No package file was given, so print available options and usage
    try:
        print('Fetching JSON file with RAID tool information...\n')
        with urllib.request.urlopen(TOOL_JSON_URL) as url:
            data = json.loads(url.read().decode())
            for each in data:
                print(each['desc'])
                print('File Name: %s' % each['file'])
                print('Download URL: %s\n' % each['url'])
            print('Fetch the desired RAID CLI tools and upload the files to')
            print('this ESOS machine, then install each using this script')
            print('with the path to the package/archive as the argument.\n')
            print('Sample install syntax: %s /tmp/8-07-14_MegaCLI.zip' %
                  __file__)
    except Exception:
        print('An error occurred while fetching the JSON file! For a list of')
        print('available RAID CLI tools, visit this URL: %s' % TOOL_JSON_URL)
