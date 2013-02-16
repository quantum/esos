/*
 * $Id$
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
#define ESOS_SUPERUSER  "root"
#define ESOS_GROUP      "root"
#define MAX_USERS       32
#define MAX_UNAME_LEN   20
#define MAX_PASSWD_LEN  64
#define ESOS_BOOT_PART  "esos_boot"
#define DEFAULT_IF_MTU  "1500"

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
#define SYSTEM_DATE_TIME        11

/* Back-End Storage menu layout */
#define BACK_STORAGE_MENU               1
#define BACK_STORAGE_ADP_PROP           1
#define BACK_STORAGE_ADP_INFO           2
#define BACK_STORAGE_ADD_VOL            3
#define BACK_STORAGE_DEL_VOL            4
#define BACK_STORAGE_VOL_PROP           5
#define BACK_STORAGE_DRBD_STAT          6
#define BACK_STORAGE_SOFT_RAID_STAT     7
#define BACK_STORAGE_LVM2_INFO          8
#define BACK_STORAGE_CREATE_FS          9
#define BACK_STORAGE_REMOVE_FS          10
#define BACK_STORAGE_ADD_VDISK_FILE     11
#define BACK_STORAGE_DEL_VDISK_FILE     12

/* Hosts menu layout */
#define HOSTS_MENU      2
#define HOSTS_ADD_GROUP 1
#define HOSTS_REM_GROUP 2
#define HOSTS_ADD_INIT  3
#define HOSTS_REM_INIT  4

/* Devices menu layout */
#define DEVICES_MENU            3
#define DEVICES_LUN_LAYOUT      1
#define DEVICES_DEV_INFO        2
#define DEVICES_ADD_DEV         3
#define DEVICES_DEL_DEV         4
#define DEVICES_MAP_TO          5
#define DEVICES_UNMAP_FROM      6

/* Targets menu layout */
#define TARGETS_MENU            4
#define TARGETS_TGT_INFO        1
#define TARGETS_ADD_ISCSI       2
#define TARGETS_REM_ISCSI       3
#define TARGETS_LIP             4
#define TARGETS_TOGGLE          5

/* Interface menu layout */
#define INTERFACE_MENU          5
#define INTERFACE_QUIT          1
#define INTERFACE_SHELL         2
#define INTERFACE_HELP          3
#define INTERFACE_SUPPORT_PKG   4
#define INTERFACE_ABOUT         5

/* Misc. limits */
#define GIBIBYTE_SIZE           1073741824LL
#define MEBIBYTE_SIZE           1048576LL
#define VDISK_WRITE_SIZE        32768
#define MAX_SHELL_CMD_LEN       256
#define MISC_STRING_LEN         128

/* Main screen information labels */
#define ADAPTERS_LABEL_ROWS     8
#define ADAPTERS_LABEL_COLS     76
#define DEVICES_LABEL_ROWS      9
#define DEVICES_LABEL_COLS      37
#define TARGETS_LABEL_ROWS      9
#define TARGETS_LABEL_COLS      37

/* System tools and utilities (binaries and scripts) */
#define CLEAR_BIN       "clear"
#define SHELL_BIN       "/bin/bash"
#define MOUNT_BIN       "/bin/mount"
#define UMOUNT_BIN      "/bin/umount"
#define LVDISPLAY_BIN   "/usr/sbin/lvdisplay"
#define SG_VPD_BIN      "/usr/bin/sg_vpd"
#define SCSTADMIN_TOOL  "/usr/sbin/scstadmin"
#define SYNC_CONF_TOOL  "/usr/local/sbin/conf_sync.sh"
#define MEGACLI_BIN     "/opt/sbin/MegaCli64"
#define CHPASSWD_BIN    "/usr/sbin/chpasswd"
#define ADDUSER_BIN     "/bin/adduser"
#define DELUSER_BIN     "/bin/deluser"
#define DELGROUP_BIN    "/bin/delgroup"
#define RC_NETWORK      "/etc/rc.d/rc.network"
#define SSMTP_BIN       "/usr/sbin/ssmtp"
#define TAR_BIN         "/bin/tar"
#define CRM_TOOL        "/usr/sbin/crm"

/* A few sysfs settings */
#define SYSFS_FC_HOST           "/sys/class/fc_host"
#define SYSFS_INFINIBAND        "/sys/class/infiniband"
#define SYSFS_SCST_TGT          "/sys/kernel/scst_tgt"
#define SYSFS_SCSI_DISK         "/sys/class/scsi_disk"
#define SYSFS_SCSI_DEVICE       "/sys/class/scsi_device"
#define SYSFS_BLOCK             "/sys/block"
#define MAX_SYSFS_ATTR_SIZE     256
#define MAX_SYSFS_PATH_SIZE     256
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
#define ESOS_VER_FILE   "/etc/esos-release"
#define BASHRC_FILE     "/etc/bashrc"
#define QLA_FW_LICENSE  "/lib/firmware/QLOGIC_FW_LICENSE"

/* Misc. path settings */
#define VDISK_MNT_BASE  "/mnt/vdisks"
#define LOCALTIME       "/etc/localtime"
#define ZONEINFO        "/usr/share/zoneinfo/posix"

/* Size/limits settings */
#define MAX_SCST_TGTS           256
#define MAX_SCST_GROUPS         128
#define MAX_SCST_LUNS           256
#define MAX_SCSI_DISKS          128
#define MAX_SCST_DEVS           128
#define MAX_SCST_INITS          128
#define MAX_SCST_DRIVERS        16
#define SCST_ISCSI_TGT_LEN      32
#define SCST_DEV_NAME_LEN       16
#define SCST_GRP_NAME_LEN       16
#define SCST_INITIATOR_LEN      64
#define MAX_SCSI_DEVICES        256

#ifdef	__cplusplus
}
#endif

#endif	/* _SYSTEM_H */

