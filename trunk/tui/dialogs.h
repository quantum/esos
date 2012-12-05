/*
 * $Id$
 */

#ifndef _DIALOGS_H
#define	_DIALOGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* Color settings */
#define COLOR_MAIN_TEXT         COLOR_PAIR(5)
#define COLOR_MAIN_BOX          COLOR_PAIR(53)
#define COLOR_DIALOG_TEXT       COLOR_PAIR(7)
#define COLOR_DIALOG_BOX        COLOR_PAIR(7)
#define COLOR_DIALOG_SELECT     COLOR_PAIR(29)|A_BOLD
#define COLOR_DIALOG_INPUT      COLOR_PAIR(5)
#define COLOR_ERROR_TEXT        COLOR_PAIR(2)
#define COLOR_ERROR_BOX         COLOR_PAIR(2)
#define COLOR_ERROR_SELECT      COLOR_PAIR(57)
#define COLOR_MENU_TEXT         COLOR_PAIR(31)
#define COLOR_STATUS_BAR        COLOR_PAIR(7)

/* Canned messages */
#define CONTINUE_MSG    "<C></B><Press ENTER to continue...>"

/* Scrolling window sizing */
#define ADP_INFO_ROWS                   16
#define ADP_INFO_COLS                   72
#define MAX_ADP_INFO_LINES              256
#define DRBD_INFO_ROWS                  10
#define DRBD_INFO_COLS                  58
#define MAX_DRBD_INFO_LINES             256
#define MDSTAT_INFO_ROWS                10
#define MDSTAT_INFO_COLS                58
#define MAX_MDSTAT_INFO_LINES           256
#define LVM2_INFO_ROWS                  10
#define LVM2_INFO_COLS                  58
#define MAX_LVM2_INFO_LINES             512
#define NET_RESTART_INFO_ROWS           15
#define NET_RESTART_INFO_COLS           60
#define MAX_NET_RESTART_INFO_LINES      128
#define SCST_INFO_ROWS                  10
#define SCST_INFO_COLS                  60
#define MAX_SCST_INFO_LINES             128
#define DEV_INFO_ROWS                   18
#define DEV_INFO_COLS                   73
#define MAX_DEV_INFO_LINES              64
#define MAKE_FS_INFO_ROWS               8
#define MAKE_FS_INFO_COLS               58
#define MAX_MAKE_FS_INFO_LINES          64
#define TGT_INFO_ROWS                   10
#define TGT_INFO_COLS                   58
#define MAX_TGT_INFO_LINES              64
#define LIP_INFO_ROWS                   6
#define LIP_INFO_COLS                   44
#define MAX_LIP_INFO_LINES              32

/* Other window/screen/label/etc. sizing limits */
#define MAX_FS_DIALOG_INFO_LINES        11
    
/* Field/entry/name/value sizing limits */
#define MAX_EMAIL_LEN           32
#define MAX_SMTP_LEN            32
#define MAX_SMTP_USER_LEN       32
#define MAX_SMTP_PASS_LEN       32
#define MAX_INI_VAL             64
#define MAX_NET_IFACE           32
#define MAX_HOSTNAME            32
#define MAX_DOMAINNAME          32
#define MAX_IPV4_ADDR_LEN       15
#define MAX_NTP_LEN             32
#define MAX_FS_LABEL            12
#define MAX_VDISK_PATH_LEN      256
#define MAX_VDISK_NAME          32

/* Misc. limits */
#define MAX_FILE_SYSTEMS        128
#define MAX_FS_ATTR_LEN         64
#define MAX_MNT_LINE_BUFFER     4096
#define MAX_BLOCK_DEVS          64
#define MAX_ZONEINFO_PATH       64
#define MAX_TZ_FILES            1024
//#define MAX_PROC_LINE           80

#ifdef	__cplusplus
}
#endif

#endif	/* _DIALOGS_H */

