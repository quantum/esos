/*
 * $Id: menu_actions.c 140 2012-07-25 15:39:08Z marc.smith $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <cdk/cdkscreen.h>

#include "menu_actions.h"
#include "main.h"

/*
 * Generic error dialog; takes an error message and displays a nice red
 * "ERROR" box to the user. Accepts mutliple lines; caller should pass NULL
 * for lines that shouldn't be set.
 */
void errorDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *error = 0;
    static char *buttons[] = {"</B>   OK   "};
    char *message[6] = {NULL};
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
    for (i = 0; i < 6; i++)
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
    char *message[6] = {NULL};
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
    for (i = 0; i < 6; i++)
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
    char *error = NULL;
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
            asprintf(&error, "opendir: %s", strerror(errno));
            errorDialog(cdk_screen, error, NULL);
            freeChar(error);
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
    char *error = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups",
            SYSFS_SCST_TGT, tgt_driver, tgt_name);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&error, "opendir: %s", strerror(errno));
        errorDialog(cdk_screen, error, NULL);
        freeChar(error);
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
                asprintf(&scroll_grp_list[j], "<C>%s", scst_tgt_groups[j]);
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
    char *error = NULL, *scroll_title = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            link_path[MAX_SYSFS_PATH_SIZE] = {0},
            dev_path[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/luns",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, tgt_group);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&error, "opendir: %s", strerror(errno));
        errorDialog(cdk_screen, error, NULL);
        freeChar(error);
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
    char *error = NULL;
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Went with the static char array method from this article:
     * http://www.eskimo.com/~scs/cclass/int/sx5.html
     * Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    /* Open the directory to get SCSI devices */
    if ((dir_stream = opendir(SYSFS_SCSI_DISK)) == NULL) {
        asprintf(&error, "opendir: %s", strerror(errno));
        errorDialog(cdk_screen, error, NULL);
        freeChar(error);
        goto cleanup;
    }

    /* Loop over each entry in the directory (SCSI devices) */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        if (dir_entry->d_type == DT_LNK) {
            asprintf(&scsi_dsk_dev[dev_cnt], "%s", dir_entry->d_name);
            dev_cnt++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Loop over our list of SCSI devices */
    for (i = 0; i < dev_cnt; i++) {
        /* Get the SCSI block device node */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/device/block", SYSFS_SCSI_DISK, scsi_dsk_dev[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            asprintf(&scsi_dsk_node[i], "opendir: %s", strerror(errno));
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
    char *error = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Loop over each SCST handler type and grab any open device names */
    j = 0;
    for (i = 0; i < 6; i++) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s",
                SYSFS_SCST_TGT, handlers[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            asprintf(&error, "opendir: %s", strerror(errno));
            errorDialog(cdk_screen, error, NULL);
            freeChar(error);
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
void getSCSTInitChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[], char initiator[]) {
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
        asprintf(&error_msg, "opendir: %s", strerror(errno));
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
            asprintf(&scroll_init_list[i], "<C>%s", init_list[i]);
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
    char scstadmin_cmd[100] = {0}, sync_conf_cmd[100] = {0};
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
    snprintf(scstadmin_cmd, 100, "%s -nonkey -write_config %s > /dev/null 2>&1", SCSTADMIN_TOOL, SCST_CONF);
    ret_val = system(scstadmin_cmd);
    if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
        asprintf(&error_msg, "Running %s failed; exited with %d.", SCSTADMIN_TOOL, exit_stat);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    
    /* Synchronize the ESOS configuration */
    snprintf(sync_conf_cmd, 100, "%s > /dev/null 2>&1", SYNC_CONF_TOOL);
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
    struct group* group_info = NULL;
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
        asprintf(&scroll_list[user_cnt], "<C>%s", *grp_member);
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
 * mutliple lines; caller should pass NULL for lines that shouldn't be set.
 */
boolean questionDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2) {
    CDKDIALOG *question = 0;
    static char *buttons[] = {"</B>   Yes   ", "</B>   No   "};
    char *message[6] = {NULL};
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
    for (i = 0; i < 6; i++)
        freeChar(message[i]);
    refreshCDKScreen(screen);
    return ret_val;
}