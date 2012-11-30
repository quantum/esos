/*
 * $Id$
 */

#ifndef _MAIN_H
#define	_MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif

#define SYSTEM_MENU 0
#define SYSTEM_SYNC_CONF 1
#define SYSTEM_NETWORK 2
#define SYSTEM_RESTART_NET 3
#define SYSTEM_MAIL 4
#define SYSTEM_TEST_EMAIL 5
#define SYSTEM_ADD_USER 6
#define SYSTEM_DEL_USER 7
#define SYSTEM_CHG_PASSWD 8
#define SYSTEM_SCST_INFO 9
#define SYSTEM_LINUXHA_INFO 10
#define SYSTEM_DATE_TIME 11

#define BACK_STORAGE_MENU 1
#define BACK_STORAGE_ADP_PROP 1
#define BACK_STORAGE_ADP_INFO 2
#define BACK_STORAGE_ADD_VOL 3
#define BACK_STORAGE_DEL_VOL 4
#define BACK_STORAGE_VOL_PROP 5
#define BACK_STORAGE_DRBD_STAT 6
#define BACK_STORAGE_SOFT_RAID_STAT 7
#define BACK_STORAGE_LVM2_INFO 8
#define BACK_STORAGE_CREATE_FS 9
#define BACK_STORAGE_REMOVE_FS 10
#define BACK_STORAGE_ADD_VDISK_FILE 11
#define BACK_STORAGE_DEL_VDISK_FILE 12

#define DEVICES_MENU 2
#define DEVICES_DEV_INFO 1
#define DEVICES_ADD_DEV 2
#define DEVICES_DEL_DEV 3
#define DEVICES_MAP_TO 4
#define DEVICES_UNMAP_FROM 5

#define TARGETS_MENU 3
#define TARGETS_TGT_INFO 1
#define TARGETS_ADD_ISCSI 2
#define TARGETS_REM_ISCSI 3
#define TARGETS_LIP 4
#define TARGETS_TOGGLE 5

#define HOSTS_MENU 4
#define HOSTS_ADD_GROUP 1
#define HOSTS_REM_GROUP 2
#define HOSTS_ADD_INIT 3
#define HOSTS_REM_INIT 4

#define INTERFACE_MENU 5
#define INTERFACE_QUIT 1
#define INTERFACE_SHELL 2

#define MAX_LABEL_LENGTH 50

#define REFRESH_DELAY 20

#define MIN_SCR_X 80
#define MIN_SCR_Y 24

#define COLOR_MAIN_TEXT COLOR_PAIR(5)
#define COLOR_MAIN_BOX COLOR_PAIR(53)
#define COLOR_DIALOG_TEXT COLOR_PAIR(7)
#define COLOR_DIALOG_BOX COLOR_PAIR(7)
#define COLOR_DIALOG_SELECT COLOR_PAIR(29)|A_BOLD
#define COLOR_DIALOG_INPUT COLOR_PAIR(5)
#define COLOR_ERROR_TEXT COLOR_PAIR(2)
#define COLOR_ERROR_BOX COLOR_PAIR(2)
#define COLOR_ERROR_SELECT COLOR_PAIR(57)
#define COLOR_MENU_TEXT COLOR_PAIR(31)
#define COLOR_STATUS_BAR COLOR_PAIR(7)

#define ADAPTERS_LABEL_ROWS 8
#define ADAPTERS_LABEL_COLS 76
#define DEVICES_LABEL_ROWS 9
#define DEVICES_LABEL_COLS 37
#define TARGETS_LABEL_ROWS 9
#define TARGETS_LABEL_COLS 37

#define ESOS_VER_FILE "/etc/esos-release"
#define ESOS_VER_MAX 50
#define USERNAME_MAX 20

#define CLEAR_CMD "clear"
#define SHELL "/bin/bash"

#define SYSFS_FC_HOST "/sys/class/fc_host"
#define SYSFS_INFINIBAND "/sys/class/infiniband"
#define SYSFS_SCST_TGT "/sys/kernel/scst_tgt"
#define SYSFS_SCSI_DISK "/sys/class/scsi_disk"
#define SYSFS_BLOCK "/sys/block"

#define MAX_SYSFS_ATTR_SIZE 256
#define MAX_SYSFS_PATH_SIZE 256
    
#define ESOS_GROUP "root"
#define MAX_USERS 32
#define MAX_UNAME_LEN 20
#define MAX_PASSWD_LEN 64

void termSize(WINDOW *screen);
void screenResize(CDKSCREEN *cdk, WINDOW *main_screen, int orig_term_x, int orig_term_y);
char *strStrip(char *string);
void statusBar(WINDOW *window);
void readAttribute(char sysfs_attr[], char attr_value[]);
int writeAttribute(char sysfs_attr[], char attr_value[]);

#ifdef	__cplusplus
}
#endif

#endif	/* _MAIN_H */

