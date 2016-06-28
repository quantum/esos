/**
 * @file dialogs.h
 * @brief Settings related to the TUI dialogs.
 * @author Copyright (c) 2012-2016 Marc A. Smith
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

/* Scrolling window sizing */
#define ADP_INFO_ROWS                   16
#define ADP_INFO_COLS                   72
#define MAX_ADP_INFO_LINES              256
#define DRBD_INFO_ROWS                  10
#define DRBD_INFO_COLS                  70
#define MAX_DRBD_INFO_LINES             256
#define MDSTAT_INFO_ROWS                10
#define MDSTAT_INFO_COLS                70
#define MAX_MDSTAT_INFO_LINES           256
#define LVM2_INFO_ROWS                  14
#define LVM2_INFO_COLS                  70
#define MAX_LVM2_INFO_LINES             512
#define NET_RESTART_INFO_ROWS           15
#define NET_RESTART_INFO_COLS           60
#define MAX_NET_RESTART_INFO_LINES      128
#define SCST_INFO_ROWS                  10
#define SCST_INFO_COLS                  60
#define MAX_SCST_INFO_LINES             128
#define DEV_INFO_ROWS                   16
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
#define LUN_LAYOUT_ROWS                 12
#define LUN_LAYOUT_COLS                 62
#define MAX_LUN_LAYOUT_LINES            128
#define CRM_INFO_ROWS                   12
#define CRM_INFO_COLS                   68
#define MAX_CRM_INFO_LINES              512
#define VDLIST_INFO_ROWS                10
#define VDLIST_INFO_COLS                70
#define MAX_VDLIST_INFO_LINES           512
#define ALUA_LAYOUT_ROWS                12
#define ALUA_LAYOUT_COLS                72
#define MAX_ALUA_LAYOUT_LINES           128

/* Other window/screen/label/etc. sizing limits */
#define MAX_FS_DIALOG_INFO_LINES        11
#define ADD_DEV_INFO_LINES              3
#define LD_PROPS_INFO_LINES             9
#define NEW_LD_INFO_LINES               3
#define ADP_PROP_INFO_LINES             10
#define ADD_VDISK_INFO_LINES            4
#define CONFIRM_DIAG_MSG_SIZE           6
#define ERROR_DIAG_MSG_SIZE             6
#define QUEST_DIAG_MSG_SIZE             6
#define MAP_DEV_INFO_LINES              8
#define CHG_PASSWD_INFO_LINES           1
#define TGT_ON_OFF_INFO_LINES           5
#define TGT_GRP_INFO_LINES              3
#define ADD_TGT_INFO_LINES              4
#define MAX_NET_INFO_LINES              10
#define NET_SHORT_INFO_LINES            1
#define HELP_MSG_SIZE                   17
#define SUPPORT_PKG_MSG_SIZE            7
#define ABOUT_MSG_SIZE                  12

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
#define MAX_FILE_SYSTEMS                128
#define MAX_FS_ATTR_LEN                 64
#define MAX_MNT_LINE_BUFFER             4096
#define MAX_BLOCK_DEVS                  64
#define MAX_ZONEINFO_PATH               64
#define MAX_TZ_FILES                    1024
#define MAX_PD_INFO_LINE_BUFF           4096
#define MAX_SLAVES_LIST_BUFF            512
#define MAX_BR_MEMBERS_LIST_BUFF        512
#define MAX_BOND_OPTS_BUFF              512
#define MAX_ETHTOOL_OPTS_BUFF           512

/* Character validation */
typedef enum {
    ASCII_CHARS, NAME_CHARS, IPADDR_CHARS, EMAIL_CHARS, INIT_CHARS
} valid_input_t;
#define VALID_ASCII_CHAR(c) ((isascii(c)) ? 1 : 0)
#define VALID_ASCII_CHAR_MSG "Only ASCII characters are valid."
#define VALID_NAME_CHAR(c) ((isascii(c) && (isalnum(c) || c == '_' || \
        c == '-' || c == '.')) ? 1 : 0)
#define VALID_NAME_CHAR_MSG \
        "Only these characters are valid: a-z A-Z 0-9 _ - ."
#define VALID_IPADDR_CHAR(c) ((isascii(c) && (isdigit(c) || \
        c == '.')) ? 1 : 0)
#define VALID_IPADDR_CHAR_MSG \
        "Only these characters are valid: 0-9 ."
#define VALID_EMAIL_CHAR(c) ((isascii(c) && (isalnum(c) || c == '_' || \
        c == '-' || c == '.' || c == '@')) ? 1 : 0)
#define VALID_EMAIL_CHAR_MSG \
        "Only these characters are valid: a-z A-Z 0-9 _ - . @"
#define VALID_INIT_CHAR(c) ((isascii(c) && (isalnum(c) || c == '_' || \
        c == '-' || c == '.' || c == ':' || c == '?' || c == '*')) ? 1 : 0)
#define VALID_INIT_CHAR_MSG \
        "Only these characters are valid: a-z A-Z 0-9 _ - . : ? *"

/* Linux NIC bonding mode */
typedef enum {
    NO_BONDING, MASTER, SLAVE
} bonding_t;

/* This would normally be set via the ESOS build */
#ifndef BUILD_OPTS
#define BUILD_OPTS "N/A"
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _DIALOGS_H */
