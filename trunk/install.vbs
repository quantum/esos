' $Id$

option explicit

' Make sure the scripting host is cscript.exe
dim sh_engine, shell_app
sh_engine = lcase(mid(wscript.fullname, instrrev(wscript.fullname, "\") + 1))
if not sh_engine = "cscript.exe" then
    set shell_app = createobject("shell.application")
    shell_app.shellexecute "cscript.exe", chr(34) & wscript.scriptfullname & chr(34) & " uac", "", "runas", 1
    wscript.quit
end if

wscript.echo "*** Enterprise Storage OS Install Script ***"
wscript.echo ""

' Setup
on error resume next
dim wsh_shell, file_sys, wmi_service
set wsh_shell = createobject("wscript.shell")
set file_sys = createobject("scripting.filesystemobject")
set wmi_service = getobject("winmgmts:\\.\root\cimv2")
wsh_shell.currentdirectory = file_sys.getparentfoldername(wscript.scriptfullname)

' Settings
dim install_common, base_path, md5sum_prog, sha256sum_prog, dd_prog, ext2fsd_path, ext2fsd_setup, mount_prog, ext2mgr_prog, bootice_prog, sevenzip_prog
install_common = "install_common"
base_path = wsh_shell.currentdirectory
md5sum_prog = base_path & "\checksum_utils\md5sum.exe"
sha256sum_prog = base_path & "\checksum_utils\sha256sum.exe"
dd_prog = base_path & "\dd-0.6beta3\dd.exe"
ext2fsd_path = base_path & "\ext2fsd-0.52"
ext2fsd_setup = ext2fsd_path & "\setup.bat"
mount_prog = ext2fsd_path & "\mount.exe"
ext2mgr_prog = ext2fsd_path & "\ext2mgr.exe"
bootice_prog = base_path & "\bootice_x86_v1.3.2.1\bootice_x86.exe"
sevenzip_prog = base_path & "\7zip-9.20\7z.exe"

' Verify the checksums
dim md5sum_cmd, sha256sum_cmd
wscript.echo "### Verifying checksums..."
md5sum_cmd = md5sum_prog & " -w -c dist_md5sum.txt"
exec_cmd wsh_shell, md5sum_cmd
sha256sum_cmd = sha256sum_prog & " -w -c dist_sha256sum.txt"
exec_cmd wsh_shell, sha256sum_cmd
wscript.echo

' List available devices
dim dd_list_cmd
wscript.echo "### Here is a list of block devices on this machine:"
dd_list_cmd = dd_prog & " --list"
exec_cmd wsh_shell, dd_list_cmd
wscript.echo

' Get USB flash drive (volume) choice from user
dim vol_path
wscript.echo "### Please type the full path of your USB drive volume (eg, \\?\device\harddisk2\partition0):"
vol_path = wscript.stdin.readline
wscript.echo
if vol_path = "" then
    exit_app
end if

' Get confirmation from user
dim confirm_write, cwd_folder, image_file, file, dd_write_cmd
wscript.echo "### Proceeding will completely wipe the '" & vol_path & "' volume. Are you sure?"
confirm_write = wscript.stdin.readline
wscript.echo
if confirm_write = "yes" or confirm_write = "y" then
    ' Find the image file name
    set cwd_folder = file_sys.getfolder(".")
    for each file in cwd_folder.files
        if file_sys.getextensionname(file) = "img" then
            set image_file = file
            exit for
        end if
    next
    if image_file <> "" then
        ' Write the image file
        wscript.echo "### Writing " & image_file & " to " & _
            vol_path & "; this may take a while..."
        dd_write_cmd = dd_prog & " if=" & image_file & " of=" & _
            vol_path & " bs=1M"
        exec_cmd wsh_shell, dd_write_cmd
        wscript.echo
        wscript.echo "### It appears the image was successfully written to disk (check for error messages)!"
    else
        wscript.echo "ERROR: No image file was found!"
        wscript.echo
        exit_app
    end if
else
    ' User bailed
    wscript.echo
    exit_app
end if

' Continue on to installing proprietary CLI tools, or finished
wscript.echo
wscript.echo
wscript.echo "*** If you would like to add any RAID controller management utilities, press ENTER to continue; otherwise press CTRL-C to quit, your ESOS USB drive install is complete. ***"
wscript.stdin.readline
wscript.echo
wscript.echo

' The ext2fsd software is required
dim ext2fsd_inf, confirm_ext2fsd, orig_cwd, ext2_load_cmd
ext2fsd_inf = wsh_shell.expandenvironmentstrings("%windir%") & "\inf\ext2fsd.inf"
if not file_sys.fileexists(ext2fsd_inf) then
    ' Ask if we can install ext2fsd
    wscript.echo "### In order to add the RAID controller CLI tools, we need to install the ext2fsd package. Is this okay?"
    confirm_ext2fsd = wscript.stdin.readline
    wscript.echo
    if confirm_ext2fsd = "yes" or confirm_ext2fsd = "y" then
        ' Load ext2fsd
        orig_cwd = wsh_shell.currentdirectory
        wsh_shell.currentdirectory = ext2fsd_path
        ext2_load_cmd = ext2fsd_setup & " install"
        exec_cmd wsh_shell, ext2_load_cmd
        wsh_shell.currentdirectory = orig_cwd
    else
        ' User bailed
        wscript.echo
        exit_app
    end if
end if

' Read the common install variables file
dim text_file, line_str, line_parts
set text_file = file_sys.opentextfile(install_common, 1)
do until text_file.atendofstream
    ' Read each line and execute to set variable
    line_str = text_file.readline
    if line_str <> "" then
        line_parts = split(line_str, "=")
        if line_parts(0) <> "" then
            executeglobal "dim " & line_parts(0)
            executeglobal line_str
        end if
    end if
loop

' Setup the temporary directory
dim temp_base, temp_name, temp_folder
set temp_base = file_sys.getspecialfolder(2)
temp_name = file_sys.gettempname
set temp_folder = file_sys.createfolder(temp_base & "\" & temp_name)

' Display proprietary tool information and download instructions
dim split_tools, tool, tool_desc, tool_file, tool_url, tool_path, install_list
split_tools = split(PROP_TOOLS, " ")
for each tool in split_tools
    execute "tool_desc = TOOL_DESC_" & tool
    wscript.echo "### " & tool_desc
    execute "tool_file = TOOL_FILE_" & tool
    wscript.echo "### Required file: " & tool_file
    execute "tool_url = TOOL_URL_" & tool
    wscript.echo "### Download URL: " & tool_url
    echo "### Place downloaded file in this directory: " & temp_folder.path
    wscript.echo
next

' Prompt user to continue
wscript.echo
wscript.echo "*** Once the file(s) have been loaded and placed in the '" & temp_folder.path & "' directory, press ENTER to install the RAID controller CLI tools on your new ESOS USB drive. ***"
wscript.stdin.readline

' Check downloaded packages
wscript.echo
wscript.echo
wscript.echo "### Checking downloaded packages..."
split_tools = split(PROP_TOOLS, " ")
for each tool in split_tools
    execute "tool_file = TOOL_FILE_" & tool
    tool_path = temp_folder.path & "\" & tool_file
    if file_sys.fileexists(tool_path) then
        wscript.echo tool_path & ": Adding to the install list."
        install_list = install_list & " " & tool
    else
        wscript.echo tool_path & ": File not found."
    end if
next

' Install the proprietary CLI tools to the ESOS USB drive (if any)
dim letter, disk_num, vol_path_parts, part, access_part_cmd, mount_cmd, unmount_cmd, split_installs, pkg, sbin_dest_path, lib_dest_path
wscript.echo
if install_list = "" then
    wscript.echo "### Nothing to do."
    wscript.echo "### Your ESOS USB drive is complete, however, no RAID controller CLI tools were installed."
else
    wscript.echo "### Installing proprietary CLI tools..."
    ' Get an available drive letter to use
    letter = get_drive_letter(wmi_service)
    ' Figure out the disk drive number
    set disk_num = ""
    vol_path_parts = split(vol_path, "\", -1)
    for each part in vol_path_parts
        if instr(lcase(part), "harddisk") then
           disk_num = mid(part, 9, 1)
           exit for
        end if
    next
    if disk_num = "" then
        wscript.echo "ERROR: Couldn't retrieve hard disk number."
        wscript.echo
        exit_app
    end if
    ' Change the usable partition on the USB flash drive
    access_part_cmd = bootice_prog & " /device=" & disk_num & ":1 /partitions /assign_letter"
    exec_cmd wsh_shell, access_part_cmd
    ' Start the ext2fsd service 
    wsh_shell.run "net start ext2fsd", 1, false
    wsh_shell.run ext2mgr_prog & " -quiet", 1, false
    wscript.sleep 5000
    ' Mount root the file system
    mount_cmd = mount_prog & " " & disk_num & " 1 " & letter
    exec_cmd wsh_shell, mount_cmd
    ' Loop over selected pacakges and install them
    orig_cwd = wsh_shell.currentdirectory
    wsh_shell.currentdirectory = temp_folder.path
    split_installs = split(install_list, " ")
    for each pkg in split_installs
        ' Install for each tool
        sbin_dest_path = letter & "\opt\sbin\"
        lib_dest_path = letter & "\opt\lib\"
        if pkg = "MegaCLI" then
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y *_MegaCLI.zip"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y Linux\MegaCli-*.rpm"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y MegaCli-*.cpio"
            file_sys.copyfile "opt\MegaRAID\MegaCli\MegaCli64", sbin_dest_path
        elseif pkg = "StorCLI" then
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y *_StorCLI.zip"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y storcli_all_os\Linux\storcli-*.rpm"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y storcli-*.cpio"
            file_sys.copyfile "opt\MegaRAID\storcli\storcli64", sbin_dest_path
            file_sys.copyfile "opt\MegaRAID\storcli\libstorelibir*", lib_dest_path
        elseif pkg = "arcconf" then
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y arcconf_*.zip"
            file_sys.copyfile "linux_x64\arcconf", sbin_dest_path
        elseif pkg = "hpacucli" then
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y hpacucli-*.x86_64.rpm"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y hpacucli-*.cpio"
            file_sys.copyfile "opt\compaq\hpacucli\bld\.hpacucli", sbin_dest_path & "hpacucli"
        elseif pkg = "linuxcli" then
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y linuxcli_*.zip"
            dim sub_folder
            for each sub_folder in temp_folder.subfolders
                if instr(sub_folder.name, "linuxcli_") > 0 then
                    file_sys.copyfile sub_folder.name & "\x86_64\cli64", sbin_dest_path
                    exit for
                end if
            next
        elseif pkg = "3DM2_CLI" then
            wscript.echo "install 3dm2_cli"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y 3DM2_CLI-*.zip"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y tdmCliLnx.tgz"
            exec_cmd wsh_shell, sevenzip_prog & " x -bd -y tdmCliLnx.tar"
            file_sys.copyfile "tw_cli.x86_64", sbin_dest_path
        end if
    next
    wsh_shell.currentdirectory = orig_cwd
    ' Unmount it
    unmount_cmd = mount_prog & " /umount " & letter
    exec_cmd wsh_shell, unmount_cmd
    ' Restore (unhide) the first partition
    exec_cmd wsh_shell, access_part_cmd
    ' Success
    wscript.echo
    wscript.echo "### ESOS USB drive installation complete!"
    wscript.echo "### You may now remove and use your ESOS USB drive."
end if

' Done
file_sys.deletefolder temp_folder.path, true
wscript.echo
exit_app


sub exit_app
    wscript.stdout.write "Press the ENTER key to exit..."
    wscript.stdin.readline
    wscript.quit
end sub


function read_all(shell_exec)
    ' Check/read stdout
    if not shell_exec.stdout.atendofstream then
        read_all = shell_exec.stdout.readall
        exit function
    end if
    ' Check/read stderr
    if not shell_exec.stderr.atendofstream then
        read_all = shell_exec.stderr.readall
        exit function
    end if
    read_all = -1
end function


function exec_cmd(wsh_shell, cmd_string)
    dim all_input, try_count, shell_exec
    all_input = ""
    try_count = 0
    ' Execute the command
    set shell_exec = wsh_shell.exec(cmd_string)
    ' Loop until we have all command output (or timeout)
    do while true
        dim input
        input = read_all(shell_exec)
        if -1 = input then
            if try_count > 10 and shell_exec.status = 1 then
                exit do
            end if
            try_count = try_count + 1
            wscript.sleep 100
        else
            all_input = all_input & input
            try_count = 0
        end if
    loop
    ' Print all command output and quit if non-zero exit returned
    wscript.echo all_input
    if shell_exec.exitcode <> 0 then
        wscript.echo "ERROR: '" & cmd_string & "' returned a non-zero exit code!"
        wscript.echo
        exit_app
    end if
end function


function get_drive_letter(wmi_service)
    dim script_dict, disk_cols, disk_objs, i, letter_test
    ' Get list of drive letters
    set script_dict = createobject("scripting.dictionary")
    set disk_cols = wmi_service.execquery("select * from win32_logicaldisk")
    for each disk_objs in disk_cols
        script_dict.add disk_objs.deviceid, disk_objs.deviceid
    next
    ' Find available drive letter
    for i = 67 to 90
        letter_test = chr(i) & ":"
        if script_dict.exists(letter_test) then
        else
            get_drive_letter = letter_test
            exit function
        end if
    next
    wscript.echo "ERROR: There are no available drive letters on this computer!"
    wscript.echo
    exit_app
end function

