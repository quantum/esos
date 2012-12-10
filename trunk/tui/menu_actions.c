/*
 * $Id$
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

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"

/*
 * Generic error dialog; takes an error message and displays a nice red
 * "ERROR" box to the user. Accepts mutliple lines; caller should pass NULL
 * for lines that shouldn't be set.
 */
void errorDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *error = 0;
    static char *buttons[] = {"</B>   OK   "};
    char *message[ERROR_DIAG_MSG_SIZE] = {NULL};
    int i = 0;

    /* Set the message */
    asprintf(&message[0], "<C></B>ERROR");
    asprintf(&message[1], " ");
    if (msg_line_1)
        asprintf(&message[2], "<C>%s", msg_line_1);
    else
        asprintf(&message[2], " ");
    if (msg_line_2)
         asprintf(&message[3], "<C>%s", msg_line_2);
    else
        asprintf(&message[3], " ");
    asprintf(&message[4], " ");
    asprintf(&message[5], " ");

    /* Display the error dialog box */
    error = newCDKDialog(screen, CENTER, CENTER, message, 6, buttons, 1,
            COLOR_ERROR_SELECT, TRUE, TRUE, FALSE);
    if (error) {
        setCDKDialogBackgroundAttrib(error, COLOR_ERROR_TEXT);
        setCDKDialogBoxAttribute(error, COLOR_ERROR_BOX);
        activateCDKDialog(error, 0);
        destroyCDKDialog(error);
    }
    for (i = 0; i < ERROR_DIAG_MSG_SIZE; i++)
        freeChar(message[i]);
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
 * used as a last check for possibly dangerous operations. Accepts mutliple
 * lines; caller should pass NULL for lines that shouldn't be set.
 */
boolean confirmDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *confirm = 0;
    static char *buttons[] = {"</B>   OK   ", "</B> Cancel "};
    char *message[CONFIRM_DIAG_MSG_SIZE] = {NULL};
    int selection = 0, i = 0;
    boolean ret_val = FALSE;

    /* Set the message */
    asprintf(&message[0], "<C></B>CONFIRM");
    asprintf(&message[1], " ");
    if (msg_line_1)
        asprintf(&message[2], "<C>%s", msg_line_1);
    else
        asprintf(&message[2], " ");
    if (msg_line_2)
         asprintf(&message[3], "<C>%s", msg_line_2);
    else
        asprintf(&message[3], " ");
    asprintf(&message[4], " ");
    asprintf(&message[5], " ");

    /* Display the confirmation dialog box */
    confirm = newCDKDialog(screen, CENTER, CENTER, message, 6, buttons, 2,
            COLOR_ERROR_SELECT, TRUE, TRUE, FALSE);
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
        freeChar(message[i]);
    refreshCDKScreen(screen);
    return ret_val;
}


/*
 * Present the user with a list of SCST targets and let them choose one. We
 * then fill the char arrays with the target name and target driver.
 * Optionally, if tgt_driver is set, then we will only display targets for
 * that driver.
 */
void getSCSTTgtChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[]) {
    CDKSCROLL *scst_tgt_list = 0;
    int tgt_choice = 0, i = 0, j = 0, k = 0, driver_cnt = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_tgt_name[MAX_SCST_TGTS] = {NULL},
            *scst_tgt_driver[MAX_SCST_TGTS] = {NULL},
            *scst_tgt_info[MAX_SCST_TGTS] = {NULL},
            *drivers[MAX_SCST_DRIVERS] = {NULL};
    char *error_msg = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    if (tgt_driver[0] != '\0') {
        /* If a driver is given, then we only want targets for that driver */
        driver_cnt = 1;
        asprintf(&drivers[0], "%s", tgt_driver);
    } else {
        /* No driver was given, so we provide all of them */
        driver_cnt = 3;
        asprintf(&drivers[0], "ib_srpt");
        asprintf(&drivers[1], "iscsi");
        asprintf(&drivers[2], "qla2x00t");
    }

    /* Loop over each SCST target driver and get targets */
    j = 0;
    for (i = 0; i < driver_cnt; i++) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s",
                SYSFS_SCST_TGT, drivers[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            asprintf(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }

        /* Loop over each entry in the directory */
        k = 0;
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The target names are directories; skip '.' and '..' */
            if (dir_entry->d_type == DT_DIR) {
                if (k > 1) {
                    asprintf(&scst_tgt_name[j], "%s", dir_entry->d_name);
                    asprintf(&scst_tgt_driver[j], "%s", drivers[i]);
                    asprintf(&scst_tgt_info[j], "<C>%s (Driver: %s)", dir_entry->d_name, drivers[i]);
                    j++;
                }
                k++;
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);
    }

    /* Make sure we actually have something to present */
    if (j == 0) {
        errorDialog(cdk_screen, "No targets found!", NULL);
        goto cleanup;
    }

    /* Get SCST target choice from user */
    scst_tgt_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 60,
            "<C></31/B>Choose a SCST target:\n", scst_tgt_info, j,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!scst_tgt_list) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(scst_tgt_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(scst_tgt_list, COLOR_DIALOG_TEXT);
    tgt_choice = activateCDKScroll(scst_tgt_list, 0);

    /* Check exit from widget */
    if (scst_tgt_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(scst_tgt_list);
        refreshCDKScreen(cdk_screen);

    } else if (scst_tgt_list->exitType == vNORMAL) {
        destroyCDKScroll(scst_tgt_list);
        refreshCDKScreen(cdk_screen);
        strncpy(tgt_name, scst_tgt_name[tgt_choice], MAX_SYSFS_ATTR_SIZE);
        /* We only set the target driver if we weren't given an initial value */
        if (tgt_driver[0] == '\0')
            strncpy(tgt_driver, scst_tgt_driver[tgt_choice], MAX_SYSFS_ATTR_SIZE);
    }

    /* Done */
    cleanup:
    for (i = 0; i < MAX_SCST_TGTS; i++) {
        freeChar(scst_tgt_name[i]);
        freeChar(scst_tgt_driver[i]);
        freeChar(scst_tgt_info[i]);
    }
    for (i = 0; i < MAX_SCST_DRIVERS; i++) {
        freeChar(drivers[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST groups and let them choose one. The
 * driver / target name combination is passed in and we then fill the
 * char array with the selected group name.
 */
void getSCSTGroupChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[]) {
    CDKSCROLL *scst_grp_list = 0;
    int group_choice = 0, i = 0, j = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_tgt_groups[MAX_SCST_GROUPS] = {NULL},
            *scroll_grp_list[MAX_SCST_GROUPS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups",
            SYSFS_SCST_TGT, tgt_driver, tgt_name);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Loop over each entry in the directory */
    i = 0;
    j = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The group names are directories; skip '.' and '..' */
        if (dir_entry->d_type == DT_DIR) {
            if (i > 1) {
                asprintf(&scst_tgt_groups[j], "%s", dir_entry->d_name);
                asprintf(&scroll_grp_list[j], "<C>%.30s", scst_tgt_groups[j]);
                j++;
            }
            i++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Make sure we actually have something to present */
    if (j == 0) {
        errorDialog(cdk_screen, "No groups found!", NULL);
        goto cleanup;
    }

    /* Get SCST target group choice from user */
    asprintf(&scroll_title, "<C></31/B>Choose a group (%s):\n", tgt_name);
    scst_grp_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
            scroll_title, scroll_grp_list, j,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!scst_grp_list) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(scst_grp_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(scst_grp_list, COLOR_DIALOG_TEXT);
    group_choice = activateCDKScroll(scst_grp_list, 0);

    /* Check exit from widget */
    if (scst_grp_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(scst_grp_list);
        refreshCDKScreen(cdk_screen);

    } else if (scst_grp_list->exitType == vNORMAL) {
        destroyCDKScroll(scst_grp_list);
        refreshCDKScreen(cdk_screen);
        strncpy(tgt_group, scst_tgt_groups[group_choice], MAX_SYSFS_ATTR_SIZE);
    }

    /* Done */
    cleanup:
    freeChar(scroll_title);
    for (i = 0; i < MAX_SCST_GROUPS; i++) {
        freeChar(scst_tgt_groups[i]);
        freeChar(scroll_grp_list[i]);
    }
    return;
}


/*
 * Present the user with a list of SCST LUNs and let them choose one. The
 * driver / target / group name combination is passed in and we return
 * the LUN as an int; return -1 if there was an error or escape.
 */
int getSCSTLUNChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[]) {
    CDKSCROLL *lun_scroll = 0;
    int lun_choice = 0, i = 0, j = 0, ret_val = 0;
    int luns[MAX_SCST_LUNS] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_lun_list[MAX_SCST_LUNS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            link_path[MAX_SYSFS_PATH_SIZE] = {0},
            dev_path[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/luns",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        ret_val = -1;
        goto cleanup;
    }

    /* Loop over each entry in the directory */
    i = j = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The LUNs are directories; skip '.' and '..' */
        if (dir_entry->d_type == DT_DIR) {
            if (i > 1) {
                luns[j] = atoi(dir_entry->d_name);
                /* We need to get the device name (link) */
                snprintf(link_path, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/ini_groups/%s/luns/%d/device",
                        SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group, luns[j]);
                readlink(link_path, dev_path, MAX_SYSFS_PATH_SIZE);
                /* For our scroll widget */
                asprintf(&scst_lun_list[j], "<C>LUN %d (Device: %s)", luns[j],
                        (strrchr(dev_path, '/') + 1));
                j++;
            }
            i++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Make sure we actually have something to present */
    if (j == 0) {
        errorDialog(cdk_screen, "No LUNs found!", NULL);
        ret_val = -1;
        goto cleanup;
    }

    /* Get SCST LUN choice from user */
    asprintf(&scroll_title, "<C></31/B>Choose a LUN (%s):\n",
            tgt_group);
    lun_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 45,
            scroll_title, scst_lun_list, j,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!lun_scroll) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        ret_val = -1;
        goto cleanup;
    }
    setCDKScrollBoxAttribute(lun_scroll, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(lun_scroll, COLOR_DIALOG_TEXT);
    lun_choice = activateCDKScroll(lun_scroll, 0);

    /* Check exit from widget */
    if (lun_scroll->exitType == vESCAPE_HIT) {
        destroyCDKScroll(lun_scroll);
        refreshCDKScreen(cdk_screen);
        ret_val = -1;

    } else if (lun_scroll->exitType == vNORMAL) {
        destroyCDKScroll(lun_scroll);
        refreshCDKScreen(cdk_screen);
        ret_val = luns[lun_choice];
    }

    /* Done */
    cleanup:
    freeChar(scroll_title);
    for (i = 0; i < MAX_SCST_LUNS; i++) {
        freeChar(scst_lun_list[i]);
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
    static char *list_title = "<C></31/B>Choose a SCSI disk:\n";
    char *error_msg = NULL, *boot_dev_node = NULL;
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Went with the static char array method from this article:
     * http://www.eskimo.com/~scs/cclass/int/sx5.html
     * Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    /* Get the ESOS boot device node */
    // TODO: This needs to be tested -- does it return NULL even if nothing was found?
    if ((boot_dev_node = blkid_get_devname(NULL, "LABEL", ESOS_BOOT_PART)) == NULL) {
        errorDialog(cdk_screen, "Calling blkid_get_devname() failed.", NULL);
        goto cleanup;
    }

    /* Open the directory to get SCSI devices */
    if ((dir_stream = opendir(SYSFS_SCSI_DISK)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Loop over each entry in the directory (SCSI devices) */
    while (((dir_entry = readdir(dir_stream)) != NULL) && (dev_cnt < MAX_SCSI_DISKS)) {
        if (dir_entry->d_type == DT_LNK) {
            asprintf(&scsi_dsk_dev[dev_cnt], "%s", dir_entry->d_name);
            dev_cnt++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Loop over our list of SCSI devices */
    i = 0;
    while (i < dev_cnt) {
        /* Get the SCSI block device node */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/block", SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            asprintf(&scsi_dsk_node[i], "opendir(): %s", strerror(errno));
        } else {
            j = 0;
            while ((dir_entry = readdir(dir_stream)) != NULL) {
                if (dir_entry->d_type == DT_DIR) {
                    /* We want to skip the '.' and '..' directories */
                    if (j > 1) {
                        /* The first directory is the node name we're using */
                        asprintf(&scsi_dsk_node[i], "%s", dir_entry->d_name);
                        break;
                    }
                    j++;
                }
            }
            closedir(dir_stream);
        }
        /* Make sure this isn't the ESOS boot device (USB); if it is, clean-up 
         * anything allocated, shift the elements, and decrement device count */
        if ((strstr(boot_dev_node, scsi_dsk_node[i])) != NULL) {
            freeChar(scsi_dsk_node[i]);
            scsi_dsk_node[i] = NULL;
            freeChar(scsi_dsk_dev[i]);
            scsi_dsk_dev[i] = NULL;
            // TODO: This needs to be checked for safeness.
            for (j = i; j < dev_cnt; j++)
                scsi_dsk_dev[j] = scsi_dsk_dev[j+1];
            dev_cnt--;
            continue;
        }
        /* Read SCSI disk attributes */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/model",
                SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
        readAttribute(dir_name, tmp_buff);
        asprintf(&scsi_dsk_model[i], "%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/vendor",
                SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
        readAttribute(dir_name, tmp_buff);
        asprintf(&scsi_dsk_vendor[i], "%s", tmp_buff);
        /* Fill the list (pretty) for our CDK label with SCSI devices */
        asprintf(&scsi_dev_info[i], "<C>[%s] %s %s (/dev/%s)", scsi_dsk_dev[i],
                scsi_dsk_vendor[i], scsi_dsk_model[i], scsi_dsk_node[i]);
        /* Next */
        i++;
    }

    /* Make sure we actually have something to present */
    if (dev_cnt == 0) {
        errorDialog(cdk_screen, "No SCSI disks found!", NULL);
        goto cleanup;
    }

    /* Get SCSI disk choice from user */
    scsi_dsk_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
            list_title, scsi_dev_info, dev_cnt,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!scsi_dsk_list) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(scsi_dsk_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(scsi_dsk_list, COLOR_DIALOG_TEXT);
    disk_choice = activateCDKScroll(scsi_dsk_list, 0);

    /* Check exit from widget */
    if (scsi_dsk_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(scsi_dsk_list);
        refreshCDKScreen(cdk_screen);

    } else if (scsi_dsk_list->exitType == vNORMAL) {
        destroyCDKScroll(scsi_dsk_list);
        refreshCDKScreen(cdk_screen);
        strncpy(ret_buff, scsi_dsk_dev[disk_choice], MAX_SYSFS_ATTR_SIZE);
    }

    /* Done */
    cleanup:
    for (i = 0; i < MAX_SCSI_DISKS; i++) {
        freeChar(scsi_dsk_dev[i]);
        freeChar(scsi_dsk_node[i]);
        freeChar(scsi_dsk_model[i]);
        freeChar(scsi_dsk_vendor[i]);
        freeChar(scsi_dev_info[i]);
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
void getSCSTDevChoice(CDKSCREEN *cdk_screen, char dev_name[], char dev_handler[]) {
    CDKSCROLL *scst_dev_list = 0;
    int dev_choice = 0, i = 0, j = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    static char *handlers[] = {"dev_disk", "dev_disk_perf", "vcdrom",
        "vdisk_blockio", "vdisk_fileio", "vdisk_nullio"};
    char *scst_dev_name[MAX_SCST_DEVS] = {NULL},
        *scst_dev_hndlr[MAX_SCST_DEVS] = {NULL},
        *scst_dev_info[MAX_SCST_DEVS] = {NULL};
    char *error_msg = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Loop over each SCST handler type and grab any open device names */
    j = 0;
    for (i = 0; i < 6; i++) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s",
                SYSFS_SCST_TGT, handlers[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            asprintf(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK) {
                asprintf(&scst_dev_name[j], "%s", dir_entry->d_name);
                asprintf(&scst_dev_hndlr[j], "%s", handlers[i]);
                asprintf(&scst_dev_info[j], "<C>%s (Handler: %s)", dir_entry->d_name, handlers[i]);
                j++;
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);
    }

    /* Make sure we actually have something to present */
    if (j == 0) {
        errorDialog(cdk_screen, "No devices found!", NULL);
        goto cleanup;
    }

    /* Get SCSI disk choice from user */
    scst_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 55,
            "<C></31/B>Choose a SCST device:\n", scst_dev_info, j,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!scst_dev_list) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(scst_dev_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(scst_dev_list, COLOR_DIALOG_TEXT);
    dev_choice = activateCDKScroll(scst_dev_list, 0);

    /* Check exit from widget */
    if (scst_dev_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(scst_dev_list);
        refreshCDKScreen(cdk_screen);

    } else if (scst_dev_list->exitType == vNORMAL) {
        destroyCDKScroll(scst_dev_list);
        refreshCDKScreen(cdk_screen);
        strncpy(dev_name, scst_dev_name[dev_choice], MAX_SYSFS_ATTR_SIZE);
        strncpy(dev_handler, scst_dev_hndlr[dev_choice], MAX_SYSFS_ATTR_SIZE);
    }

    /* Done */
    cleanup:
    for (i = 0; i < MAX_SCST_DEVS; i++) {
        freeChar(scst_dev_name[i]);
        freeChar(scst_dev_hndlr[i]);
        freeChar(scst_dev_info[i]);
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
    static char *list_title = "<C></31/B>Choose an adapter:\n";
    char *error_msg = NULL;

    /* Get MegaRAID adapters */
    adp_count = getMRAdapterCount();
    if (adp_count == -1) {
        errorDialog(cdk_screen, "The MegaCLI tool isn't working (or is not installed).", NULL);
        return -1;
    } else if (adp_count == 0) {
        errorDialog(cdk_screen, "No adapters found!", NULL);
        return -1;
    } else {
        for (i = 0; i < adp_count; i++) {
            mr_adapters[i] = getMRAdapter(i);
            if (!mr_adapters[i]) {
                asprintf(&error_msg,
                        "Couldn't get data from MegaRAID adapter # %d!", i);
                errorDialog(cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                return -1;
            } else {
                asprintf(&adapters[i], "<C>MegaRAID Adapter # %d: %s", i,
                        mr_adapters[i]->prod_name);
            }
        }
    }

    /* Get adapter choice from user */
    adapter_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 8, 50,
            list_title, adapters, adp_count,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    setCDKScrollBoxAttribute(adapter_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(adapter_list, COLOR_DIALOG_TEXT);
    adp_choice = activateCDKScroll(adapter_list, 0);

    if (adapter_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(adapter_list);
        refreshCDKScreen(cdk_screen);
        return -1;

    } else if (adapter_list->exitType == vNORMAL) {
        destroyCDKScroll(adapter_list);
        refreshCDKScreen(cdk_screen);
    }

    /* Done */
    for (i = 0; i < MAX_ADAPTERS; i++) {
        freeChar(adapters[i]);
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

    /* Open the directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/initiators",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Loop over each entry in the directory */
    i = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The initiator names are files; skip 'mgmt' */
        if ((dir_entry->d_type == DT_REG) &&
                (strcmp(dir_entry->d_name, "mgmt") != 0)) {
            asprintf(&init_list[i], "%s", dir_entry->d_name);
            asprintf(&scroll_init_list[i], "<C>%.40s", init_list[i]);
            i++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Make sure we actually have something to present */
    if (i == 0) {
        errorDialog(cdk_screen, "No initiators found!", NULL);
        goto cleanup;
    }

    /* Get SCST initiator choice from user */
    asprintf(&scroll_title, "<C></31/B>Choose an initiator (%s):\n",
            tgt_group);
    lun_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 45,
            scroll_title, scroll_init_list, i,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!lun_scroll) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(lun_scroll, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(lun_scroll, COLOR_DIALOG_TEXT);
    lun_choice = activateCDKScroll(lun_scroll, 0);

    /* Check exit from widget */
    if (lun_scroll->exitType == vESCAPE_HIT) {
        destroyCDKScroll(lun_scroll);
        refreshCDKScreen(cdk_screen);

    } else if (lun_scroll->exitType == vNORMAL) {
        destroyCDKScroll(lun_scroll);
        refreshCDKScreen(cdk_screen);
        strncpy(initiator, init_list[lun_choice], MAX_SYSFS_ATTR_SIZE);
    }

    /* Done */
    cleanup:
    freeChar(scroll_title);
    for (i = 0; i < MAX_SCST_INITS; i++) {
        freeChar(init_list[i]);
        freeChar(scroll_init_list[i]);
    }
    return;
}


/*
 * Synchronize the ESOS configuration files to the USB drive; this will also
 * dump the SCST config. to a flat file.
 */
void syncConfig(CDKSCREEN *main_cdk_screen) {
    CDKLABEL *sync_msg = 0;
    char scstadmin_cmd[MAX_SHELL_CMD_LEN] = {0},
            sync_conf_cmd[MAX_SHELL_CMD_LEN] = {0};
    static char *message[] = {"", "",
                "</B>   Synchronizing ESOS configuration...   ", "", ""};
    char *error_msg = NULL;
    int ret_val = 0, exit_stat = 0;

    /* Display a nice short label message while we sync */
    sync_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
            message, 5, TRUE, FALSE);
    if (!sync_msg) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        return;
    }
    setCDKLabelBackgroundAttrib(sync_msg, COLOR_DIALOG_TEXT);
    setCDKLabelBoxAttribute(sync_msg, COLOR_DIALOG_BOX);
    refreshCDKScreen(main_cdk_screen);
    
    /* Dump the SCST configuration to a file */
    snprintf(scstadmin_cmd, MAX_SHELL_CMD_LEN, "%s -nonkey -write_config %s > /dev/null 2>&1", SCSTADMIN_TOOL, SCST_CONF);
    ret_val = system(scstadmin_cmd);
    if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
        asprintf(&error_msg, "Running %s failed; exited with %d.", SCSTADMIN_TOOL, exit_stat);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    
    /* Synchronize the ESOS configuration */
    snprintf(sync_conf_cmd, MAX_SHELL_CMD_LEN, "%s > /dev/null 2>&1", SYNC_CONF_TOOL);
    ret_val = system(sync_conf_cmd);
    if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
        asprintf(&error_msg, "Running %s failed; exited with %d.", SYNC_CONF_TOOL, exit_stat);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    
    /* Done */
    cleanup:
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
    char *esos_grp_members[MAX_USERS] = {NULL}, *scroll_list[MAX_USERS] = {NULL};
    int i = 0, user_cnt = 0, user_choice = 0;
    
    /* Get the specified ESOS group */
    group_info = getgrnam(ESOS_GROUP);
    if (!group_info) {
        errorDialog(cdk_screen, "Couldn't get ESOS group information!", NULL);
        return;
    }
    
    /* Add group members for scroll widget */
    user_cnt = 0;
    for (grp_member = group_info->gr_mem; *grp_member; grp_member++) {
        asprintf(&esos_grp_members[user_cnt], "%s", *grp_member);
        asprintf(&scroll_list[user_cnt], "<C>%.25s", *grp_member);
        user_cnt++;
    }

    /* Get user account choice */
    user_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 10, 30,
            "<C></31/B>Choose a user account:\n", scroll_list, user_cnt,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!user_scroll) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(user_scroll, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(user_scroll, COLOR_DIALOG_TEXT);
    user_choice = activateCDKScroll(user_scroll, 0);

    /* Check exit from widget */
    if (user_scroll->exitType == vESCAPE_HIT) {
        destroyCDKScroll(user_scroll);
        refreshCDKScreen(cdk_screen);
        /* User hit escape, so don't set anything for char array */

    } else if (user_scroll->exitType == vNORMAL) {
        destroyCDKScroll(user_scroll);
        refreshCDKScreen(cdk_screen);
        snprintf(user_acct, MAX_UNAME_LEN, "%s", esos_grp_members[user_choice]);
    }

    /* Done */
    cleanup:
    for (i = 0; i < user_cnt; i++) {
        freeChar(esos_grp_members[i]);
        freeChar(scroll_list[i]);
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
    static char *buttons[] = {"</B>   Yes   ", "</B>   No   "};
    char *message[QUEST_DIAG_MSG_SIZE] = {NULL};
    int selection = 0, i = 0;
    boolean ret_val = FALSE;

    /* Set the message */
    asprintf(&message[0], "<C></B>QUESTION");
    asprintf(&message[1], " ");
    if (msg_line_1)
        asprintf(&message[2], "<C>%s", msg_line_1);
    else
        asprintf(&message[2], " ");
    if (msg_line_2)
         asprintf(&message[3], "<C>%s", msg_line_2);
    else
        asprintf(&message[3], " ");
    asprintf(&message[4], " ");
    asprintf(&message[5], " ");

    /* Display the question dialog box */
    question = newCDKDialog(screen, CENTER, CENTER, message, 6, buttons, 2,
            COLOR_DIALOG_SELECT, TRUE, TRUE, FALSE);
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
        freeChar(message[i]);
    refreshCDKScreen(screen);
    return ret_val;
}


/*
 * Present a list of file systems from /etc/fstab and have the user pick one.
 */
void getFSChoice(CDKSCREEN *cdk_screen, char fs_name[], char fs_path[], char fs_type[], boolean *mounted) {
    CDKSCROLL *fs_scroll = 0;
    char *fs_names[MAX_FILE_SYSTEMS] = {NULL}, *fs_paths[MAX_FILE_SYSTEMS] = {NULL},
            *fs_types[MAX_FILE_SYSTEMS] = {NULL}, *scroll_list[MAX_USERS] = {NULL};
    char *error_msg = NULL;
    static char *no_touch = "/proc /sys /dev/pts /dev/shm /boot /mnt/root /mnt/conf /mnt/logs";
    char mnt_line_buffer[MAX_MNT_LINE_BUFFER] = {0};
    int i = 0, fs_cnt = 0, user_choice = 0, mnt_line_size = 0, mnt_dir_size = 0;
    boolean fs_mounted[MAX_FILE_SYSTEMS] = {FALSE};
    FILE *fstab_file = NULL, *mtab_file = NULL;
    struct mntent *fstab_entry = NULL, *mtab_entry = NULL;

    /* Make a list of file systems that are mounted (by mount path, not device) */
    if ((mtab_file = setmntent(MTAB, "r")) == NULL) {
        asprintf(&error_msg, "setmntent(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        return;
    }
    while ((mtab_entry = getmntent(mtab_file)) != NULL) {
        /* We add two extra: one for a space, and one for the null byte */
        mnt_dir_size = strlen(mtab_entry->mnt_dir) + 2;
        mnt_line_size = mnt_line_size + mnt_dir_size;
        // TODO: This totes needs to be tested (strcat)
        if (mnt_line_size >= MAX_MNT_LINE_BUFFER) {
            errorDialog(cdk_screen, "The maximum mount line buffer size has been reached!", NULL);
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
        asprintf(&error_msg, "setmntent(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        return;
    }
    
    /* Loop over fstab entries */
    fs_cnt = 0;
    while ((fstab_entry = getmntent(fstab_file)) != NULL) {
        /* We don't want to grab special entries from fstab that we shouldn't touch */
        if (strstr(no_touch, fstab_entry->mnt_dir) == NULL) {
            asprintf(&fs_names[fs_cnt], "%s", fstab_entry->mnt_fsname);
            asprintf(&fs_paths[fs_cnt], "%s", fstab_entry->mnt_dir);
            asprintf(&fs_types[fs_cnt], "%s", fstab_entry->mnt_type);
            if (strstr(mnt_line_buffer, fstab_entry->mnt_dir) == NULL)
                fs_mounted[fs_cnt] = FALSE;
            else
                fs_mounted[fs_cnt] = TRUE;
            asprintf(&scroll_list[fs_cnt], "<C>%-25.25s %-20.20s %-5.5s (Mounted: %d)", fs_names[fs_cnt],
                    fs_paths[fs_cnt], fs_types[fs_cnt], fs_mounted[fs_cnt]);
            fs_cnt++;
        }
    }

    /* Close up shop */
    endmntent(fstab_file);

    /* Make sure we actually have something to present */
    if (fs_cnt == 0) {
        errorDialog(cdk_screen, "No useful file systems were found!", NULL);
        goto cleanup;
    }

    /* Get file system choice */
    fs_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
            "<C></31/B>Choose a file system:\n", scroll_list, fs_cnt,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!fs_scroll) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(fs_scroll, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(fs_scroll, COLOR_DIALOG_TEXT);
    user_choice = activateCDKScroll(fs_scroll, 0);

    /* Check exit from widget */
    if (fs_scroll->exitType == vESCAPE_HIT) {
        destroyCDKScroll(fs_scroll);
        refreshCDKScreen(cdk_screen);
        /* User hit escape, so don't set anything for char array */

    } else if (fs_scroll->exitType == vNORMAL) {
        destroyCDKScroll(fs_scroll);
        refreshCDKScreen(cdk_screen);
        snprintf(fs_name, MAX_FS_ATTR_LEN, "%s", fs_names[user_choice]);
        snprintf(fs_path, MAX_FS_ATTR_LEN, "%s", fs_paths[user_choice]);
        snprintf(fs_type, MAX_FS_ATTR_LEN, "%s", fs_types[user_choice]);
        *mounted = fs_mounted[user_choice];
    }

    /* Done */
    cleanup:
    for (i = 0; i < fs_cnt; i++) {
        freeChar(fs_names[i]);
        freeChar(fs_paths[i]);
        freeChar(fs_types[i]);
        freeChar(scroll_list[i]);
    }
    return;
}


/*
 * Give the user a list of block devices (SCSI, etc.) and have them select one.
 * We return a char array with the "/dev/XXX" path for the chosen device.
 */
char *getBlockDevChoice(CDKSCREEN *cdk_screen) {
    CDKSCROLL *block_dev_list = 0;
    int blk_dev_choice = 0, i = 0, dev_cnt = 0;
    char *blk_dev_name[MAX_BLOCK_DEVS] = {NULL}, *blk_dev_info[MAX_BLOCK_DEVS] = {NULL},
            *blk_dev_size[MAX_BLOCK_DEVS] = {NULL}, *blk_dev_scroll_lines[MAX_BLOCK_DEVS] = {NULL};
    static char *list_title = "<C></31/B>Choose a block device:\n";
    char *error_msg = NULL, *boot_dev_node = NULL;
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Went with the static char array method from this article:
     * http://www.eskimo.com/~scs/cclass/int/sx5.html
     * Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    /* Get the ESOS boot device node */
    // TODO: This needs to be tested -- does it return NULL even if nothing was found?
    if ((boot_dev_node = blkid_get_devname(NULL, "LABEL", ESOS_BOOT_PART)) == NULL) {
        errorDialog(cdk_screen, "Calling blkid_get_devname() failed.", NULL);
        goto cleanup;
    }

    /* Open the directory to get block devices */
    if ((dir_stream = opendir(SYSFS_BLOCK)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Loop over each entry in the directory (block devices) */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        if (dir_entry->d_type == DT_LNK) {
            /* We don't want to show the ESOS boot block device (USB drive) */
            if (strstr(boot_dev_node, dir_entry->d_name) != NULL) {
                freeChar(boot_dev_node);
                continue;
            /* For DRBD block devices (not sure if the /dev/drbdX format is
             forced when using drbdadm, so this may be a problem */
            } else if ((strstr(dir_entry->d_name, "drbd")) != NULL) {
                asprintf(&blk_dev_name[dev_cnt], "%s", dir_entry->d_name);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                asprintf(&blk_dev_info[dev_cnt], "DRBD Device"); /* Nothing extra... yet */
                dev_cnt++;
            /* For software RAID (md) devices; it appears the mdadm tool forces
             the /dev/mdX device node name format */
            } else if ((strstr(dir_entry->d_name, "md")) != NULL) {
                asprintf(&blk_dev_name[dev_cnt], "%s", dir_entry->d_name);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/md/level",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_info[dev_cnt], "Level: %s", tmp_buff);
                dev_cnt++;
            /* For normal SCSI block devices */
            } else if ((strstr(dir_entry->d_name, "sd")) != NULL) {
                asprintf(&blk_dev_name[dev_cnt], "%s", dir_entry->d_name);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/model",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_info[dev_cnt], "Model: %s", tmp_buff);
                dev_cnt++;
            /* For device mapper (eg, LVM2) block devices */
            } else if ((strstr(dir_entry->d_name, "dm-")) != NULL) {
                asprintf(&blk_dev_name[dev_cnt], "%s", dir_entry->d_name);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_size[dev_cnt], "%s", tmp_buff);
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/dm/name",
                        SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                readAttribute(dir_name, tmp_buff);
                asprintf(&blk_dev_info[dev_cnt], "Name: %s", tmp_buff);
                dev_cnt++;
            }
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Make sure we actually have something to present */
    if (dev_cnt == 0) {
        errorDialog(cdk_screen, "No block devices found!", NULL);
        goto cleanup;
    }

    /* Fill the list (pretty) for our CDK label with block devices */
    for (i = 0; i < dev_cnt; i++) {
        asprintf(&blk_dev_scroll_lines[i], "<C>/dev/%-5.5s Size: %-12.12s %-30.30s",
                blk_dev_name[i], blk_dev_size[i], blk_dev_info[i]);
    }

    /* Get block device choice from user */
    block_dev_list = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 65,
            list_title, blk_dev_scroll_lines, dev_cnt,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!block_dev_list) {
        errorDialog(cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(block_dev_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(block_dev_list, COLOR_DIALOG_TEXT);
    blk_dev_choice = activateCDKScroll(block_dev_list, 0);

    /* Check exit from widget */
    if (block_dev_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(block_dev_list);
        refreshCDKScreen(cdk_screen);

    } else if (block_dev_list->exitType == vNORMAL) {
        destroyCDKScroll(block_dev_list);
        refreshCDKScreen(cdk_screen);
        snprintf(ret_buff, MAX_SYSFS_ATTR_SIZE, "/dev/%s", blk_dev_name[blk_dev_choice]);
    }

    /* Done */
    cleanup:
    for (i = 0; i < MAX_BLOCK_DEVS; i++) {
        freeChar(blk_dev_name[i]);
        freeChar(blk_dev_info[i]);
        freeChar(blk_dev_size[i]);
        freeChar(blk_dev_scroll_lines[i]);
    }
    if (ret_buff[0] != '\0')
        return ret_buff;
    else
        return NULL;
}
