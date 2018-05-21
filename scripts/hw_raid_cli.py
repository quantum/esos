#! /usr/bin/python3

# A simple Python wrapper script for the propietary CLI RAID tools. The purpose
# of this script is abstract the provisioning of hardware RAID, so we can use
# the same command syntax for creating logical drives, hot spares, etc.

from optparse import OptionParser, OptionGroup
import os
import subprocess
import sys


# Common stuff
STORCLI_BIN = '/opt/sbin/storcli64'
PERCCLI_BIN = '/opt/sbin/perccli64'
ARCCONF_BIN = '/opt/sbin/arcconf'
CTRLR_TYPES = ['MegaRAID', 'PERC', 'AACRAID']


class MegaRAID():
    def __init__(self):
        self.working = self.__is_tool_avail()

    def __is_tool_avail(self):
        # File has to exist and be executable
        return os.path.isfile(STORCLI_BIN) and os.access(STORCLI_BIN, os.X_OK)

    def __get_ctrlr_cnt(self):
        # No controllers exist if the tool isn't available
        if not self.__is_tool_avail():
            return 0
        # Get the number of controllers
        command = STORCLI_BIN + ' show ctrlcount 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit == 0:
            for line in iter(stdout.splitlines()):
                if 'Controller Count =' in line:
                    count = line.split('=')[1]
                    return int(count)

    def list_ctrlrs(self, id_num=-1):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the controller information and print it
        for each in range(first_id, last_id):
            command = STORCLI_BIN + ' /c' + str(each) + ' show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                model = serial = ''
                for line in iter(stdout.splitlines()):
                    if 'Product Name =' in line:
                        model = line.split('=')[1].strip()
                    elif 'Serial Number =' in line:
                        serial = line.split('=')[1].strip()
                print('MegaRAID,%s,%s,%s' % (each, model, serial))

    def list_phy_drives(self, id_num=-1, avail_only=False):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the physical drives and print them
        for each in range(first_id, last_id):
            command = STORCLI_BIN + ' /c' + str(each) + '/eall/sall show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                dash_line_cnt = 0
                encl_id = slot_num = state = size = model = ''
                for line in iter(stdout.splitlines()):
                    if '----------' in line:
                        dash_line_cnt += 1
                        continue
                    if dash_line_cnt == 2:
                        # Each line is physical drive
                        p_drive_info = line.split()
                        if avail_only:
                            if p_drive_info[3] != '-':
                                continue
                        encl_slot = p_drive_info[0].split(':')
                        encl_id = encl_slot[0]
                        slot_num = encl_slot[1]
                        state = p_drive_info[2]
                        size = p_drive_info[4] + ' ' + p_drive_info[5]
                        model = p_drive_info[11]
                        print('MegaRAID,%s,%s,%s,%s,%s,%s' % (each, encl_id,
                                                              slot_num, state,
                                                              size, model))

    def list_log_drives(self, id_num=-1):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the logical drives and print them
        for each in range(first_id, last_id):
            command = STORCLI_BIN + ' /c' + str(each) + '/vall show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                dash_line_cnt = 0
                ld_id = raid_lvl = state = size = name = ''
                for line in iter(stdout.splitlines()):
                    if '----------' in line:
                        dash_line_cnt += 1
                        continue
                    if dash_line_cnt == 2:
                        # Each line is logical drive drive
                        l_drive_info = line.split()
                        dg_vd = l_drive_info[0].split('/')
                        ld_id = dg_vd[1]
                        raid_lvl = l_drive_info[1]
                        state = l_drive_info[2]
                        size = l_drive_info[7] + ' ' + l_drive_info[8]
                        if len(l_drive_info) == 10:
                            name = l_drive_info[9]
                        print('MegaRAID,%s,%s,%s,%s,%s,%s' % (each, ld_id,
                                                              raid_lvl, state,
                                                              size, name))

    def add_log_drive(self, ctrlr_id, raid_lvl, phys_drives, read_cache,
                      write_cache):
        # All arguments are required, so build the command and execute it
        if read_cache:
            read_opt = "ra"
        else:
            read_opt = "nora"
        if write_cache:
            write_opt = "wb"
        else:
            write_opt = "wt"
        command = STORCLI_BIN + ' /c' + str(ctrlr_id) + ' add vd r' + \
                  raid_lvl + ' drives=' + phys_drives + ' ' + read_opt + \
                  ' ' + write_opt + ' 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_log_drive(self, ctrlr_id, ld_id):
        # All arguments are required, so build the command and execute it
        command = STORCLI_BIN + ' /c' + str(ctrlr_id) + '/v' + \
                  str(ld_id) + ' del force 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def add_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = STORCLI_BIN + ' /c' + str(ctrlr_id) + '/e' + \
                  str(encl_id) + '/s' + str(slot_num) + \
                  ' add hotsparedrive 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = STORCLI_BIN + ' /c' + str(ctrlr_id) + '/e' + \
                  str(encl_id) + '/s' + str(slot_num) + \
                  ' delete hotsparedrive 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)


class PERC():
    def __init__(self):
        self.working = self.__is_tool_avail()

    def __is_tool_avail(self):
        # File has to exist and be executable
        return os.path.isfile(PERCCLI_BIN) and os.access(PERCCLI_BIN, os.X_OK)

    def __get_ctrlr_cnt(self):
        # No controllers exist if the tool isn't available
        if not self.__is_tool_avail():
            return 0
        # Get the number of controllers
        command = PERCCLI_BIN + ' show ctrlcount 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit == 0:
            for line in iter(stdout.splitlines()):
                if 'Controller Count =' in line:
                    count = line.split('=')[1]
                    return int(count)

    def list_ctrlrs(self, id_num=-1):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the controller information and print it
        for each in range(first_id, last_id):
            command = PERCCLI_BIN + ' /c' + str(each) + ' show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                model = serial = ''
                for line in iter(stdout.splitlines()):
                    if 'Product Name =' in line:
                        model = line.split('=')[1].strip()
                    elif 'Serial Number =' in line:
                        serial = line.split('=')[1].strip()
                print('PERC,%s,%s,%s' % (each, model, serial))

    def list_phy_drives(self, id_num=-1, avail_only=False):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the physical drives and print them
        for each in range(first_id, last_id):
            command = PERCCLI_BIN + ' /c' + str(each) + '/eall/sall show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                dash_line_cnt = 0
                encl_id = slot_num = state = size = model = ''
                for line in iter(stdout.splitlines()):
                    if '----------' in line:
                        dash_line_cnt += 1
                        continue
                    if dash_line_cnt == 2:
                        # Each line is physical drive
                        p_drive_info = line.split()
                        if avail_only:
                            if p_drive_info[3] != '-':
                                continue
                        encl_slot = p_drive_info[0].split(':')
                        encl_id = encl_slot[0]
                        slot_num = encl_slot[1]
                        state = p_drive_info[2]
                        size = p_drive_info[4] + ' ' + p_drive_info[5]
                        model = p_drive_info[11]
                        print('PERC,%s,%s,%s,%s,%s,%s' % (each, encl_id,
                                                          slot_num, state,
                                                          size, model))

    def list_log_drives(self, id_num=-1):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt()
            first_id = 0
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the logical drives and print them
        for each in range(first_id, last_id):
            command = PERCCLI_BIN + ' /c' + str(each) + '/vall show 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                dash_line_cnt = 0
                ld_id = raid_lvl = state = size = name = ''
                for line in iter(stdout.splitlines()):
                    if '----------' in line:
                        dash_line_cnt += 1
                        continue
                    if dash_line_cnt == 2:
                        # Each line is logical drive drive
                        l_drive_info = line.split()
                        dg_vd = l_drive_info[0].split('/')
                        ld_id = dg_vd[1]
                        raid_lvl = l_drive_info[1]
                        state = l_drive_info[2]
                        size = l_drive_info[7] + ' ' + l_drive_info[8]
                        if len(l_drive_info) == 10:
                            name = l_drive_info[9]
                        print('PERC,%s,%s,%s,%s,%s,%s' % (each, ld_id,
                                                          raid_lvl, state,
                                                          size, name))

    def add_log_drive(self, ctrlr_id, raid_lvl, phys_drives, read_cache,
                      write_cache):
        # All arguments are required, so build the command and execute it
        if read_cache:
            read_opt = "ra"
        else:
            read_opt = "nora"
        if write_cache:
            write_opt = "wb"
        else:
            write_opt = "wt"
        command = PERCCLI_BIN + ' /c' + str(ctrlr_id) + ' add vd r' + \
                  raid_lvl + ' drives=' + phys_drives + ' ' + read_opt + \
                  ' ' + write_opt + ' 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_log_drive(self, ctrlr_id, ld_id):
        # All arguments are required, so build the command and execute it
        command = PERCCLI_BIN + ' /c' + str(ctrlr_id) + '/v' + \
                  str(ld_id) + ' del force 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def add_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = PERCCLI_BIN + ' /c' + str(ctrlr_id) + '/e' + \
                  str(encl_id) + '/s' + str(slot_num) + \
                  ' add hotsparedrive 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = PERCCLI_BIN + ' /c' + str(ctrlr_id) + '/e' + \
                  str(encl_id) + '/s' + str(slot_num) + \
                  ' delete hotsparedrive 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)


class AACRAID():
    def __init__(self):
        self.working = self.__is_tool_avail()

    def __is_tool_avail(self):
        # File has to exist and be executable
        return os.path.isfile(ARCCONF_BIN) and os.access(ARCCONF_BIN, os.X_OK)

    def __get_ctrlr_cnt(self):
        # No controllers exist if the tool isn't available
        if not self.__is_tool_avail():
            return 0
        # Get the number of controllers
        command = ARCCONF_BIN + ' getversion 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit == 0:
            for line in iter(stdout.splitlines()):
                if 'Controllers found:' in line:
                    count = line.split(':')[1]
                    return int(count)

    def list_ctrlrs(self, id_num=-1):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt() + 1
            first_id = 1
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the controller information and print it
        for each in range(first_id, last_id):
            command = ARCCONF_BIN + ' getconfig ' + str(each) + ' ad 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                model = serial = ''
                for line in iter(stdout.splitlines()):
                    if 'Controller Model' in line:
                        model = line.split(':')[1].strip()
                    elif 'Controller Serial Number' in line:
                        serial = line.split(':')[1].strip()
                print('AACRAID,%s,%s,%s' % (each, model, serial))

    def list_phy_drives(self, id_num=-1, avail_only=False):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt() + 1
            first_id = 1
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the physical drives and print them
        for each in range(first_id, last_id):
            command = ARCCONF_BIN + ' getconfig ' + str(each) + ' pd 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                found_drive = False
                encl_id = slot_num = state = size = model = ''
                pd_in_use = False
                for line in iter(stdout.splitlines()):
                    if 'Device is a Hard drive' in line:
                        found_drive = True
                        continue
                    if found_drive and '         State' in line:
                        state = line.split(':')[1].strip()
                        continue
                    if found_drive and 'Reported Channel,Device(T:L)' in line:
                        encl_slot = line.split(':')[2].strip()
                        encl_slot_2 = encl_slot.split('(')[0]
                        encl_id = encl_slot_2.split(',')[0]
                        slot_num = encl_slot_2.split(',')[1]
                        continue
                    if found_drive and 'Model' in line:
                        model = line.split(':')[1].strip()
                        continue
                    if found_drive and 'Used Size' in line:
                        if line.split(':')[1].strip() == '0 MB':
                            pd_in_use = False
                        else:
                            pd_in_use = True
                    if found_drive and 'Total Size' in line:
                        size = line.split(':')[1].strip()
                        # This is the last line to find, so we're done here
                        found_drive = False
                        if avail_only:
                            if pd_in_use:
                                continue
                        print('AACRAID,%s,%s,%s,%s,%s,%s' % (each, encl_id,
                                                             slot_num, state,
                                                             size, model))

    def list_log_drives(self, id_num=-1, avail_only=False):
        # Either for all, or a specific controller
        if id_num == -1:
            last_id = self.__get_ctrlr_cnt() + 1
            first_id = 1
        else:
            last_id = id_num + 1
            first_id = id_num
        # Get the logical drives and print them
        for each in range(first_id, last_id):
            command = ARCCONF_BIN + ' getconfig ' + str(each) + ' ld 2>&1'
            process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                       shell=True)
            stdout, stderr = process.communicate()
            cmd_exit = process.returncode
            if cmd_exit == 0:
                found_drive = False
                ld_id = raid_lvl = state = size = name = ''
                pd_in_use = False
                for line in iter(stdout.splitlines()):
                    if 'Logical device number' in line:
                        ld_id = line.split(' ')[3].strip()
                        found_drive = True
                        continue
                    if found_drive and 'Logical device name' in line:
                        name = line.split(':')[1].strip()
                        continue
                    if found_drive and 'RAID level' in line:
                        raid_lvl = 'RAID' + line.split(':')[1].strip()
                        continue
                    if found_drive and 'Status of logical device' in line:
                        state = line.split(':')[1].strip()
                        continue
                    if found_drive and '   Size' in line:
                        size = line.split(':')[1].strip()
                        # This is the last line to find, so we're done here
                        found_drive = False
                        print('AACRAID,%s,%s,%s,%s,%s,%s' % (each, ld_id,
                                                             raid_lvl, state,
                                                             size, name))

    def add_log_drive(self, ctrlr_id, raid_lvl, phys_drives, read_cache,
                      write_cache):
        # All arguments are required, so build the command and execute it
        if read_cache:
            read_opt = "ron"
        else:
            read_opt = "roff"
        if write_cache:
            write_opt = "wb"
        else:
            write_opt = "wt"
        command = ARCCONF_BIN + ' create ' + str(ctrlr_id) + \
                  ' logicaldrive ' + read_opt + ' ' + write_opt + \
                  ' max ' + raid_lvl
        for each in phys_drives.split(','):
            encl_slot = each.split(':')
            command = command + ' ' + encl_slot[0] + ' ' + encl_slot[1]
        command = command + ' 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_log_drive(self, ctrlr_id, ld_id):
        # All arguments are required, so build the command and execute it
        command = ARCCONF_BIN + ' delete ' + str(ctrlr_id) + \
                  ' logicaldrive ' + str(ld_id) + ' noprompt 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def add_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = ARCCONF_BIN + ' setstate ' + str(ctrlr_id) + \
                  ' device ' + str(encl_id) + ' ' + str(slot_num) + \
                  ' hsp noprompt 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)

    def rem_hsp_drive(self, ctrlr_id, encl_id, slot_num):
        # All arguments are required, so build the command and execute it
        command = ARCCONF_BIN + ' setstate ' + str(ctrlr_id) + \
                  ' device ' + str(encl_id) + ' ' + str(slot_num) + \
                  ' rdy noprompt 2>&1'
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        cmd_exit = process.returncode
        if cmd_exit != 0:
            print(stdout)
            sys.exit(cmd_exit)


def main():
    # Parse options
    parser = OptionParser(conflict_handler='error')
    command_group = OptionGroup(parser, 'Commands', 'Use only one command '
                                                    'option at a time; combine '
                                                    'with other options below.')
    command_group.add_option('--list-controllers', dest='list_controllers',
                             action='store_true',
                             help='list available RAID controllers')
    command_group.add_option('--list-physical-drives',
                             dest='list_physical_drives', action='store_true',
                             help='list all physical drives')
    command_group.add_option('--list-logical-drives',
                             dest='list_logical_drives', action='store_true',
                             help='list all logical drives')
    command_group.add_option('--add-logical-drive', dest='add_logical_drive',
                             action='store_true',
                             help='add a new logical drive')
    command_group.add_option('--rem-logical-drive', dest='rem_logical_drive',
                             action='store_true',
                             help='remove an existing logical drive')
    command_group.add_option('--add-hot-spare', dest='add_hot_spare',
                             action='store_true',
                             help='add a new global hot spare drive')
    command_group.add_option('--rem-hot-spare', dest='rem_hot_spare',
                             action='store_true',
                             help='remove an existing global hot spare drive')
    parser.add_option_group(command_group)

    common_group = OptionGroup(parser, 'Options Available for All Commands')
    common_group.add_option('--type', dest='type',
                            help='the RAID controller hardware type',
                            metavar='TYPE', type='choice', choices=CTRLR_TYPES)
    common_group.add_option('--ctrlr-id', dest='ctrlr_id',
                            help='the RAID controller ID',
                            metavar='ID_NUM', type='int')
    parser.add_option_group(common_group)

    pdrives_group = OptionGroup(parser, 'Specific Options for the '
                                        '"--list-physical-drives" Command')
    pdrives_group.add_option('--avail-only', dest='avail_only',
                             action='store_true',
                             help='only display available/unused '
                                  'physical drives')
    parser.add_option_group(pdrives_group)

    add_ld_group = OptionGroup(parser, 'Specific Options for the '
                                       '"--add-logical-drive" Command')
    add_ld_group.add_option('--raid-level', dest='raid_level',
                            help='the RAID level of the new logical drive',
                            metavar='LEVEL', type='string')
    add_ld_group.add_option('--phys-drives', dest='phys_drives',
                            help='the enclosure/slot string of the physical '
                                 'drives (eg, "0:1,0:2") for the new '
                                 'logical drive',
                            metavar='DRIVES', type='string')
    add_ld_group.add_option('--no-read-cache', dest='no_read_cache',
                            action='store_true',
                            help='don\'t enable read-ahead cache '
                                 '(default is enabled)')
    add_ld_group.add_option('--no-write-cache', dest='no_write_cache',
                            action='store_true',
                            help='don\'t enable write-back cache '
                                 '(default is enabled)')
    parser.add_option_group(add_ld_group)

    rem_ld_group = OptionGroup(parser, 'Specific Options for the '
                                       '"--rem-logical-drive" Command')
    rem_ld_group.add_option('--rem-ld-id', dest='rem_ld_id',
                            help='the logical drive ID number of '
                                 'the LD to remove',
                            metavar='ID_NUM', type='int')
    parser.add_option_group(rem_ld_group)

    add_hsp_group = OptionGroup(parser, 'Specific Options for the '
                                        '"--add-hot-spare" Command')
    add_hsp_group.add_option('--add-encl-id', dest='add_encl_id',
                             help='the enclosure ID of the physical '
                                  'drive to add',
                             metavar='ID_NUM', type='int')
    add_hsp_group.add_option('--add-slot-num', dest='add_slot_num',
                             help='the slot number of the physical '
                                  'drive to add',
                             metavar='ID_NUM', type='int')
    parser.add_option_group(add_hsp_group)

    rem_hsp_group = OptionGroup(parser, 'Specific Options for the '
                                        '"--rem-hot-spare" Command')
    rem_hsp_group.add_option('--rem-encl-id', dest='rem_encl_id',
                             help='the enclosure ID of the hot spare to remove',
                             metavar='ID_NUM', type='int')
    rem_hsp_group.add_option('--rem-slot-num', dest='rem_slot_num',
                             help='the slot number of the hot spare to remove',
                             metavar='ID_NUM', type='int')
    parser.add_option_group(rem_hsp_group)
    (options, args) = parser.parse_args()

    # Make sure we only (or at least) got one command option
    cmd_count = 0
    if options.list_controllers:
        cmd_count += 1
    if options.list_physical_drives:
        cmd_count += 1
    if options.list_logical_drives:
        cmd_count += 1
    if options.add_logical_drive:
        cmd_count += 1
    if options.rem_logical_drive:
        cmd_count += 1
    if options.add_hot_spare:
        cmd_count += 1
    if options.rem_hot_spare:
        cmd_count += 1
    if cmd_count < 1:
        parser.error('you must specify one command option')
    if cmd_count > 1:
        parser.error('all command options are mutually exclusive, '
                     'pick only one')

    # Create objects for the RAID CLI tools
    megaraid = MegaRAID()
    perc = PERC()
    aacraid = AACRAID()

    # Perform checks for the specific command option,
    # and execute the action if it's all good
    if options.list_controllers:
        if options.type is not None or options.ctrlr_id is not None:
            if not (options.type is not None and options.ctrlr_id is not None):
                parser.error('--type and --ctrlr-id must both be specified')
            else:
                if options.type == 'MegaRAID':
                    if megaraid.working:
                        megaraid.list_ctrlrs(options.ctrlr_id)
                elif options.type == 'PERC':
                    if perc.working:
                        perc.list_ctrlrs(options.ctrlr_id)
                elif options.type == 'AACRAID':
                    if aacraid.working:
                        aacraid.list_ctrlrs(options.ctrlr_id)
        else:
            if megaraid.working:
                megaraid.list_ctrlrs()
            if perc.working:
                perc.list_ctrlrs()
            if aacraid.working:
                aacraid.list_ctrlrs()

    elif options.list_physical_drives:
        if options.type is not None or options.ctrlr_id is not None:
            if not (options.type is not None and options.ctrlr_id is not None):
                parser.error('--type and --ctrlr-id must both be specified')
            else:
                if options.type == 'MegaRAID':
                    if megaraid.working:
                        megaraid.list_phy_drives(options.ctrlr_id,
                                                 options.avail_only)
                elif options.type == 'PERC':
                    if perc.working:
                        perc.list_phy_drives(options.ctrlr_id,
                                             options.avail_only)
                elif options.type == 'AACRAID':
                    if aacraid.working:
                        aacraid.list_phy_drives(options.ctrlr_id,
                                                options.avail_only)
        else:
            if megaraid.working:
                megaraid.list_phy_drives(-1, options.avail_only)
            if perc.working:
                perc.list_phy_drives(-1, options.avail_only)
            if aacraid.working:
                aacraid.list_phy_drives(-1, options.avail_only)

    elif options.list_logical_drives:
        if options.type is not None or options.ctrlr_id is not None:
            if not (options.type is not None and options.ctrlr_id is not None):
                parser.error('--type and --ctrlr-id must both be specified')
            else:
                if options.type == 'MegaRAID':
                    if megaraid.working:
                        megaraid.list_log_drives(options.ctrlr_id)
                elif options.type == 'PERC':
                    if perc.working:
                        perc.list_log_drives(options.ctrlr_id)
                elif options.type == 'AACRAID':
                    if aacraid.working:
                        aacraid.list_log_drives(options.ctrlr_id)
        else:
            if megaraid.working:
                megaraid.list_log_drives()
            if perc.working:
                perc.list_log_drives()
            if aacraid.working:
                aacraid.list_log_drives()

    elif options.add_logical_drive:
        if not (options.type is not None and options.ctrlr_id is not None):
            parser.error('--type and --ctrlr-id are required options')
        if not (options.raid_level is not None and
                        options.phys_drives is not None):
            parser.error('--raid-level and --phys-drives are required options')
        if options.no_read_cache:
            read_cache = False
        else:
            read_cache = True
        if options.no_write_cache:
            write_cache = False
        else:
            write_cache = True
        if options.type == 'MegaRAID':
            if megaraid.working:
                megaraid.add_log_drive(options.ctrlr_id, options.raid_level,
                                       options.phys_drives, read_cache,
                                       write_cache)
        elif options.type == 'PERC':
            if perc.working:
                perc.add_log_drive(options.ctrlr_id, options.raid_level,
                                   options.phys_drives, read_cache,
                                   write_cache)
        elif options.type == 'AACRAID':
            if aacraid.working:
                aacraid.add_log_drive(options.ctrlr_id, options.raid_level,
                                      options.phys_drives, read_cache,
                                      write_cache)

    elif options.rem_logical_drive:
        if not (options.type is not None and options.ctrlr_id is not None):
            parser.error('--type and --ctrlr-id are required options')
        if options.rem_ld_id is None:
            parser.error('--rem-ld-id is a required option')
        if options.type == 'MegaRAID':
            if megaraid.working:
                megaraid.rem_log_drive(options.ctrlr_id, options.rem_ld_id)
        elif options.type == 'PERC':
            if perc.working:
                perc.rem_log_drive(options.ctrlr_id, options.rem_ld_id)
        elif options.type == 'AACRAID':
            if aacraid.working:
                aacraid.rem_log_drive(options.ctrlr_id, options.rem_ld_id)

    elif options.add_hot_spare:
        if not (options.type is not None and options.ctrlr_id is not None):
            parser.error('--type and --ctrlr-id are required options')
        if not (options.add_encl_id is not None and
                        options.add_slot_num is not None):
            parser.error('--add-encl-id and --add-slot-num are '
                         'required options')
        if options.type == 'MegaRAID':
            if megaraid.working:
                megaraid.add_hsp_drive(options.ctrlr_id, options.add_encl_id,
                                       options.add_slot_num)
        elif options.type == 'PERC':
            if perc.working:
                perc.add_hsp_drive(options.ctrlr_id, options.add_encl_id,
                                   options.add_slot_num)
        elif options.type == 'AACRAID':
            if aacraid.working:
                aacraid.add_hsp_drive(options.ctrlr_id, options.add_encl_id,
                                      options.add_slot_num)

    elif options.rem_hot_spare:
        if not (options.type is not None and options.ctrlr_id is not None):
            parser.error('--type and --ctrlr-id are required options')
        if not (options.rem_encl_id is not None and
                        options.rem_slot_num is not None):
            parser.error('--rem-encl-id and --rem-slot-num are '
                         'required options')
        if options.type == 'MegaRAID':
            if megaraid.working:
                megaraid.rem_hsp_drive(options.ctrlr_id, options.rem_encl_id,
                                       options.rem_slot_num)
        elif options.type == 'PERC':
            if perc.working:
                perc.rem_hsp_drive(options.ctrlr_id, options.rem_encl_id,
                                   options.rem_slot_num)
        elif options.type == 'AACRAID':
            if aacraid.working:
                aacraid.rem_hsp_drive(options.ctrlr_id, options.rem_encl_id,
                                      options.rem_slot_num)


if __name__ == '__main__':
    main()
