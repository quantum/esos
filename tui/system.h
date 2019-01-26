/**
 * @file system.h
 * @brief System settings and configuration options for the program.
 * @author Copyright (c) 2019 Quantum Corporation
 */

#ifndef _SYSTEM_H
#define	_SYSTEM_H

#ifdef	__cplusplus
extern "C" {
#endif

/* User interface (text/curses) settings */
#define REFRESH_DELAY           20
#define MIN_SCR_X               80
#define MIN_SCR_Y               24
#define MAX_LABEL_LENGTH        50
#define STAT_BAR_ESOS_VER_MAX   50
#define STAT_BAR_UNAME_MAX      20
#define CDK_MENU_MAX_SIZE       32

/* Misc. ESOS system-wide settings */
#define ESOS_SUPERUSER          "root"
#define ESOS_GROUP              "root"
#define MAX_USERS               32
#define MAX_UNAME_LEN           20
#define MAX_PASSWD_LEN          64
#define ESOS_ROOT_PART          "esos_root"
#define DEFAULT_ETH_MTU         "1500"
#define DEFAULT_IB_MTU          "65520"
#define TEMP_DIR                "/tmp"
#define SYS_FILE_SYSTEMS        "/proc /sys /dev/pts /dev/shm /boot " \
        "/mnt/root /mnt/conf /mnt/logs /tmp"
#define USAGE_POST_URL          "http://usage.esos-project.com/post_info.php"
#define INET_TEST_HOST          "usage.esos-project.com"

/* System menu layout */
#define SYSTEM_MENU             0
#define SYSTEM_SYNC_CONF        1
#define SYSTEM_NETWORK          2
#define SYSTEM_RESTART_NET      3
#define SYSTEM_MAIL             4
#define SYSTEM_TEST_EMAIL       5
#define SYSTEM_ADD_USER         6
#define SYSTEM_DEL_USER         7
#define SYSTEM_CHG_PASSWD       8
#define SYSTEM_SCST_INFO        9
#define SYSTEM_CRM_STATUS       10
#define SYSTEM_DRBD_STATUS      11
#define SYSTEM_DATE_TIME        12

/* Hardware RAID menu layout */
#define HW_RAID_MENU            1
#define HW_RAID_ADD_VOL         1
#define HW_RAID_REM_VOL         2
#define HW_RAID_ADD_HSP         3
#define HW_RAID_REM_HSP         4

/* Software RAID menu layout */
#define SW_RAID_MENU            2
#define SW_RAID_MD_STAT         1
#define SW_RAID_ADD_ARRAY       2
#define SW_RAID_REM_ARRAY       3
#define SW_RAID_FAULT_DEV       4
#define SW_RAID_ADD_DEV         5
#define SW_RAID_REM_DEV         6

/* Logical Volume Management (LVM) menu layout */
#define LVM_MENU                3
#define LVM_LV_LIST             1
#define LVM_ADD_PV              2
#define LVM_REM_PV              3
#define LVM_ADD_VG              4
#define LVM_REM_VG              5
#define LVM_ADD_LV              6
#define LVM_REM_LV              7

/* File System menu layout */
#define FILE_SYS_MENU           4
#define FILE_SYS_VDISK_LIST     1
#define FILE_SYS_ADD_FS         2
#define FILE_SYS_REM_FS         3
#define FILE_SYS_ADD_VDISK      4
#define FILE_SYS_REM_VDISK      5

/* Hosts menu layout */
#define HOSTS_MENU              0
#define HOSTS_ADD_GROUP         1
#define HOSTS_REM_GROUP         2
#define HOSTS_ADD_INIT          3
#define HOSTS_REM_INIT          4

/* Devices menu layout */
#define DEVICES_MENU            1
#define DEVICES_LUN_LAYOUT      1
#define DEVICES_DEV_INFO        2
#define DEVICES_ADD_DEV         3
#define DEVICES_REM_DEV         4
#define DEVICES_MAP_TO          5
#define DEVICES_UNMAP_FROM      6

/* Targets menu layout */
#define TARGETS_MENU            2
#define TARGETS_TGT_INFO        1
#define TARGETS_ADD_ISCSI       2
#define TARGETS_REM_ISCSI       3
#define TARGETS_LIP             4
#define TARGETS_TOGGLE          5
#define TARGETS_SET_REL_TGT_ID  6

/* ALUA menu layout */
#define ALUA_MENU               3
#define ALUA_DEV_GRP_LAYOUT     1
#define ALUA_ADD_DEV_GRP        2
#define ALUA_REM_DEV_GRP        3
#define ALUA_ADD_TGT_GRP        4
#define ALUA_REM_TGT_GRP        5
#define ALUA_ADD_DEV_TO_GRP     6
#define ALUA_REM_DEV_FROM_GRP   7
#define ALUA_ADD_TGT_TO_GRP     8
#define ALUA_REM_TGT_FROM_GRP   9

/* Interface menu layout */
#define INTERFACE_MENU          4
#define INTERFACE_QUIT          1
#define INTERFACE_SHELL         2
#define INTERFACE_THEME         3
#define INTERFACE_HELP          4
#define INTERFACE_SUPPORT_PKG   5
#define INTERFACE_ABOUT         6

/* Misc. limits */
#define GIBIBYTE_SIZE           1073741824LL
#define MEBIBYTE_SIZE           1048576LL
#define VDISK_WRITE_SIZE        262144
#define MAX_SHELL_CMD_LEN       256
#define MISC_STRING_LEN         1024
#define UUID_STR_SIZE           64
#define MAX_TUI_STR_LEN         256
#define MAX_CMD_LINE_LEN        128

/* Main screen information labels */
#define MAX_INFO_LABEL_ROWS     512
#define TARGETS_LABEL_COLS      76
#define SESSIONS_LABEL_COLS     76
#define TARGETS_LABEL_TITLE     "FC HBAs / IB HCAs / FCoE Adapters"
#define SESSIONS_LABEL_TITLE    "Active Sessions"

/* System tools and utilities (binaries and scripts) */
#define CLEAR_BIN       "/usr/bin/clear"
#define SHELL_BIN       "/bin/bash"
#define MOUNT_BIN       "/bin/mount"
#define UMOUNT_BIN      "/bin/umount"
#define LVDISPLAY_BIN   "/usr/sbin/lvdisplay"
#define UDEVADM_BIN     "/usr/sbin/udevadm"
#define SCSTADMIN_TOOL  "/usr/sbin/scstadmin"
#define SYNC_CONF_TOOL  "/usr/local/sbin/usb_sync.sh"
#define MEGACLI_BIN     "/opt/sbin/MegaCli64"
#define CHPASSWD_BIN    "/usr/sbin/chpasswd"
#define ADDUSER_BIN     "/usr/sbin/adduser"
#define DELUSER_BIN     "/usr/sbin/deluser"
#define DELGROUP_BIN    "/usr/sbin/delgroup"
#define RC_NETWORK      "/etc/rc.d/rc.network"
#define SSMTP_BIN       "/usr/sbin/ssmtp"
#define TAR_BIN         "/usr/bin/tar"
#define CRM_TOOL        "/usr/sbin/crm"
#define SUPPORT_TOOL    "/usr/local/sbin/support_pkg.sh"
#define HWRAID_CLI_TOOL "/usr/local/sbin/hw_raid_cli.py"
#define MDADM_BIN       "/sbin/mdadm"
#define PVS_BIN         "/usr/sbin/pvs"
#define VGS_BIN         "/usr/sbin/vgs"
#define LVS_BIN         "/usr/sbin/lvs"
#define PVCREATE_BIN    "/usr/sbin/pvcreate"
#define PVREMOVE_BIN    "/usr/sbin/pvremove"
#define VGCREATE_BIN    "/usr/sbin/vgcreate"
#define VGREMOVE_BIN    "/usr/sbin/vgremove"
#define LVCREATE_BIN    "/usr/sbin/lvcreate"
#define LVREMOVE_BIN    "/usr/sbin/lvremove"
#define RPC_AGENT_BIN   "/opt/sbin/rpc_agent"

/* A few sysfs settings */
#define SYSFS_FC_HOST           "/sys/class/fc_host"
#define SYSFS_INFINIBAND        "/sys/class/infiniband"
#define SYSFS_SCST_TGT          "/sys/kernel/scst_tgt"
#define SYSFS_SCSI_DISK         "/sys/class/scsi_disk"
#define SYSFS_SCSI_DEVICE       "/sys/class/scsi_device"
#define SYSFS_BLOCK             "/sys/block"
#define SYSFS_NET               "/sys/class/net"
#define MAX_SYSFS_ATTR_SIZE     256
#define MAX_SYSFS_PATH_SIZE     2048
#define SCSI_CHANGER_TYPE       8
#define SCSI_TAPE_TYPE          1

/* System files (configuration, etc.) */
#define PROC_DRBD       "/proc/drbd"
#define PROC_MDSTAT     "/proc/mdstat"
#define SSMTP_CONF      "/etc/ssmtp/ssmtp.conf"
#define NETWORK_CONF    "/etc/network.conf"
#define NTP_SERVER      "/etc/ntp_server"
#define SCST_CONF       "/etc/scst.conf"
#define FSTAB           "/etc/fstab"
#define FSTAB_TMP       "/etc/fstab.new"
#define MTAB            "/proc/mounts"
#define ESOS_LICENSE    "/usr/share/doc/esos/LICENSE"
#define GLOBAL_BASHRC   "/etc/bashrc"
#define ESOS_CONF       "/etc/esos.conf"

/* Misc. path settings */
#define VDISK_MNT_BASE  "/mnt/vdisks"
#define LOCALTIME       "/etc/localtime"
#define ZONEINFO        "/usr/share/zoneinfo/posix"

/* Size/limits settings */
#define MAX_SCST_TGTS               256
#define MAX_SCST_GROUPS             128
#define MAX_SCST_LUNS               256
#define MAX_SCSI_DISKS              128
#define MAX_SCST_DEVS               128
#define MAX_SCST_INITS              128
#define MAX_SCST_DRIVERS            16
#define MAX_SCST_SESSNS             512
#define MAX_SCST_SESS_INITS         128
#define MAX_SCST_DEV_GRPS           64
#define MAX_SCST_TGT_GRPS           64
#define MAX_SCST_DEV_GRP_DEVS       64
#define MAX_SCST_TGT_GRP_TGTS       64
#define MAX_FC_ADAPTERS             64
#define MAX_IB_ADAPTERS             64
#define SCST_ISCSI_TGT_LEN          32
#define MAX_SCST_ISCSI_TGT_LEN      SCST_ISCSI_TGT_LEN + 1
#define SCST_DEV_NAME_LEN           16
#define MAX_SCST_DEV_NAME_LEN       SCST_DEV_NAME_LEN + 1
#define SCST_SEC_GRP_NAME_LEN       16
#define MAX_SCST_SEC_GRP_NAME_LEN   SCST_SEC_GRP_NAME_LEN + 1
#define SCST_INITIATOR_LEN          128
#define MAX_SCST_INITIATOR_LEN      SCST_INITIATOR_LEN + 1
#define SCST_DEV_GRP_NAME_LEN       16
#define MAX_SCST_DEV_GRP_NAME_LEN   SCST_DEV_GRP_NAME_LEN + 1
#define SCST_TGT_GRP_NAME_LEN       16
#define MAX_SCST_TGT_GRP_NAME_LEN   SCST_TGT_GRP_NAME_LEN + 1
#define SCST_TGT_NAME_LEN           64
#define MAX_SCST_TGT_NAME_LEN       SCST_TGT_NAME_LEN + 1
#define LVM_VOL_GRP_NAME_LEN        16
#define MAX_LVM_VOL_GRP_NAME_LEN    LVM_VOL_GRP_NAME_LEN + 1
#define MAX_SCSI_DEVICES            256
#define MIN_SCST_REL_TGT_ID         0
#define MAX_SCST_REL_TGT_ID         65535
#define MIN_SCST_TGT_GRP_ID         0
#define MAX_SCST_TGT_GRP_ID         65535
#define MIN_SCST_LUN_VAL            0
#define MAX_SCST_LUN_VAL            255
#define MAX_SIMPLE_MENU_OPTS        16

/* Logger settings */
#define TUI_LOG_PREFIX      "esos_tui"
#define TUI_LOG_OPTIONS     LOG_PID
#define TUI_LOG_FACILITY    LOG_LOCAL4
#define TUI_LOG_PRIORITY    LOG_ERR

/* Other useful macros */
#define FREE_NULL(p) if ((p) != 0) { free (p); p = 0; }
#define DEBUG_LOG(...) syslog(TUI_LOG_PRIORITY, __VA_ARGS__)
#define SAFE_ASPRINTF(...) assert(asprintf(__VA_ARGS__) != -1)

#ifdef	__cplusplus
}
#endif

#endif	/* _SYSTEM_H */
