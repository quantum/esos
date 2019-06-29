/**
 * @file dialogs.h
 * @brief Settings related to the TUI dialogs.
 * @author Copyright (c) 2019 Quantum Corporation
 */

#ifndef _DIALOGS_H
#define	_DIALOGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * CDK initCDKColor() Map: PAIR,FOREGROUND,BACKGROUND
 *
 * 1,WHITE,WHITE    9,RED,WHITE         17,GREEN,WHITE      25,YELLOW,WHITE
 * 2,WHITE,RED      10,RED,RED          18,GREEN,RED        26,YELLOW,RED
 * 3,WHITE,GREEN    11,RED,GREEN        19,GREEN,GREEN      27,YELLOW,GREEN
 * 4,WHITE,YELLOW   12,RED,YELLOW       20,GREEN,YELLOW     28,YELLOW,YELLOW
 * 5,WHITE,BLUE     13,RED,BLUE         21,GREEN,BLUE       29,YELLOW,BLUE
 * 6,WHITE,MAGENTA  14,RED,MAGENTA      22,GREEN,MAGENTA    30,YELLOW,MAGENTA
 * 7,WHITE,CYAN     15,RED,CYAN         23,GREEN,CYAN       31,YELLOW,CYAN
 * 8,WHITE,BLACK    16,RED,BLACK        24,GREEN,BLACK      32,YELLOW,BLACK
 *
 * 33,BLUE,WHITE    41,MAGENTA,WHITE    49,CYAN,WHITE       57,BLACK,WHITE
 * 34,BLUE,RED      42,MAGENTA,RED      50,CYAN,RED         58,BLACK,RED
 * 35,BLUE,GREEN    43,MAGENTA,GREEN    51,CYAN,GREEN       59,BLACK,GREEN
 * 36,BLUE,YELLOW   44,MAGENTA,YELLOW   52,CYAN,YELLOW      60,BLACK,YELLOW
 * 37,BLUE,BLUE     45,MAGENTA,BLUE     53,CYAN,BLUE        61,BLACK,BLUE
 * 38,BLUE,MAGENTA  46,MAGENTA,MAGENTA  54,CYAN,MAGENTA     62,BLACK,MAGENTA
 * 39,BLUE,CYAN     47,MAGENTA,CYAN     55,CYAN,CYAN        63,BLACK,CYAN
 * 40,BLUE,BLACK    48,MAGENTA,BLACK    56,CYAN,BLACK       64,BLACK,BLACK
 */

/* Our color/theme settings and variables */
#define MAX_TUI_THEMES  10
typedef enum {BLUE_TUI, BLACK_TUI} ThemeNum;
extern ThemeNum g_curr_theme;
extern chtype g_color_main_text[MAX_TUI_THEMES];
extern chtype g_color_main_box[MAX_TUI_THEMES];
extern chtype g_color_dialog_text[MAX_TUI_THEMES];
extern chtype g_color_dialog_box[MAX_TUI_THEMES];
extern chtype g_color_dialog_select[MAX_TUI_THEMES];
extern chtype g_color_dialog_input[MAX_TUI_THEMES];
extern chtype g_color_error_text[MAX_TUI_THEMES];
extern chtype g_color_error_box[MAX_TUI_THEMES];
extern chtype g_color_error_select[MAX_TUI_THEMES];
extern chtype g_color_menu_text[MAX_TUI_THEMES];
extern chtype g_color_status_bar[MAX_TUI_THEMES];
extern chtype g_color_mentry_box[MAX_TUI_THEMES];
extern int g_color_menu_letter[MAX_TUI_THEMES];
extern char *g_color_menu_bg[MAX_TUI_THEMES];
extern int g_color_info_header[MAX_TUI_THEMES];
extern int g_color_dialog_title[MAX_TUI_THEMES];

/* Scrolling window sizing */
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
#define ESOS_LICENSE_ROWS               10
#define ESOS_LICENSE_COLS               74
#define MAX_ESOS_LICENSE_LINES          768

/* Other window/screen/label/etc. sizing limits */
#define MAX_FS_DIALOG_INFO_LINES        11
#define ADD_DEV_INFO_LINES              3
#define NEW_LD_INFO_LINES               3
#define NEW_ARRAY_INFO_LINES            3
#define ADD_VDISK_INFO_LINES            4
#define NEW_LV_INFO_LINES               4
#define IP_ADDR_INFO_LINES              6
#define CONFIRM_DIAG_MSG_SIZE           6
#define ERROR_DIAG_MSG_SIZE             6
#define INFORM_DIAG_MSG_SIZE            6
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
#define ABOUT_MSG_SIZE                  14

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
#define MAX_VDISK_NAME_LEN      32
#define MAX_ARRAY_NAME_LEN      32
#define MAX_LV_NAME_LEN         32

/* Misc. limits */
#define MAX_FILE_SYSTEMS                128
#define MAX_FS_ATTR_LEN                 64
#define MAX_MNT_LINE_BUFFER             4096
#define MAX_BLOCK_DEVS                  64
#define MAX_ZONEINFO_PATH               2048
#define MAX_TZ_FILES                    1024
#define MAX_PD_INFO_LINE_BUFF           4096
#define MAX_DEV_INFO_LINE_BUFF          4096
#define MAX_SLAVES_LIST_BUFF            512
#define MAX_BR_MEMBERS_LIST_BUFF        512
#define MAX_BOND_OPTS_BUFF              512
#define MAX_ETHTOOL_OPTS_BUFF           512
#define MAX_HWRAID_CTRLRS               64
#define MAX_HWRAID_PDRVS                256
#define MAX_HWRAID_LDRVS                128
#define MAX_MD_ARRAYS                   64
#define MAX_MD_MEMBERS                  128
#define MAX_LVM_PVS                     256
#define MAX_LVM_VGS                     128
#define MAX_LVM_LVS                     512

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
