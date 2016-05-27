/**
 * @file menu_common.c
 * @brief Contains the common menu actions used by most menus.
 * @author Copyright (c) 2012-2016 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <cdk/cdkscreen.h>
#include <mntent.h>
#include <blkid/blkid.h>
#include <fcntl.h>
#include <syslog.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/*
 * Generic error dialog; takes an error message and displays a nice red
 * "ERROR" box to the user. Accepts multiple lines; caller should pass NULL
 * for lines that shouldn't be set.
 */
void errorDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *error = 0;
    char *message[ERROR_DIAG_MSG_SIZE] = {NULL};
    int i = 0;

    /* Set the message */
    SAFE_ASPRINTF(&message[0], "<C></B>ERROR");
    SAFE_ASPRINTF(&message[1], " ");
    if (msg_line_1)
        SAFE_ASPRINTF(&message[2], "<C>%s", msg_line_1);
    else
        SAFE_ASPRINTF(&message[2], " ");
    if (msg_line_2)
         SAFE_ASPRINTF(&message[3], "<C>%s", msg_line_2);
    else
        SAFE_ASPRINTF(&message[3], " ");
    SAFE_ASPRINTF(&message[4], " ");
    SAFE_ASPRINTF(&message[5], " ");

    /* Display the error dialog box */
    error = newCDKDialog(screen, CENTER, CENTER, message, ERROR_DIAG_MSG_SIZE,
            g_ok_msg, 1, COLOR_ERROR_SELECT, TRUE, TRUE, FALSE);
    if (error) {
        setCDKDialogBackgroundAttrib(error, COLOR_ERROR_TEXT);
        setCDKDialogBoxAttribute(error, COLOR_ERROR_BOX);
        activateCDKDialog(error, 0);
        destroyCDKDialog(error);
    }
    for (i = 0; i < ERROR_DIAG_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(screen);
    return;
}


/*
 * Two callback functions for use with CDK buttons (and traverse).
 */
void okButtonCB(CDKBUTTON *button) {
    exitOKCDKScreenOf(&button->obj);
}
void cancelButtonCB(CDKBUTTON *button) {
    exitCancelCDKScreenOf(&button->obj);
}


/*
 * Confirmation dialog with a message and two buttons (OK/Cancel). Typically
 * used as a last check for possibly dangerous operations. Accepts multiple
 * lines; caller should pass NULL for lines that shouldn't be set.
 */
boolean confirmDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *confirm = 0;
    char *message[CONFIRM_DIAG_MSG_SIZE] = {NULL};
    int selection = 0, i = 0;
    boolean ret_val = FALSE;

    /* Set the message */
    SAFE_ASPRINTF(&message[0], "<C></B>CONFIRM");
    SAFE_ASPRINTF(&message[1], " ");
    if (msg_line_1)
        SAFE_ASPRINTF(&message[2], "<C>%s", msg_line_1);
    else
        SAFE_ASPRINTF(&message[2], " ");
    if (msg_line_2)
         SAFE_ASPRINTF(&message[3], "<C>%s", msg_line_2);
    else
        SAFE_ASPRINTF(&message[3], " ");
    SAFE_ASPRINTF(&message[4], " ");
    SAFE_ASPRINTF(&message[5], " ");

    /* Display the confirmation dialog box */
    confirm = newCDKDialog(screen, CENTER, CENTER, message,
            CONFIRM_DIAG_MSG_SIZE, g_ok_cancel_msg, 2, COLOR_ERROR_SELECT,
            TRUE, TRUE, FALSE);
    if (confirm) {
        setCDKDialogBackgroundAttrib(confirm, COLOR_ERROR_TEXT);
        setCDKDialogBoxAttribute(confirm, COLOR_ERROR_BOX);
        injectCDKDialog(confirm, KEY_RIGHT);
        selection = activateCDKDialog(confirm, 0);

        /* Check how we exited the widget */
        if (confirm->exitType == vESCAPE_HIT) {
            ret_val = FALSE;
        } else if (confirm->exitType == vNORMAL) {
            if (selection == 0)
                ret_val = TRUE;
            else if (selection == 1)
                ret_val = FALSE;
        }
        destroyCDKDialog(confirm);
    }
    for (i = 0; i < CONFIRM_DIAG_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(screen);
    return ret_val;
}


/*
 * Present the user with a list of SCST targets and let them choose one. We
 * then fill the char arrays with the target name and target driver.
 * Optionally, if tgt_driver is set, then we will only display targets for
 * that driver.
 */
void getSCSTTgtChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[]) {
    CDKSCROLL *scst_tgt_list = 0;
    int tgt_choice = 0, i = 0, j = 0, driver_cnt = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_tgt_name[MAX_SCST_TGTS] = {NULL},
            *scst_tgt_driver[MAX_SCST_TGTS] = {NULL},
            *scst_tgt_info[MAX_SCST_TGTS] = {NULL};
    char *error_msg = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    char drivers[MAX_SCST_DRIVERS][MISC_STRING_LEN] = {{0}, {0}};
    boolean finished = FALSE;

    while (1) {
        if (tgt_driver[0] != '\0') {
            /* If a driver is given, then we only want
             * targets for that driver */
            driver_cnt = 1;
            snprintf(drivers[0], MISC_STRING_LEN, "%s", tgt_driver);
        } else {
            /* No driver was given, so get a list of them */
            if (!listSCSTTgtDrivers(drivers, &driver_cnt)) {
                SAFE_ASPRINTF(&error_msg, "%s", TGT_DRIVERS_ERR);
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }

        /* Loop over each SCST target driver and get targets */
        for (i = 0; i < driver_cnt; i++) {
            /* Open the directory */
            snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s",
                    SYSFS_SCST_TGT, drivers[i]);
            if ((dir_stream = opendir(dir_name)) == NULL) {
                SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                finished = TRUE;
                break;
            }

            /* Loop over each entry in the directory */
            while ((dir_entry = readdir(dir_stream)) != NULL) {
                /* The target names are directories; skip '.' and '..' */
                if ((dir_entry->d_type == DT_DIR) &&
                        (strcmp(dir_entry->d_name, ".") != 0) &&
                        (strcmp(dir_entry->d_name, "..") != 0)) {
                    if (j < MAX_SCST_TGTS) {
                        SAFE_ASPRINTF(&scst_tgt_name[j], "%s", dir_entry->d_name);
                        SAFE_ASPRINTF(&scst_tgt_driver[j], "%s", drivers[i]);
                        SAFE_ASPRINTF(&scst_tgt_info[j], "<C>%s (Driver: %s)",
                                dir_entry->d_name, drivers[i]);
                        j++;
                    }
                }
            }

            /* Close the directory stream */
            closedir(dir_stream);
        }
        if (finished)
            break;

        /* Make sure we actually have something to present */
        if (j == 0) {
            errorDialog(cdk_screen, "No targets found!", NULL);
            break;
        }

        /* Get SCST target choice from user */
        scst_tgt_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 60,
                "<C></31/B>Choose a SCST Target\n", scst_tgt_info, j,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_tgt_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_tgt_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_tgt_list, COLOR_DIALOG_TEXT);
        tgt_choice = activateCDKScroll(scst_tgt_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_tgt_list->exitType == vNORMAL) {
            strncpy(tgt_name, scst_tgt_name[tgt_choice], MAX_SYSFS_ATTR_SIZE);
            /* We only set the target driver if we weren't
             * given an initial value */
            if (tgt_driver[0] == '\0')
                strncpy(tgt_driver, scst_tgt_driver[tgt_choice],
                    MAX_SYSFS_ATTR_SIZE);
        }
        break;
    }

    /* Done */
    destroyCDKScroll(scst_tgt_list);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_SCST_TGTS; i++) {
        FREE_NULL(scst_tgt_name[i]);
        FREE_NULL(scst_tgt_driver[i]);
        FREE_NULL(scst_tgt_info[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST groups and let them choose one. The
 * driver / target name combination is passed in and we then fill the
 * char array with the selected group name.
 */
void getSCSTGroupChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[], char tgt_group[]) {
    CDKSCROLL *scst_grp_list = 0;
    int group_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_tgt_groups[MAX_SCST_GROUPS] = {NULL},
            *scroll_grp_list[MAX_SCST_GROUPS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups",
                SYSFS_SCST_TGT, tgt_driver, tgt_name);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The group names are directories; skip '.' and '..' */
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                if (i < MAX_SCST_GROUPS) {
                    SAFE_ASPRINTF(&scst_tgt_groups[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scroll_grp_list[i], "<C>%.30s",
                            scst_tgt_groups[i]);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No groups found!", NULL);
            break;
        }

        /* Get SCST target group choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose a Group (%s)\n", tgt_name);
        scst_grp_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
                scroll_title, scroll_grp_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_grp_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_grp_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_grp_list, COLOR_DIALOG_TEXT);
        group_choice = activateCDKScroll(scst_grp_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_grp_list->exitType == vNORMAL)
            strncpy(tgt_group, scst_tgt_groups[group_choice],
                    MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scst_grp_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_GROUPS; i++) {
        FREE_NULL(scst_tgt_groups[i]);
        FREE_NULL(scroll_grp_list[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST LUNs and let them choose one. The
 * driver / target / group name combination is passed in and we return
 * the LUN as an int; return -1 if there was an error or escape.
 */
int getSCSTLUNChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[],
        char tgt_group[]) {
    CDKSCROLL *lun_scroll = 0;
    int lun_choice = 0, i = 0, ret_val = 0, dev_path_size = 0;
    int luns[MAX_SCST_LUNS] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_lun_list[MAX_SCST_LUNS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            link_path[MAX_SYSFS_PATH_SIZE] = {0},
            dev_path[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/luns",
                SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            ret_val = -1;
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The LUNs are directories; skip '.' and '..' */
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                luns[i] = atoi(dir_entry->d_name);
                /* We need to get the device name (link) */
                snprintf(link_path, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/ini_groups/%s/luns/%d/device",
                        SYSFS_SCST_TGT, tgt_driver, tgt_name,
                        tgt_group, luns[i]);
                /* Read the link to get device name (doesn't
                 * append null byte) */
                dev_path_size = readlink(link_path, dev_path,
                        MAX_SYSFS_PATH_SIZE);
                if (dev_path_size < MAX_SYSFS_PATH_SIZE)
                    *(dev_path + dev_path_size) = '\0';
                /* For our scroll widget */
                if (i < MAX_SCST_LUNS) {
                    SAFE_ASPRINTF(&scst_lun_list[i], "<C>LUN %d (Device: %s)",
                            luns[i], (strrchr(dev_path, '/') + 1));
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No LUNs found!", NULL);
            ret_val = -1;
            break;
        }

        /* Get SCST LUN choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose a LUN (%s)\n",
                tgt_group);
        lun_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 45,
                scroll_title, scst_lun_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!lun_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            ret_val = -1;
            break;
        }
        setCDKScrollBoxAttribute(lun_scroll, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(lun_scroll, COLOR_DIALOG_TEXT);
        lun_choice = activateCDKScroll(lun_scroll, 0);

        /* Check exit from widget (set only if normal exit) */
        ret_val = -1;
        if (lun_scroll->exitType == vNORMAL)
            ret_val = luns[lun_choice];
        break;
    }

    /* Done */
    destroyCDKScroll(lun_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_LUNS; i++) {
        FREE_NULL(scst_lun_list[i]);
    }
    return ret_val;
}


/*
 * Give the user a list of SCSI disk devices and have them select one.
 * We return a char array with "H:C:I:L" for the chosen disk.
 */
char *getSCSIDiskChoice(CDKSCREEN *cdk_screen) {
    CDKSCROLL *scsi_dsk_list = 0;
    int disk_choice = 0, i = 0, dev_cnt = 0, j = 0;
    char *scsi_dsk_dev[MAX_SCSI_DISKS] = {NULL},
            *scsi_dsk_node[MAX_SCSI_DISKS] = {NULL},
            *scsi_dsk_model[MAX_SCSI_DISKS] = {NULL},
            *scsi_dsk_vendor[MAX_SCSI_DISKS] = {NULL},
            *scsi_dev_info[MAX_SCSI_DISKS] = {NULL};
    char *error_msg = NULL, *boot_dev_node = NULL;
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    while (1) {
        /* Get the ESOS boot device node */
        if ((boot_dev_node = blkid_get_devname(NULL, "LABEL",
                ESOS_BOOT_PART)) == NULL) {
            /* The function above returns NULL if the device isn't found */
            SAFE_ASPRINTF(&boot_dev_node, " ");
        }

        /* Open the directory to get SCSI disks */
        if ((dir_stream = opendir(SYSFS_SCSI_DISK)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory (SCSI disks) */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK && dev_cnt < MAX_SCSI_DISKS) {
                SAFE_ASPRINTF(&scsi_dsk_dev[dev_cnt], "%s", dir_entry->d_name);
                dev_cnt++;
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Loop over our list of SCSI disks */
        while ((i < dev_cnt) && (i < MAX_SCSI_DISKS)) {
            /* Get the SCSI block device node */
            snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/block",
                    SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
            if ((dir_stream = opendir(dir_name)) == NULL) {
                if (i < MAX_SCSI_DISKS)
                    SAFE_ASPRINTF(&scsi_dsk_node[i], "opendir(): %s",
                            strerror(errno));
            } else {
                while ((dir_entry = readdir(dir_stream)) != NULL) {
                    /* We want to skip the '.' and '..' directories */
                    if ((dir_entry->d_type == DT_DIR) &&
                            (strcmp(dir_entry->d_name, ".") != 0) &&
                            (strcmp(dir_entry->d_name, "..") != 0)) {
                        /* The first directory is the node name we're using */
                        if (i < MAX_SCSI_DISKS)
                            SAFE_ASPRINTF(&scsi_dsk_node[i], "%s",
                                    dir_entry->d_name);
                        break;
                    }
                }
                closedir(dir_stream);
            }
            /* Make sure this isn't the ESOS boot device (USB); if it is,
             * clean-up anything allocated, shift the elements, and
             * decrement device count */
            if ((strstr(boot_dev_node, scsi_dsk_node[i])) != NULL) {
                FREE_NULL(scsi_dsk_node[i]);
                scsi_dsk_node[i] = NULL;
                FREE_NULL(scsi_dsk_dev[i]);
                scsi_dsk_dev[i] = NULL;
                // TODO: This needs to be checked for safeness.
                for (j = i; j < dev_cnt; j++)
                    scsi_dsk_dev[j] = scsi_dsk_dev[j + 1];
                dev_cnt--;
                continue;
            }
            /* Read SCSI disk attributes */
            if (i < MAX_SCSI_DISKS) {
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/model",
                        SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
                readAttribute(dir_name, tmp_buff);
                SAFE_ASPRINTF(&scsi_dsk_model[i], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/vendor",
                        SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
                readAttribute(dir_name, tmp_buff);
                SAFE_ASPRINTF(&scsi_dsk_vendor[i], "%s", tmp_buff);
                /* Fill the list (pretty) for our CDK label with SCSI disks */
                SAFE_ASPRINTF(&scsi_dev_info[i], "<C>[%s] %s %s (/dev/%s)",
                        scsi_dsk_dev[i], scsi_dsk_vendor[i],
                        scsi_dsk_model[i], scsi_dsk_node[i]);
                /* Next */
                i++;
            }
        }

        /* Make sure we actually have something to present */
        if (dev_cnt == 0) {
            errorDialog(cdk_screen, "No SCSI disks found!", NULL);
            break;
        }

        /* Get SCSI disk choice from user */
        scsi_dsk_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
                "<C></31/B>Choose a SCSI Disk\n", scsi_dev_info, dev_cnt,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scsi_dsk_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scsi_dsk_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scsi_dsk_list, COLOR_DIALOG_TEXT);
        disk_choice = activateCDKScroll(scsi_dsk_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scsi_dsk_list->exitType == vNORMAL)
            strncpy(ret_buff, scsi_dsk_dev[disk_choice], MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scsi_dsk_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(boot_dev_node);
    for (i = 0; i < MAX_SCSI_DISKS; i++) {
        FREE_NULL(scsi_dsk_dev[i]);
        FREE_NULL(scsi_dsk_node[i]);
        FREE_NULL(scsi_dsk_model[i]);
        FREE_NULL(scsi_dsk_vendor[i]);
        FREE_NULL(scsi_dev_info[i]);
    }
    if (ret_buff[0] != '\0')
        return ret_buff;
    else
        return NULL;
}


/*
 * Present the user with a list of SCST devices and let them choose one. We
 * then fill the char arrays with the device name and the handler.
 */
void getSCSTDevChoice(CDKSCREEN *cdk_screen, char dev_name[],
        char dev_handler[]) {
    CDKSCROLL *scst_dev_list = 0;
    int dev_choice = 0, i = 0, j = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_dev_name[MAX_SCST_DEVS] = {NULL},
        *scst_dev_hndlr[MAX_SCST_DEVS] = {NULL},
        *scst_dev_info[MAX_SCST_DEVS] = {NULL};
    char *error_msg = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    boolean finished = FALSE;

    while (1) {
        /* Loop over each SCST handler type and grab any open device names */
        for (i = 0; i < (int)g_scst_handlers_size(); i++) {
            /* Open the directory */
            snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s",
                    SYSFS_SCST_TGT, g_scst_handlers[i]);
            if ((dir_stream = opendir(dir_name)) == NULL) {
                SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                finished = TRUE;
                break;
            }

            /* Loop over each entry in the directory */
            while ((dir_entry = readdir(dir_stream)) != NULL) {
                if (dir_entry->d_type == DT_LNK) {
                    if (j < MAX_SCST_DEVS) {
                        SAFE_ASPRINTF(&scst_dev_name[j], "%s", dir_entry->d_name);
                        SAFE_ASPRINTF(&scst_dev_hndlr[j], "%s", g_scst_handlers[i]);
                        SAFE_ASPRINTF(&scst_dev_info[j], "<C>%s (Handler: %s)",
                                dir_entry->d_name, g_scst_handlers[i]);
                        j++;
                    }
                }
            }

            /* Close the directory stream */
            closedir(dir_stream);
        }
        if (finished)
            break;

        /* Make sure we actually have something to present */
        if (j == 0) {
            errorDialog(cdk_screen, "No devices found!", NULL);
            break;
        }

        /* Get SCST device choice from user */
        scst_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
                "<C></31/B>Choose a SCST Device\n", scst_dev_info, j,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_dev_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_dev_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_dev_list, COLOR_DIALOG_TEXT);
        dev_choice = activateCDKScroll(scst_dev_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_dev_list->exitType == vNORMAL) {
            strncpy(dev_name, scst_dev_name[dev_choice],
                    MAX_SYSFS_ATTR_SIZE);
            strncpy(dev_handler, scst_dev_hndlr[dev_choice],
                    MAX_SYSFS_ATTR_SIZE);
        }
        break;
    }

    /* Done */
    destroyCDKScroll(scst_dev_list);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_SCST_DEVS; i++) {
        FREE_NULL(scst_dev_name[i]);
        FREE_NULL(scst_dev_hndlr[i]);
        FREE_NULL(scst_dev_info[i]);
    }
    return;
}


/*
 * Present a list of adapters to the user and return the adapter
 * number (ID) selected. Currently only MegaRAID adapters.
 * This function will also fill the adapter array.
 */
int getAdpChoice(CDKSCREEN *cdk_screen, MRADAPTER *mr_adapters[]) {
    CDKSCROLL *adapter_list = 0;
    int adp_count = 0, adp_choice = 0, i = 0;
    char *adapters[MAX_ADAPTERS] = {NULL};
    char *error_msg = NULL;

    /* Get MegaRAID adapters */
    adp_count = getMRAdapterCount();
    if (adp_count == -1) {
        errorDialog(cdk_screen,
                "The MegaCLI tool isn't working (or is not installed).", NULL);
        return -1;
    } else if (adp_count == 0) {
        errorDialog(cdk_screen, "No adapters found!", NULL);
        return -1;
    } else {
        for (i = 0; i < adp_count; i++) {
            mr_adapters[i] = getMRAdapter(i);
            if (!mr_adapters[i]) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't get data from MegaRAID adapter # %d!", i);
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                return -1;
            } else {
                if (i < MAX_ADAPTERS)
                    SAFE_ASPRINTF(&adapters[i], "<C>MegaRAID Adapter # %d: %s",
                            i, mr_adapters[i]->prod_name);
            }
        }
    }

    /* Get adapter choice from user */
    adapter_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 8, 50,
            "<C></31/B>Choose an Adapter\n", adapters, adp_count,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    setCDKScrollBoxAttribute(adapter_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(adapter_list, COLOR_DIALOG_TEXT);
    adp_choice = activateCDKScroll(adapter_list, 0);

    /* If the user hit escape, return -1 */
    if (adapter_list->exitType == vESCAPE_HIT)
        adp_choice = -1;

    /* Done */
    destroyCDKScroll(adapter_list);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_ADAPTERS; i++) {
        FREE_NULL(adapters[i]);
    }
    return adp_choice;
}


/*
 * Present the user with a list of SCST initiators for a particular group and
 * have them choose one. The driver / target / group name combination is passed
 * in and we fill the initiator char array.
 */
void getSCSTInitChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[], char tgt_group[], char initiator[]) {
    CDKSCROLL *lun_scroll = 0;
    int lun_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *init_list[MAX_SCST_INITS] = {NULL},
            *scroll_init_list[MAX_SCST_INITS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/initiators",
                SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The initiator names are files; skip 'mgmt' */
            if ((dir_entry->d_type == DT_REG) &&
                    (strcmp(dir_entry->d_name, "mgmt") != 0)) {
                if (i < MAX_SCST_INITS) {
                    SAFE_ASPRINTF(&init_list[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scroll_init_list[i], "<C>%.40s", init_list[i]);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No initiators found!", NULL);
            break;
        }

        /* Get SCST initiator choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose an Initiator (%s)\n",
                tgt_group);
        lun_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 45,
                scroll_title, scroll_init_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!lun_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(lun_scroll, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(lun_scroll, COLOR_DIALOG_TEXT);
        lun_choice = activateCDKScroll(lun_scroll, 0);

        /* Check exit from widget and copy data if normal */
        if (lun_scroll->exitType == vNORMAL)
            strncpy(initiator, init_list[lun_choice], MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(lun_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_INITS; i++) {
        FREE_NULL(init_list[i]);
        FREE_NULL(scroll_init_list[i]);
    }
    return;
}


/*
 * Synchronize the ESOS configuration files to the USB drive; this will also
 * dump the SCST configuration to a flat file (before sync'ing).
 */
void syncConfig(CDKSCREEN *main_cdk_screen) {
    CDKLABEL *sync_msg = 0;
    char scstadmin_cmd[MAX_SHELL_CMD_LEN] = {0},
            sync_conf_cmd[MAX_SHELL_CMD_LEN] = {0};
    char *error_msg = NULL;
    int ret_val = 0, exit_stat = 0;

    /* Display a nice short label message while we sync */
    sync_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
            g_sync_label_msg, g_sync_label_msg_size(), TRUE, FALSE);
    if (!sync_msg) {
        errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
        return;
    }
    setCDKLabelBackgroundAttrib(sync_msg, COLOR_DIALOG_TEXT);
    setCDKLabelBoxAttribute(sync_msg, COLOR_DIALOG_BOX);
    refreshCDKScreen(main_cdk_screen);

    while (1) {
        /* Dump the SCST configuration to a file, if its loaded */
        if (isSCSTLoaded()) {
            snprintf(scstadmin_cmd, MAX_SHELL_CMD_LEN,
                    "%s -force -nonkey -write_config %s > /dev/null 2>&1",
                    SCSTADMIN_TOOL, SCST_CONF);
            ret_val = system(scstadmin_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, SCSTADMIN_TOOL, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }

        /* Synchronize the ESOS configuration */
        snprintf(sync_conf_cmd, MAX_SHELL_CMD_LEN, "%s > /dev/null 2>&1",
                SYNC_CONF_TOOL);
        ret_val = system(sync_conf_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, SYNC_CONF_TOOL, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
        break;
    }

    /* Done */
    destroyCDKLabel(sync_msg);
    return;
}


/*
 * Give the user a list of user accounts to choose from; the list of users
 * is everyone that belongs to the "ESOS users" group.
 */
void getUserAcct(CDKSCREEN *cdk_screen, char user_acct[]) {
    CDKSCROLL *user_scroll = 0;
    struct group *group_info = NULL;
    char **grp_member = NULL;
    char *esos_grp_members[MAX_USERS] = {NULL},
            *scroll_list[MAX_USERS] = {NULL};
    int i = 0, user_cnt = 0, user_choice = 0;

    /* Get the specified ESOS group */
    group_info = getgrnam(ESOS_GROUP);
    if (!group_info) {
        errorDialog(cdk_screen, "Couldn't get ESOS group information!", NULL);
        return;
    }

    /* Add group members for scroll widget */
    for (grp_member = group_info->gr_mem; *grp_member; grp_member++) {
        if (user_cnt < MAX_USERS) {
            SAFE_ASPRINTF(&esos_grp_members[user_cnt], "%s", *grp_member);
            SAFE_ASPRINTF(&scroll_list[user_cnt], "<C>%.25s", *grp_member);
            user_cnt++;
        }
    }

    while (1) {
        /* Get user account choice */
        user_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 10, 30,
                "<C></31/B>Choose a User Account\n", scroll_list, user_cnt,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!user_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(user_scroll, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(user_scroll, COLOR_DIALOG_TEXT);
        user_choice = activateCDKScroll(user_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (user_scroll->exitType == vNORMAL)
            snprintf(user_acct, MAX_UNAME_LEN, "%s",
                    esos_grp_members[user_choice]);
        break;
    }

    /* Done */
    destroyCDKScroll(user_scroll);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_USERS; i++) {
        FREE_NULL(esos_grp_members[i]);
        FREE_NULL(scroll_list[i]);
    }
    return;
}


/*
 * Question dialog with a message and two buttons (OK/Cancel). Typically
 * used as a convenience to run some function after another dialog. Accepts
 * multiple lines; caller should pass NULL for lines that shouldn't be set.
 */
boolean questionDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *question = 0;
    char *message[QUEST_DIAG_MSG_SIZE] = {NULL};
    int selection = 0, i = 0;
    boolean ret_val = FALSE;

    /* Set the message */
    SAFE_ASPRINTF(&message[0], "<C></B>QUESTION");
    SAFE_ASPRINTF(&message[1], " ");
    if (msg_line_1)
        SAFE_ASPRINTF(&message[2], "<C>%s", msg_line_1);
    else
        SAFE_ASPRINTF(&message[2], " ");
    if (msg_line_2)
         SAFE_ASPRINTF(&message[3], "<C>%s", msg_line_2);
    else
        SAFE_ASPRINTF(&message[3], " ");
    SAFE_ASPRINTF(&message[4], " ");
    SAFE_ASPRINTF(&message[5], " ");

    /* Display the question dialog box */
    question = newCDKDialog(screen, CENTER, CENTER, message,
            QUEST_DIAG_MSG_SIZE, g_yes_no_msg, 2, COLOR_DIALOG_SELECT,
            TRUE, TRUE, FALSE);
    if (question) {
        setCDKDialogBackgroundAttrib(question, COLOR_DIALOG_TEXT);
        setCDKDialogBoxAttribute(question, COLOR_DIALOG_BOX);
        selection = activateCDKDialog(question, 0);

        /* Check how we exited the widget */
        if (question->exitType == vESCAPE_HIT) {
            ret_val = FALSE;
        } else if (question->exitType == vNORMAL) {
            if (selection == 0)
                ret_val = TRUE;
            else if (selection == 1)
                ret_val = FALSE;
        }
        destroyCDKDialog(question);
    }
    for (i = 0; i < QUEST_DIAG_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(screen);
    return ret_val;
}


/*
 * Present a list of file systems from /etc/fstab and have the user pick one.
 */
void getFSChoice(CDKSCREEN *cdk_screen, char fs_name[], char fs_path[],
        char fs_type[], boolean *mounted) {
    CDKSCROLL *fs_scroll = 0;
    char *fs_names[MAX_FILE_SYSTEMS] = {NULL},
            *fs_paths[MAX_FILE_SYSTEMS] = {NULL},
            *fs_types[MAX_FILE_SYSTEMS] = {NULL},
            *scroll_list[MAX_USERS] = {NULL};
    char *error_msg = NULL;
    char mnt_line_buffer[MAX_MNT_LINE_BUFFER] = {0};
    int i = 0, fs_cnt = 0, user_choice = 0, mnt_line_size = 0, mnt_dir_size = 0;
    boolean fs_mounted[MAX_FILE_SYSTEMS] = {FALSE};
    FILE *fstab_file = NULL, *mtab_file = NULL;
    struct mntent *fstab_entry = NULL, *mtab_entry = NULL;

    /* Make a list of file systems that are mounted (by mount
     * path, not device) */
    if ((mtab_file = setmntent(MTAB, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
        return;
    }
    while ((mtab_entry = getmntent(mtab_file)) != NULL) {
        /* We add two extra: one for a space, and one for the null byte */
        mnt_dir_size = strlen(mtab_entry->mnt_dir) + 2;
        mnt_line_size = mnt_line_size + mnt_dir_size;
        if (mnt_line_size >= MAX_MNT_LINE_BUFFER) {
            errorDialog(cdk_screen,
                    "The maximum mount line buffer size has been reached!",
                    NULL);
            endmntent(mtab_file);
            return;
        } else {
            strcat(mnt_line_buffer, mtab_entry->mnt_dir);
            strcat(mnt_line_buffer, " ");
        }
    }
    endmntent(mtab_file);

    /* Open the file system tab file */
    if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
        return;
    }

    /* Loop over fstab entries */
    fs_cnt = 0;
    while ((fstab_entry = getmntent(fstab_file)) != NULL) {
        /* We don't want to grab special entries from fstab that
         * we shouldn't touch */
        if (strstr(SYS_FILE_SYSTEMS, fstab_entry->mnt_dir) == NULL) {
            if (fs_cnt < MAX_FILE_SYSTEMS) {
                SAFE_ASPRINTF(&fs_names[fs_cnt], "%s", fstab_entry->mnt_fsname);
                SAFE_ASPRINTF(&fs_paths[fs_cnt], "%s", fstab_entry->mnt_dir);
                SAFE_ASPRINTF(&fs_types[fs_cnt], "%s", fstab_entry->mnt_type);
                if (strstr(mnt_line_buffer, fstab_entry->mnt_dir) == NULL)
                    fs_mounted[fs_cnt] = FALSE;
                else
                    fs_mounted[fs_cnt] = TRUE;
                SAFE_ASPRINTF(&scroll_list[fs_cnt],
                        "<C>%-25.25s %-20.20s %-5.5s (Mounted: %d)",
                        fs_names[fs_cnt], fs_paths[fs_cnt],
                        fs_types[fs_cnt], fs_mounted[fs_cnt]);
                fs_cnt++;
            }
        }
    }

    /* Close up shop */
    endmntent(fstab_file);

    while (1) {
        /* Make sure we actually have something to present */
        if (fs_cnt == 0) {
            errorDialog(cdk_screen, "No useful file systems were found!", NULL);
            break;
        }

        /* Get file system choice */
        fs_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                "<C></31/B>Choose a File System\n", scroll_list, fs_cnt,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!fs_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(fs_scroll, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(fs_scroll, COLOR_DIALOG_TEXT);
        user_choice = activateCDKScroll(fs_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (fs_scroll->exitType == vNORMAL) {
            snprintf(fs_name, MAX_FS_ATTR_LEN, "%s", fs_names[user_choice]);
            snprintf(fs_path, MAX_FS_ATTR_LEN, "%s", fs_paths[user_choice]);
            snprintf(fs_type, MAX_FS_ATTR_LEN, "%s", fs_types[user_choice]);
            *mounted = fs_mounted[user_choice];
        }
        break;
    }

    /* Done */
    destroyCDKScroll(fs_scroll);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < fs_cnt; i++) {
        FREE_NULL(fs_names[i]);
        FREE_NULL(fs_paths[i]);
        FREE_NULL(fs_types[i]);
        FREE_NULL(scroll_list[i]);
    }
    return;
}


/*
 * Give the user a list of block devices (SCSI, etc.) and have them select one.
 * We return a char array with the "/dev/X" path for the chosen device.
 */
char *getBlockDevChoice(CDKSCREEN *cdk_screen) {
    CDKSCROLL *block_dev_list = 0;
    int blk_dev_choice = 0, i = 0, dev_cnt = 0, exit_stat = 0, ret_val = 0,
            blk_dev_fd = 0;
    char *blk_dev_name[MAX_BLOCK_DEVS] = {NULL},
            *blk_dev_info[MAX_BLOCK_DEVS] = {NULL},
            *blk_dev_size[MAX_BLOCK_DEVS] = {NULL},
            *blk_dev_scroll_lines[MAX_BLOCK_DEVS] = {NULL};
    char *error_msg = NULL, *boot_dev_node = NULL, *dev_node_ptr = NULL,
            *cmd_str = NULL, *block_dev = NULL;
    static char ret_buff[MAX_SYSFS_PATH_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            sym_links[MAX_SYSFS_ATTR_SIZE] = {0},
            dev_node_test[MISC_STRING_LEN] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    FILE *udevadm_cmd = NULL;
    boolean finished = FALSE;

    /* Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    while (1) {
        /* Get the ESOS boot device node */
        if ((boot_dev_node = blkid_get_devname(NULL, "LABEL",
                ESOS_BOOT_PART)) == NULL) {
            /* The function above returns NULL if the device isn't found */
            SAFE_ASPRINTF(&boot_dev_node, " ");
        } else {
            /* Found the device so chop off the partition number */
            *(boot_dev_node + strlen(boot_dev_node) - 1) = '\0';
        }

        /* Open the directory to get block devices */
        if ((dir_stream = opendir(SYSFS_BLOCK)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory (block devices) */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK) {
                snprintf(dev_node_test, MISC_STRING_LEN,
                        "/dev/%s", dir_entry->d_name);
                /* Test to see if the block device is already open */
                if ((blk_dev_fd = open(dev_node_test, O_EXCL)) == -1) {
                    continue;
                } else {
                    if (close(blk_dev_fd) == -1) {
                        SAFE_ASPRINTF(&error_msg, "close(): %s",
                                strerror(errno));
                        errorDialog(cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        finished = TRUE;
                        break;
                    }
                }

                if (strcmp(boot_dev_node, dev_node_test) == 0) {
                    /* We don't want to show the ESOS boot block
                     * device (USB drive) */
                    continue;

                } else if ((strstr(dev_node_test, "/dev/drbd")) != NULL) {
                    /* For DRBD block devices (not sure if the /dev/drbdX
                     * format is forced when using drbdadm, so this may
                     * be a problem */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        /* Nothing extra for DRBD... yet */
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt], "DRBD Device");
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/md")) != NULL) {
                    /* For software RAID (md) devices; it appears the mdadm
                     * tool forces the /dev/mdX device node name format */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/md/level", SYSFS_BLOCK,
                                blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt], "Level: %s",
                                tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/sd")) != NULL) {
                    /* For normal SCSI block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/device/model",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt], "Model: %s",
                                tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/dm-")) != NULL) {
                    /* For device mapper (eg, LVM2) block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/dm/name",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt], "Name: %s",
                                tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/cciss")) != NULL) {
                    /* For Compaq SMART array controllers */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/device/raid_level",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt],
                                "RAID Level: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/zd")) != NULL) {
                    /* For ZFS block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        SAFE_ASPRINTF(&blk_dev_name[dev_cnt], "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/queue/logical_block_size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        SAFE_ASPRINTF(&blk_dev_info[dev_cnt],
                                "Block Size: %s", tmp_buff);
                        dev_cnt++;
                    }
                }
                // TODO: Still more controller block devices (ida, rd)
                // need to be added but we need hardware so we can
                // confirm sysfs attributes.
            }
        }
        if (finished)
            break;

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (dev_cnt == 0) {
            errorDialog(cdk_screen, "No block devices found!", NULL);
            break;
        }

        /* Fill the list (pretty) for our CDK label with block devices */
        for (i = 0; i < dev_cnt; i++) {
            if (i < MAX_BLOCK_DEVS) {
                SAFE_ASPRINTF(&blk_dev_scroll_lines[i],
                        "<C>%-10.10s Size: %-12.12s %-30.30s",
                        blk_dev_name[i], blk_dev_size[i], blk_dev_info[i]);
            }
        }

        /* Get block device choice from user */
        block_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 65,
                "<C></31/B>Choose a Block Device\n", blk_dev_scroll_lines,
                dev_cnt, FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!block_dev_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(block_dev_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(block_dev_list, COLOR_DIALOG_TEXT);
        blk_dev_choice = activateCDKScroll(block_dev_list, 0);

        if (block_dev_list->exitType == vNORMAL) {
            /* Return a unique block device node */
            SAFE_ASPRINTF(&block_dev, "/dev/%s", blk_dev_name[blk_dev_choice]);
            if ((strstr(block_dev, "/dev/sd")) != NULL) {
                /* Get a unique symbolic link to the SCSI disk */
                SAFE_ASPRINTF(&cmd_str,
                        "%s info --root --query symlink --name %s 2>&1",
                        UDEVADM_BIN, block_dev);
                udevadm_cmd = popen(cmd_str, "r");
                fgets(sym_links, sizeof (sym_links), udevadm_cmd);
                if ((exit_stat = pclose(udevadm_cmd)) == -1) {
                    ret_val = -1;
                } else {
                    if (WIFEXITED(exit_stat))
                        ret_val = WEXITSTATUS(exit_stat);
                    else
                        ret_val = -1;
                }
                FREE_NULL(cmd_str);
                if (ret_val != 0) {
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                            UDEVADM_BIN, ret_val);
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                /* We only want the first sym link */
                dev_node_ptr = strtok(sym_links, " ");
                assert(dev_node_ptr != NULL);
                snprintf(ret_buff, MAX_SYSFS_PATH_SIZE, "%s", dev_node_ptr);

            } else if ((strstr(block_dev, "/dev/dm-")) != NULL) {
                /* A /dev/dm-* device, we're assuming its
                 * an LVM logical volume */
                if ((dev_node_ptr = strstr(block_dev, "dm-")) != NULL) {
                    snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/dm/name",
                            SYSFS_BLOCK, dev_node_ptr);
                    readAttribute(attr_path, attr_value);
                    snprintf(ret_buff, MAX_SYSFS_PATH_SIZE, "/dev/mapper/%s",
                            attr_value);
                }

            } else {
                /* Not a normal SCSI disk or block device (md, drbd, etc.) */
                snprintf(ret_buff, MAX_SYSFS_PATH_SIZE, "%s", block_dev);
            }
        }
        break;
    }

    /* Done */
    destroyCDKScroll(block_dev_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(boot_dev_node);
    FREE_NULL(block_dev);
    for (i = 0; i < MAX_BLOCK_DEVS; i++) {
        FREE_NULL(blk_dev_name[i]);
        FREE_NULL(blk_dev_info[i]);
        FREE_NULL(blk_dev_size[i]);
        FREE_NULL(blk_dev_scroll_lines[i]);
    }
    if (ret_buff[0] != '\0')
        return ret_buff;
    else
        return NULL;
}


/*
 * Give the user a list of SCSI devices (based on scsi_dev_type) and have
 * them select one. We return a char array with "H:C:I:L" for the chosen device.
 */
char *getSCSIDevChoice(CDKSCREEN *cdk_screen, int scsi_dev_type) {
    CDKSCROLL *scsi_dev_list = 0;
    int dev_choice = 0, i = 0, dev_cnt = 0;
    char *scsi_device[MAX_SCSI_DEVICES] = {NULL},
            *scsi_dev_rev[MAX_SCSI_DEVICES] = {NULL},
            *scsi_dev_model[MAX_SCSI_DEVICES] = {NULL},
            *scsi_dev_vendor[MAX_SCSI_DEVICES] = {NULL},
            *scsi_dev_info[MAX_SCSI_DEVICES] = {NULL};
    char *error_msg = NULL, *list_title = NULL;
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    while (1) {
        /* Open the directory to get SCSI devices */
        if ((dir_stream = opendir(SYSFS_SCSI_DEVICE)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory (SCSI devices) */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK && dev_cnt < MAX_SCSI_DEVICES) {
                /* We only want devices that match the given type */
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/type",
                        SYSFS_SCSI_DEVICE, dir_entry->d_name);
                readAttribute(dir_name, tmp_buff);
                if ((atoi(tmp_buff)) == scsi_dev_type) {
                    SAFE_ASPRINTF(&scsi_device[dev_cnt], "%s", dir_entry->d_name);
                    dev_cnt++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Loop over our list of SCSI devices and get some attributes */
        while (i < dev_cnt) {
            if (i < MAX_SCSI_DEVICES) {
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/model",
                        SYSFS_SCSI_DEVICE, scsi_device[i]);
                readAttribute(dir_name, tmp_buff);
                SAFE_ASPRINTF(&scsi_dev_model[i], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/vendor",
                        SYSFS_SCSI_DEVICE, scsi_device[i]);
                readAttribute(dir_name, tmp_buff);
                SAFE_ASPRINTF(&scsi_dev_vendor[i], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/rev",
                        SYSFS_SCSI_DEVICE, scsi_device[i]);
                readAttribute(dir_name, tmp_buff);
                SAFE_ASPRINTF(&scsi_dev_rev[i], "%s", tmp_buff);
                /* Fill the list (pretty) for our CDK label with SCSI devices */
                SAFE_ASPRINTF(&scsi_dev_info[i], "<C>[%s] %s %s %s", scsi_device[i],
                        scsi_dev_vendor[i], scsi_dev_model[i], scsi_dev_rev[i]);
                /* Next */
                i++;
            }
        }

        /* Make sure we actually have something to present */
        if (dev_cnt == 0) {
            errorDialog(cdk_screen, "No SCSI devices found!", NULL);
            break;
        }

        /* Get SCSI device choice from user */
        SAFE_ASPRINTF(&list_title, "<C></31/B>Choose a SCSI Device (Type %d)\n",
                scsi_dev_type);
        scsi_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
                list_title, scsi_dev_info, dev_cnt,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scsi_dev_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scsi_dev_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scsi_dev_list, COLOR_DIALOG_TEXT);
        dev_choice = activateCDKScroll(scsi_dev_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scsi_dev_list->exitType == vNORMAL)
            strncpy(ret_buff, scsi_device[dev_choice], MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scsi_dev_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(list_title);
    for (i = 0; i < MAX_SCSI_DEVICES; i++) {
        FREE_NULL(scsi_device[i]);
        FREE_NULL(scsi_dev_rev[i]);
        FREE_NULL(scsi_dev_model[i]);
        FREE_NULL(scsi_dev_vendor[i]);
        FREE_NULL(scsi_dev_info[i]);
    }
    if (ret_buff[0] != '\0')
        return ret_buff;
    else
        return NULL;
}


/*
 * Present the user with a list of SCST ALUA device groups and let them
 * choose one. We then fill the char array with the chosen device group name.
 */
void getSCSTDevGrpChoice(CDKSCREEN *cdk_screen, char alua_dev_group[]) {
    CDKSCROLL *scst_dev_grp_list = 0;
    int dev_grp_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_dev_grp[MAX_SCST_DEV_GRPS] = {NULL},
            *scst_dev_grp_info[MAX_SCST_DEV_GRPS] = {NULL};
    char *error_msg = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/device_groups",
                SYSFS_SCST_TGT);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                if (i < MAX_SCST_DEV_GRPS) {
                    SAFE_ASPRINTF(&scst_dev_grp[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scst_dev_grp_info[i], "<C>%.30s",
                            dir_entry->d_name);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No device groups found!", NULL);
            break;
        }

        /* Get SCST device group choice from user */
        scst_dev_grp_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE,
                11, 32, "<C></31/B>Choose a SCST Device Group\n",
                scst_dev_grp_info, i, FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_dev_grp_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_dev_grp_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_dev_grp_list, COLOR_DIALOG_TEXT);
        dev_grp_choice = activateCDKScroll(scst_dev_grp_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_dev_grp_list->exitType == vNORMAL)
            strncpy(alua_dev_group, scst_dev_grp[dev_grp_choice],
                    MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scst_dev_grp_list);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_SCST_DEV_GRPS; i++) {
        FREE_NULL(scst_dev_grp[i]);
        FREE_NULL(scst_dev_grp_info[i]);
    }
    return;
}

/*
 * Present the user with a list of SCST ALUA target groups and let them
 * choose one. The device group name is passed in and we then fill the
 * char array with the selected target group name.
 */
void getSCSTTgtGrpChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_tgt_group[]) {
    CDKSCROLL *scst_tgt_grp_list = 0;
    int tgt_grp_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_tgt_groups[MAX_SCST_TGT_GRPS] = {NULL},
            *scroll_tgt_grp_list[MAX_SCST_TGT_GRPS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/target_groups",
                SYSFS_SCST_TGT, alua_dev_group);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The group names are directories; skip '.' and '..' */
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                if (i < MAX_SCST_TGT_GRPS) {
                    SAFE_ASPRINTF(&scst_tgt_groups[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scroll_tgt_grp_list[i], "<C>%.30s",
                            scst_tgt_groups[i]);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No target groups found!", NULL);
            break;
        }

        /* Get SCST target group choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose a Target Group (%s)\n",
                alua_dev_group);
        scst_tgt_grp_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE,
                11, 44, scroll_title, scroll_tgt_grp_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_tgt_grp_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_tgt_grp_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_tgt_grp_list, COLOR_DIALOG_TEXT);
        tgt_grp_choice = activateCDKScroll(scst_tgt_grp_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_tgt_grp_list->exitType == vNORMAL)
            strncpy(alua_tgt_group, scst_tgt_groups[tgt_grp_choice],
                MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scst_tgt_grp_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_TGT_GRPS; i++) {
        FREE_NULL(scst_tgt_groups[i]);
        FREE_NULL(scroll_tgt_grp_list[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST ALUA device group devices and let them
 * choose one. The device group name is passed in and we then fill the
 * char array with the selected device name.
 */
void getSCSTDevGrpDevChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_dev_grp_dev[]) {
    CDKSCROLL *scst_dev_grp_dev_list = 0;
    int dev_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_device_names[MAX_SCST_DEV_GRP_DEVS] = {NULL},
            *scroll_dev_list[MAX_SCST_DEV_GRP_DEVS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/devices",
                SYSFS_SCST_TGT, alua_dev_group);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK) {
                if (i < MAX_SCST_DEV_GRP_DEVS) {
                    SAFE_ASPRINTF(&scst_device_names[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scroll_dev_list[i], "<C>%.30s",
                            scst_device_names[i]);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No devices found!", NULL);
            break;
        }

        /* Get SCST device group device choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose a Device (%s)\n",
                alua_dev_group);
        scst_dev_grp_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE,
                11, 44, scroll_title, scroll_dev_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_dev_grp_dev_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_dev_grp_dev_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_dev_grp_dev_list, COLOR_DIALOG_TEXT);
        dev_choice = activateCDKScroll(scst_dev_grp_dev_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_dev_grp_dev_list->exitType == vNORMAL)
            strncpy(alua_dev_grp_dev, scst_device_names[dev_choice],
                MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scst_dev_grp_dev_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_DEV_GRP_DEVS; i++) {
        FREE_NULL(scst_device_names[i]);
        FREE_NULL(scroll_dev_list[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST ALUA device group devices and let them
 * choose one. The device group name is passed in and we then fill the
 * char array with the selected device name.
 */
void getSCSTTgtGrpTgtChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_tgt_group[], char alua_tgt_grp_tgt[]) {
    CDKSCROLL *scst_tgt_grp_tgt_list = 0;
    int tgt_choice = 0, i = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_target_names[MAX_SCST_TGT_GRP_TGTS] = {NULL},
            *scroll_tgt_list[MAX_SCST_TGT_GRP_TGTS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    while (1) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/target_groups/%s",
                SYSFS_SCST_TGT, alua_dev_group, alua_tgt_group);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) ||
                    (dir_entry->d_type == DT_LNK)) {
                if (i < MAX_SCST_TGT_GRP_TGTS) {
                    SAFE_ASPRINTF(&scst_target_names[i], "%s", dir_entry->d_name);
                    SAFE_ASPRINTF(&scroll_tgt_list[i], "<C>%.30s",
                            scst_target_names[i]);
                    i++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Make sure we actually have something to present */
        if (i == 0) {
            errorDialog(cdk_screen, "No targets found!", NULL);
            break;
        }

        /* Get SCST device group device choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></31/B>Choose a Target (%s -> %s)\n",
                alua_dev_group, alua_tgt_group);
        scst_tgt_grp_tgt_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE,
                11, 44, scroll_title, scroll_tgt_list, i,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!scst_tgt_grp_tgt_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_tgt_grp_tgt_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(scst_tgt_grp_tgt_list, COLOR_DIALOG_TEXT);
        tgt_choice = activateCDKScroll(scst_tgt_grp_tgt_list, 0);

        /* Check exit from widget and copy data if normal */
        if (scst_tgt_grp_tgt_list->exitType == vNORMAL)
            strncpy(alua_tgt_grp_tgt, scst_target_names[tgt_choice],
                MAX_SYSFS_ATTR_SIZE);
        break;
    }

    /* Done */
    destroyCDKScroll(scst_tgt_grp_tgt_list);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_TGT_GRP_TGTS; i++) {
        FREE_NULL(scst_target_names[i]);
        FREE_NULL(scroll_tgt_list[i]);
    }
    return;
}


/*
 * Check that the given char pointer string contains "valid characters" based
 * on the valid-character-type parameter. If the string is good, return TRUE,
 * and FALSE if its bad (doesn't pass test). If the string does not pass
 * validation, we also print an error message pop-up to the given CDK screen.
 */
boolean checkInputStr(CDKSCREEN *cdk_screen, valid_input_t char_test_type,
        char *input_str) {
    int i = 0;

    /* Make sure this is sane */
    if (input_str == NULL) {
        errorDialog(cdk_screen, NULL_STR_PTR_ERR, NULL);
        return FALSE;
    }

    while (*input_str != '\0') {
        /* So we don't hang forever if the null character is missing */
        if (i > MAX_TUI_STR_LEN) {
            errorDialog(cdk_screen, MAX_STR_SIZE_ERR, NULL);
            return FALSE;
        }
        /* Make sure the input string is valid */
        switch (char_test_type) {
            case ASCII_CHARS:
                if (!VALID_ASCII_CHAR(*input_str)) {
                    errorDialog(cdk_screen, INVALID_CHAR_ERR,
                            VALID_ASCII_CHAR_MSG);
                    return FALSE;
                }
                break;
            case NAME_CHARS:
                if (!VALID_NAME_CHAR(*input_str)) {
                    errorDialog(cdk_screen, INVALID_CHAR_ERR,
                            VALID_NAME_CHAR_MSG);
                    return FALSE;
                }
                break;
            case IPADDR_CHARS:
                if (!VALID_IPADDR_CHAR(*input_str)) {
                    errorDialog(cdk_screen, INVALID_CHAR_ERR,
                            VALID_IPADDR_CHAR_MSG);
                    return FALSE;
                }
                break;
            case EMAIL_CHARS:
                if (!VALID_EMAIL_CHAR(*input_str)) {
                    errorDialog(cdk_screen, INVALID_CHAR_ERR,
                            VALID_EMAIL_CHAR_MSG);
                    return FALSE;
                }
                break;
            case INIT_CHARS:
                if (!VALID_INIT_CHAR(*input_str)) {
                    errorDialog(cdk_screen, INVALID_CHAR_ERR,
                            VALID_INIT_CHAR_MSG);
                    return FALSE;
                }
                break;
            default:
                DEBUG_LOG(DEFAULT_CASE_HIT);
                return FALSE;
        }
        ++input_str;
        ++i;
    }

    /* The input/entry field string is empty */
    if (i == 0) {
        errorDialog(cdk_screen, EMPTY_FIELD_ERR, NULL);
        return FALSE;
    }

    /* We made it this far, so must be valid */
    return TRUE;
}


/*
 * Retrieve a list of of network interfaces (eg, Ethernet) on this system,
 * build a scroll widget dialog and present it to the user. The top option (0)
 * in the scroll widget represents the "general" network settings option.
 */
void getNetConfChoice(CDKSCREEN* cdk_screen, boolean *general_opt,
        char iface_name[], char iface_mac[], char iface_speed[],
        char iface_duplex[], bonding_t *iface_bonding, boolean *iface_bridge,
        char **slaves, int *slave_cnt, char **br_members, int *br_member_cnt) {
    CDKSCROLL *net_conf_list = 0;
    struct ifreq ifr; /* How do we properly initialize this? */
    struct if_nameindex* if_name = NULL;
    struct ethtool_cmd edata = {0};
    unsigned char* mac_addy = NULL;
    int sock = 0, i = 0, j = 0, net_conf_choice = 0;
    char *net_scroll_msg[MAX_NET_IFACE] = {NULL},
            *net_if_name[MAX_NET_IFACE] = {NULL},
            *net_if_mac[MAX_NET_IFACE] = {NULL},
            *net_if_speed[MAX_NET_IFACE] = {NULL},
            *net_if_duplex[MAX_NET_IFACE] = {NULL};
    char *error_msg = NULL;
    char eth_duplex[MISC_STRING_LEN] = {0}, temp_str[MISC_STRING_LEN] = {0};
    __be16 eth_speed = 0;
    short saved_ifr_flags = 0;
    bonding_t net_if_bonding[MAX_NET_IFACE] = {0};
    struct stat bridge_test = {0};
    boolean net_if_bridge[MAX_NET_IFACE] = {FALSE};

    /* Get socket handle */
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        errorDialog(cdk_screen, "Couldn't get socket handle!", NULL);
        return;
    }

    /* Get all network interface names */
    if_name = if_nameindex();

    /* We set the counter ahead since the first row is for general settings */
    j = 1;
    SAFE_ASPRINTF(&net_scroll_msg[0], "<C>General Network Settings");

    while ((if_name[i].if_index != 0) && (if_name[i].if_name != NULL) &&
            (j < MAX_NET_IFACE)) {
        /* Put the interface name into the ifreq struct */
        memcpy(&ifr.ifr_name, if_name[i].if_name, IFNAMSIZ);

        /* Get interface flags */
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
            SAFE_ASPRINTF(&error_msg, "ioctl(): SIOCGIFFLAGS Error (%s)",
                    ifr.ifr_name);
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }
        saved_ifr_flags = ifr.ifr_flags;

        /* We don't want to include the loopback interface */
        if (!(saved_ifr_flags & IFF_LOOPBACK)) {

            /* Get interface hardware address (MAC) */
            if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
                SAFE_ASPRINTF(&error_msg, "ioctl(): SIOCGIFHWADDR Error (%s)",
                        ifr.ifr_name);
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                return;
            }

            if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
                /* For Ethernet interfaces */
                mac_addy = (unsigned char*) &ifr.ifr_hwaddr.sa_data;
                SAFE_ASPRINTF(&net_if_name[j], "%s", if_name[i].if_name);
                SAFE_ASPRINTF(&net_if_mac[j], "%02X:%02X:%02X:%02X:%02X:%02X",
                        mac_addy[0], mac_addy[1], mac_addy[2], mac_addy[3],
                        mac_addy[4], mac_addy[5]);

                /* Check if interface is a bridge */
                snprintf(temp_str, MISC_STRING_LEN, "%s/%s/bridge",
                        SYSFS_NET, ifr.ifr_name);
                if (stat(temp_str, &bridge_test) == 0) {
                    net_if_bridge[j] = TRUE;
                    snprintf(temp_str, MISC_STRING_LEN, "Ethernet Bridge");
                    SAFE_ASPRINTF(&net_scroll_msg[j], "<C>%-9s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], temp_str);
                    /* We can continue to the next iteration if its a bridge */
                    j++;
                    i++;
                    continue;
                } else {
                    net_if_bridge[j] = FALSE;
                }

                /* Check for NIC bonding */
                if (saved_ifr_flags & IFF_MASTER) {
                    net_if_bonding[j] = MASTER;
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            g_bonding_map[net_if_bonding[j]]);
                    SAFE_ASPRINTF(&net_scroll_msg[j], "<C>%-9s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], temp_str);
                    /* We can continue to the next iteration if its a master */
                    j++;
                    i++;
                    continue;
                } else if (saved_ifr_flags & IFF_SLAVE) {
                    net_if_bonding[j] = SLAVE;
                    /* Already a bonding slave interface, add it to the list */
                    if (*slave_cnt < MAX_NET_IFACE) {
                        SAFE_ASPRINTF(&slaves[*slave_cnt], "%s", net_if_name[j]);
                        *slave_cnt = *slave_cnt + 1;
                    }
                } else {
                    net_if_bonding[j] = NO_BONDING;
                    /* No bonding for this interface, add it to the list */
                    if (*slave_cnt < MAX_NET_IFACE) {
                        SAFE_ASPRINTF(&slaves[*slave_cnt], "%s", net_if_name[j]);
                        *slave_cnt = *slave_cnt + 1;
                    }
                    /* For now we only grab interfaces that have no
                     * part in bonding */
                    if (*br_member_cnt < MAX_NET_IFACE) {
                        SAFE_ASPRINTF(&br_members[*br_member_cnt],
                                "%s", net_if_name[j]);
                        *br_member_cnt = *br_member_cnt + 1;
                    }
                }

                /* Get additional Ethernet interface information */
                ifr.ifr_data = (caddr_t) & edata;
                edata.cmd = ETHTOOL_GSET;
                if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
                    SAFE_ASPRINTF(&error_msg, "ioctl(): SIOCETHTOOL Error (%s)",
                            ifr.ifr_name);
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    return;
                }

                /* Get speed of Ethernet link; we use the returned speed value
                 * to determine the link status -- probably not the best
                 * solution long term, but its easy for now */
                eth_speed = ethtool_cmd_speed(&edata);
                if (eth_speed == 0 || eth_speed == (__be16) (-1) ||
                        eth_speed == (__be32) (-1)) {
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            g_bonding_map[net_if_bonding[j]]);
                    SAFE_ASPRINTF(&net_scroll_msg[j], "<C>%-9s%-21s%-16s%-26s",
                            net_if_name[j], net_if_mac[j],
                            temp_str, "No Link");
                } else {
                    switch (edata.duplex) {
                        case DUPLEX_HALF:
                            snprintf(eth_duplex, MISC_STRING_LEN,
                                    "Half Duplex");
                            break;
                        case DUPLEX_FULL:
                            snprintf(eth_duplex, MISC_STRING_LEN,
                                    "Full Duplex");
                            break;
                        default:
                            snprintf(eth_duplex, MISC_STRING_LEN,
                                    "Unknown Duplex");
                            break;
                    }
                    SAFE_ASPRINTF(&net_if_speed[j], "%u Mb/s", eth_speed);
                    SAFE_ASPRINTF(&net_if_duplex[j], "%s", eth_duplex);
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            g_bonding_map[net_if_bonding[j]]);
                    SAFE_ASPRINTF(&net_scroll_msg[j],
                            "<C>%-9s%-21s%-16s%-12s%-14s",
                            net_if_name[j], net_if_mac[j],
                            temp_str, net_if_speed[j],
                            net_if_duplex[j]);
                }
                j++;

            } else if (ifr.ifr_hwaddr.sa_family == ARPHRD_INFINIBAND) {
                /* For InfiniBand interfaces */
                mac_addy = (unsigned char*) &ifr.ifr_hwaddr.sa_data;
                SAFE_ASPRINTF(&net_if_name[j], "%s", if_name[i].if_name);
                /* Yes, the link-layer address is 20 bytes, but we'll
                 * keep it simple */
                SAFE_ASPRINTF(&net_if_mac[j],
                        "%02X:%02X:%02X:%02X:%02X:%02X...",
                        mac_addy[0], mac_addy[1], mac_addy[2],
                        mac_addy[3], mac_addy[4], mac_addy[5]);
                SAFE_ASPRINTF(&net_scroll_msg[j], "<C>%-9s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], "IPoIB");
                j++;
            }
        }
        i++;
    }

    /* Clean up */
    if_freenameindex(if_name);

    while (1) {
        /* Scroll widget for network configuration choices */
        net_conf_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 76,
                "<C></31/B>Choose a Network Configuration Option\n",
                net_scroll_msg, j, FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!net_conf_list) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(net_conf_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(net_conf_list, COLOR_DIALOG_TEXT);
        net_conf_choice = activateCDKScroll(net_conf_list, 0);

        /* Check exit from widget */
        if (net_conf_list->exitType == vNORMAL) {
            if (net_conf_choice == 0) {
                /* User chose the "general" network setting option */
                *general_opt = TRUE;
            } else {
                /* User chose to configure an interface */
                *general_opt = FALSE;
                if (net_if_name[net_conf_choice] != NULL)
                    snprintf(iface_name, MISC_STRING_LEN, "%s",
                        net_if_name[net_conf_choice]);
                if (net_if_mac[net_conf_choice] != NULL)
                    snprintf(iface_mac, MISC_STRING_LEN, "%s",
                        net_if_mac[net_conf_choice]);
                if (net_if_speed[net_conf_choice] != NULL)
                    snprintf(iface_speed, MISC_STRING_LEN, "%s",
                        net_if_speed[net_conf_choice]);
                if (net_if_duplex[net_conf_choice] != NULL)
                    snprintf(iface_duplex, MISC_STRING_LEN, "%s",
                        net_if_duplex[net_conf_choice]);
                *iface_bonding = net_if_bonding[net_conf_choice];
                *iface_bridge = net_if_bridge[net_conf_choice];
            }
        }
        break;
    }

    /* Done */
    destroyCDKScroll(net_conf_list);
    refreshCDKScreen(cdk_screen);
    for (i = 0; i < MAX_NET_IFACE; i++) {
        FREE_NULL(net_scroll_msg[i]);
        FREE_NULL(net_if_name[i]);
        FREE_NULL(net_if_mac[i]);
        FREE_NULL(net_if_speed[i]);
        FREE_NULL(net_if_duplex[i]);
    }
    return;
}
