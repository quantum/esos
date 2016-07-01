/**
 * @file menu_alua.c
 * @brief Contains the menu actions for the 'ALUA' menu.
 * @author Copyright (c) 2012-2016 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Device/Target Group Layout" dialog.
 */
void devTgtGrpLayoutDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *alua_info = 0;
    char *scroll_title = NULL;
    char *swindow_info[MAX_ALUA_LAYOUT_LINES] = {NULL};
    int i = 0, line_pos = 0;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    DIR *dev_grp_dir_stream = NULL, *tgt_grp_dir_stream = NULL,
            *tgt_dir_stream = NULL, *dev_dir_stream = NULL;
    struct dirent *dev_grp_dir_entry = NULL, *tgt_grp_dir_entry = NULL,
            *tgt_dir_entry = NULL, *dev_dir_entry = NULL;

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&scroll_title,
            "<C></%d/B>SCST ALUA Device/Target Group Layout\n",
            g_color_dialog_title[g_curr_theme]);
    alua_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (ALUA_LAYOUT_ROWS + 2), (ALUA_LAYOUT_COLS + 2), scroll_title,
            MAX_ALUA_LAYOUT_LINES, TRUE, FALSE);
    if (!alua_info) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(alua_info, g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(alua_info, g_color_dialog_box[g_curr_theme]);

    line_pos = 0;
    while (1) {
        /* We'll start with the SCST device groups */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups", SYSFS_SCST_TGT);
        if ((dev_grp_dir_stream = opendir(dir_name)) == NULL) {
            if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "opendir(): %s",
                        strerror(errno));
                line_pos++;
            }
            break;
        }
        while ((dev_grp_dir_entry = readdir(dev_grp_dir_stream)) != NULL) {
            /* The device group names are directories; skip '.' and '..' */
            if ((dev_grp_dir_entry->d_type == DT_DIR) &&
                    (strcmp(dev_grp_dir_entry->d_name, ".") != 0) &&
                    (strcmp(dev_grp_dir_entry->d_name, "..") != 0)) {
                if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                    SAFE_ASPRINTF(&swindow_info[line_pos],
                            "</B>Device Group:<!B> %s",
                            dev_grp_dir_entry->d_name);
                    line_pos++;
                }

                /* Now get all of the device names for this device group */
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                        "%s/device_groups/%s/devices",
                        SYSFS_SCST_TGT, dev_grp_dir_entry->d_name);
                if ((dev_dir_stream = opendir(dir_name)) == NULL) {
                    if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                        SAFE_ASPRINTF(&swindow_info[line_pos],
                                "opendir(): %s", strerror(errno));
                        line_pos++;
                    }
                    break;
                }
                while ((dev_dir_entry = readdir(dev_dir_stream)) != NULL) {
                    /* The device names are links */
                    if (dev_dir_entry->d_type == DT_LNK) {
                        if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                            SAFE_ASPRINTF(&swindow_info[line_pos],
                                    "\t</B>Device:<!B> %s",
                                    dev_dir_entry->d_name);
                            line_pos++;
                        }
                    }
                }
                closedir(dev_dir_stream);

                /* Now get all of the target groups for this device group */
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                        "%s/device_groups/%s/target_groups", SYSFS_SCST_TGT,
                        dev_grp_dir_entry->d_name);
                if ((tgt_grp_dir_stream = opendir(dir_name)) == NULL) {
                    if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                        SAFE_ASPRINTF(&swindow_info[line_pos], "opendir(): %s",
                                strerror(errno));
                        line_pos++;
                    }
                    break;
                }
                while ((tgt_grp_dir_entry = readdir(tgt_grp_dir_stream)) !=
                        NULL) {
                    /* The target group names are directories;
                     * skip '.' and '..' */
                    if ((tgt_grp_dir_entry->d_type == DT_DIR) &&
                            (strcmp(tgt_grp_dir_entry->d_name, ".") != 0) &&
                            (strcmp(tgt_grp_dir_entry->d_name, "..") != 0)) {
                        if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                            snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                            "%s/device_groups/%s/target_groups/%s/group_id",
                            SYSFS_SCST_TGT, dev_grp_dir_entry->d_name,
                                    tgt_grp_dir_entry->d_name);
                            readAttribute(dir_name, tmp_buff);
                            SAFE_ASPRINTF(&swindow_info[line_pos],
                                    "\t</B>Target Group:<!B> %s (Group ID: %s)",
                                    tgt_grp_dir_entry->d_name, tmp_buff);
                            line_pos++;
                        }

                        /* Loop over each target name for the
                         * current target group */
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/device_groups/%s/target_groups/%s",
                                SYSFS_SCST_TGT, dev_grp_dir_entry->d_name,
                                tgt_grp_dir_entry->d_name);
                        if ((tgt_dir_stream = opendir(dir_name)) == NULL) {
                            if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                                SAFE_ASPRINTF(&swindow_info[line_pos],
                                        "opendir(): %s", strerror(errno));
                                line_pos++;
                            }
                            break;
                        }
                        while ((tgt_dir_entry = readdir(tgt_dir_stream)) !=
                                NULL) {
                            /* The target names are links (if local),
                             * or directories (if remote); skip '.' and '..' */
                            if (((tgt_dir_entry->d_type == DT_DIR) &&
                                    (strcmp(tgt_dir_entry->d_name,
                                    ".") != 0) &&
                                    (strcmp(tgt_dir_entry->d_name,
                                    "..") != 0)) ||
                                    (tgt_dir_entry->d_type == DT_LNK)) {
                                if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                                    snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                            "%s/device_groups/%s/"
                                            "target_groups/%s/%s/rel_tgt_id",
                                            SYSFS_SCST_TGT,
                                            dev_grp_dir_entry->d_name,
                                            tgt_grp_dir_entry->d_name,
                                            tgt_dir_entry->d_name);
                                    readAttribute(dir_name, tmp_buff);
                                    SAFE_ASPRINTF(&swindow_info[line_pos],
                                            "\t\t</B>Target:<!B> %s "
                                            "(Rel Tgt ID: %s)",
                                            tgt_dir_entry->d_name, tmp_buff);
                                    line_pos++;
                                }
                            }
                        }
                        closedir(tgt_dir_stream);
                    }
                }
                closedir(tgt_grp_dir_stream);

                /* Print a blank line to separate targets */
                if (line_pos < MAX_ALUA_LAYOUT_LINES) {
                    SAFE_ASPRINTF(&swindow_info[line_pos], " ");
                    line_pos++;
                }
            }
        }
        closedir(dev_grp_dir_stream);
        break;
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_ALUA_LAYOUT_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], " ");
        line_pos++;
    }
    if (line_pos < MAX_ALUA_LAYOUT_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;
    }

    /* Set the scrolling window content */
    setCDKSwindowContents(alua_info, swindow_info, line_pos);

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(alua_info, 'g');
    activateCDKSwindow(alua_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(alua_info);

    /* Done */
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_ALUA_LAYOUT_LINES; i++)
        FREE_NULL(swindow_info[i]);
    return;
}


/**
 * @brief Run the "Add Device Group" dialog.
 */
void addDevGrpDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *dev_grp_name_entry = 0;
    char attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *entry_title = NULL, *dev_grp_name = NULL, *error_msg = NULL;
    int temp_int = 0;

    while (1) {
        /* Get new device group name (entry widget) */
        SAFE_ASPRINTF(&entry_title, "<C></%d/B>Add New Device Group\n",
                g_color_dialog_title[g_curr_theme]);
        dev_grp_name_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
                entry_title, "</B>New Group Name (no spaces): ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                SCST_DEV_GRP_NAME_LEN, 0, SCST_DEV_GRP_NAME_LEN, TRUE, FALSE);
        if (!dev_grp_name_entry) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(dev_grp_name_entry,
                g_color_dialog_box[g_curr_theme]);
        setCDKEntryBackgroundAttrib(dev_grp_name_entry,
                g_color_dialog_text[g_curr_theme]);

        /* Draw the entry widget */
        curs_set(1);
        dev_grp_name = activateCDKEntry(dev_grp_name_entry, 0);

        /* Check exit from widget */
        if (dev_grp_name_entry->exitType == vNORMAL) {
            /* Check group name for bad characters */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS, dev_grp_name))
                break;

            /* Add the new device group */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/device_groups/mgmt", SYSFS_SCST_TGT);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                    "create %s", dev_grp_name);
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't add SCST (ALUA) device group: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* Done */
    FREE_NULL(entry_title);
    destroyCDKEntry(dev_grp_name_entry);
    return;
}


/**
 * @brief Run the "Remove Device Group" dialog.
 */
void remDevGrpDialog(CDKSCREEN *main_cdk_screen) {
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg,
            "Are you sure you want to delete SCST device group '%s?'",
            dev_grp_name);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST device group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/mgmt", SYSFS_SCST_TGT);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", dev_grp_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg,
                    "Couldn't delete SCST (ALUA) device group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Target Group" dialog.
 */
void addTgtGrpDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *tgt_grp_window = 0;
    CDKSCREEN *tgt_grp_screen = 0;
    CDKLABEL *tgt_grp_info = 0;
    CDKENTRY *grp_name = 0;
    CDKSCALE *group_id = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    char *tgt_grp_info_msg[TGT_GRP_INFO_LINES] = {NULL};
    int tgt_grp_window_lines = 0, tgt_grp_window_cols = 0, window_y = 0,
            window_x = 0, temp_int = 0, i = 0, traverse_ret = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* New CDK screen */
    tgt_grp_window_lines = 10;
    tgt_grp_window_cols = 50;
    window_y = ((LINES / 2) - (tgt_grp_window_lines / 2));
    window_x = ((COLS / 2) - (tgt_grp_window_cols / 2));
    tgt_grp_window = newwin(tgt_grp_window_lines, tgt_grp_window_cols,
            window_y, window_x);
    if (tgt_grp_window == NULL) {
        errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
        return;
    }
    tgt_grp_screen = initCDKScreen(tgt_grp_window);
    if (tgt_grp_screen == NULL) {
        errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
        return;
    }
    boxWindow(tgt_grp_window, g_color_dialog_box[g_curr_theme]);
    wbkgd(tgt_grp_window, g_color_dialog_text[g_curr_theme]);
    wrefresh(tgt_grp_window);

    while (1) {
        /* Information label */
        SAFE_ASPRINTF(&tgt_grp_info_msg[0],
                "</%d/B>Adding new SCST target group...",
                g_color_dialog_title[g_curr_theme]);
        SAFE_ASPRINTF(&tgt_grp_info_msg[1], " ");
        SAFE_ASPRINTF(&tgt_grp_info_msg[2],
                "</B>Device group:<!B>\t%s", dev_grp_name);
        tgt_grp_info = newCDKLabel(tgt_grp_screen, (window_x + 1),
                (window_y + 1), tgt_grp_info_msg, TGT_GRP_INFO_LINES,
                FALSE, FALSE);
        if (!tgt_grp_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(tgt_grp_info,
                g_color_dialog_text[g_curr_theme]);

        /* Target group name (entry) */
        grp_name = newCDKEntry(tgt_grp_screen, (window_x + 1), (window_y + 5),
                NULL, "</B>Target Group Name: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                SCST_TGT_GRP_NAME_LEN, 0, SCST_TGT_GRP_NAME_LEN, FALSE, FALSE);
        if (!grp_name) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(grp_name, g_color_dialog_input[g_curr_theme]);

        /* Target group ID (scale) */
        group_id = newCDKScale(tgt_grp_screen, (window_x + 1), (window_y + 6),
                NULL, "</B>Target Group ID:  ",
                g_color_dialog_select[g_curr_theme], 7, 0, MIN_SCST_TGT_GRP_ID,
                MAX_SCST_TGT_GRP_ID, 1, 100, FALSE, FALSE);
        if (!group_id) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKScaleBackgroundAttrib(group_id,
                g_color_dialog_text[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(tgt_grp_screen, (window_x + 16),
                (window_y + 8), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(tgt_grp_screen, (window_x + 26),
                (window_y + 8), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(tgt_grp_screen);
        traverse_ret = traverseCDKScreen(tgt_grp_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check the entry field for bad characters */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                    getCDKEntryValue(grp_name)))
                break;

            /* Add the new target group */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/device_groups/%s/target_groups/mgmt",
                    SYSFS_SCST_TGT, dev_grp_name);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "create %s",
                    getCDKEntryValue(grp_name));
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't add SCST (ALUA) target group: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }

            /* Set the target group ID */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/device_groups/%s/target_groups/%s/group_id",
                    SYSFS_SCST_TGT, dev_grp_name, getCDKEntryValue(grp_name));
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "%d",
                    getCDKScaleValue(group_id));
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't set SCST (ALUA) target group ID: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    for (i = 0; i < TGT_GRP_INFO_LINES; i++)
        FREE_NULL(tgt_grp_info_msg[i]);
    if (tgt_grp_screen != NULL) {
        destroyCDKScreenObjects(tgt_grp_screen);
        destroyCDKScreen(tgt_grp_screen);
        delwin(tgt_grp_window);
    }
    return;
}


/**
 * @brief Run the "Remove Target Group" dialog.
 */
void remTgtGrpDialog(CDKSCREEN *main_cdk_screen) {
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    tgt_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Get target group choice from user (based on previously
     * selected device group) */
    getSCSTTgtGrpChoice(main_cdk_screen, dev_grp_name, tgt_grp_name);
    if (tgt_grp_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg,
            "Are you sure you want to delete SCST target group '%s?'",
            tgt_grp_name);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST target group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/target_groups/mgmt",
                SYSFS_SCST_TGT, dev_grp_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", tgt_grp_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg,
                    "Couldn't delete SCST (ALUA) target group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Device to Group" dialog.
 */
void addDevToGrpDialog(CDKSCREEN *main_cdk_screen) {
    char device_name[MAX_SYSFS_ATTR_SIZE] = {0},
    dev_handler[MAX_SYSFS_ATTR_SIZE] = {0},
    dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    int temp_int = 0;

    /* Have the user choose a SCST device */
    getSCSTDevChoice(main_cdk_screen, device_name, dev_handler);
    if (device_name[0] == '\0')
        return;

    /* Have the user choose a SCST device group (to add the device to) */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Add the SCST device to the device group (ALUA) */
    snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
            "%s/device_groups/%s/devices/mgmt",
            SYSFS_SCST_TGT, dev_grp_name);
    snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add %s", device_name);
    if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
        SAFE_ASPRINTF(&error_msg,
                "Couldn't add SCST (ALUA) device to device group: %s",
                strerror(temp_int));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Remove Device from Group" dialog.
 */
void remDevFromGrpDialog(CDKSCREEN *main_cdk_screen) {
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    device_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Get device choice from user (based on previously
     * selected device group) */
    getSCSTDevGrpDevChoice(main_cdk_screen, dev_grp_name, device_name);
    if (device_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg, "device '%s' from group '%s?'",
            device_name, dev_grp_name);
    confirm = confirmDialog(main_cdk_screen,
            "Are you sure you want to remove SCST", confirm_msg);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Remove the SCST device from the device group (ALUA) */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/devices/mgmt",
                SYSFS_SCST_TGT, dev_grp_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", device_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg,
                    "Couldn't delete SCST (ALUA) device from device group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Target to Group" dialog.
 */
void addTgtToGrpDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *add_tgt_window = 0;
    CDKSCREEN *add_tgt_screen = 0;
    CDKLABEL *add_tgt_info = 0;
    CDKENTRY *tgt_name = 0;
    CDKSCALE *rel_tgt_id = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    tgt_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    char *add_tgt_info_msg[TGT_GRP_INFO_LINES] = {NULL};
    int add_tgt_window_lines = 0, add_tgt_window_cols = 0, window_y = 0,
            window_x = 0, temp_int = 0, i = 0, traverse_ret = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Get target group choice from user (based on previously
     * selected device group) */
    getSCSTTgtGrpChoice(main_cdk_screen, dev_grp_name, tgt_grp_name);
    if (tgt_grp_name[0] == '\0')
        return;

    /* New CDK screen */
    add_tgt_window_lines = 11;
    add_tgt_window_cols = 55;
    window_y = ((LINES / 2) - (add_tgt_window_lines / 2));
    window_x = ((COLS / 2) - (add_tgt_window_cols / 2));
    add_tgt_window = newwin(add_tgt_window_lines, add_tgt_window_cols,
            window_y, window_x);
    if (add_tgt_window == NULL) {
        errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
        return;
    }
    add_tgt_screen = initCDKScreen(add_tgt_window);
    if (add_tgt_screen == NULL) {
        errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
        return;
    }
    boxWindow(add_tgt_window, g_color_dialog_box[g_curr_theme]);
    wbkgd(add_tgt_window, g_color_dialog_text[g_curr_theme]);
    wrefresh(add_tgt_window);

    while (1) {
        /* Information label */
        SAFE_ASPRINTF(&add_tgt_info_msg[0],
                "</%d/B>Adding SCST target to target group...",
                g_color_dialog_title[g_curr_theme]);
        SAFE_ASPRINTF(&add_tgt_info_msg[1], " ");
        SAFE_ASPRINTF(&add_tgt_info_msg[2],
                "</B>Device group:<!B>\t%s", dev_grp_name);
        SAFE_ASPRINTF(&add_tgt_info_msg[3],
                "</B>Target group:<!B>\t%s", tgt_grp_name);
        add_tgt_info = newCDKLabel(add_tgt_screen, (window_x + 1),
                (window_y + 1), add_tgt_info_msg, ADD_TGT_INFO_LINES,
                FALSE, FALSE);
        if (!add_tgt_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(add_tgt_info,
                g_color_dialog_text[g_curr_theme]);

        /* Target name (entry) */
        tgt_name = newCDKEntry(add_tgt_screen, (window_x + 1), (window_y + 6),
                NULL, "</B>Target Name:         ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                20, 0, SCST_TGT_NAME_LEN, FALSE, FALSE);
        if (!tgt_name) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(tgt_name, g_color_dialog_input[g_curr_theme]);

        /* Relative target ID (scale) */
        rel_tgt_id = newCDKScale(add_tgt_screen, (window_x + 1), (window_y + 7),
                NULL, "</B>Relative Target ID: ",
                g_color_dialog_select[g_curr_theme],
                7, 0, MIN_SCST_REL_TGT_ID, MAX_SCST_REL_TGT_ID,
                1, 100, FALSE, FALSE);
        if (!rel_tgt_id) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKScaleBackgroundAttrib(rel_tgt_id,
                g_color_dialog_text[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(add_tgt_screen, (window_x + 16),
                (window_y + 9), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(add_tgt_screen, (window_x + 26),
                (window_y + 9), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(add_tgt_screen);
        traverse_ret = traverseCDKScreen(add_tgt_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check the entry field for bad characters */
            if (!checkInputStr(main_cdk_screen, INIT_CHARS,
                    getCDKEntryValue(tgt_name)))
                break;

            /* Add the target to the target group */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/device_groups/%s/target_groups/%s/mgmt",
                    SYSFS_SCST_TGT, dev_grp_name, tgt_grp_name);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add %s",
                    getCDKEntryValue(tgt_name));
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't add SCST (ALUA) target to target group: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }

            /* Set the relative target ID */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/device_groups/%s/target_groups/%s/%s/rel_tgt_id",
                    SYSFS_SCST_TGT, dev_grp_name, tgt_grp_name,
                    getCDKEntryValue(tgt_name));
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "%d",
                    getCDKScaleValue(rel_tgt_id));
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg, SET_REL_TGT_ID_ERR,
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    for (i = 0; i < ADD_TGT_INFO_LINES; i++)
        FREE_NULL(add_tgt_info_msg[i]);
    if (add_tgt_screen != NULL) {
        destroyCDKScreenObjects(add_tgt_screen);
        destroyCDKScreen(add_tgt_screen);
        delwin(add_tgt_window);
    }
    return;
}


/**
 * @brief Run the "Remove Target from Group" dialog.
 */
void remTgtFromGrpDialog(CDKSCREEN *main_cdk_screen) {
    char dev_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    tgt_grp_name[MAX_SYSFS_ATTR_SIZE] = {0},
    target_name[MAX_SYSFS_ATTR_SIZE] = {0},
    attr_path[MAX_SYSFS_PATH_SIZE] = {0},
    attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST device group */
    getSCSTDevGrpChoice(main_cdk_screen, dev_grp_name);
    if (dev_grp_name[0] == '\0')
        return;

    /* Get target group choice from user (based on previously
     * selected device group) */
    getSCSTTgtGrpChoice(main_cdk_screen, dev_grp_name, tgt_grp_name);
    if (tgt_grp_name[0] == '\0')
        return;

    /* Get target group choice from user (based on previously
     * selected device group) */
    getSCSTTgtGrpTgtChoice(main_cdk_screen, dev_grp_name, tgt_grp_name,
            target_name);
    if (target_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg, "target '%s' from group '%s?'",
            target_name, tgt_grp_name);
    confirm = confirmDialog(main_cdk_screen,
            "Are you sure you want to remove SCST", confirm_msg);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Remove the SCST target from the target group (ALUA) */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/device_groups/%s/target_groups/%s/mgmt",
                SYSFS_SCST_TGT, dev_grp_name, tgt_grp_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", target_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg,
                    "Couldn't delete SCST (ALUA) target from target group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}
