/**
 * @file menu_devices.c
 * @brief Contains the menu actions for the 'Devices' menu.
 * @author Copyright (c) 2012-2016 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <syslog.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Device Information" dialog.
 */
void devInfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *dev_info = 0;
    char scst_dev[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_hndlr[MAX_SYSFS_ATTR_SIZE] = {0},
            dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char *swindow_info[MAX_DEV_INFO_LINES] = {NULL};
    char *swindow_title = NULL;
    int i = 0, line_cnt = 0;

    /* Have the user choose a SCST device */
    getSCSTDevChoice(main_cdk_screen, scst_dev, scst_hndlr);
    if (scst_dev[0] == '\0' || scst_hndlr[0] == '\0')
        return;

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&swindow_title, "<C></%d/B>SCST Device Information\n",
            g_color_dialog_title[g_curr_theme]);
    dev_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (DEV_INFO_ROWS + 2), (DEV_INFO_COLS + 2), swindow_title,
            MAX_DEV_INFO_LINES, TRUE, FALSE);
    if (!dev_info) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(dev_info, g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(dev_info, g_color_dialog_box[g_curr_theme]);

    /* Add device information (common attributes) */
    SAFE_ASPRINTF(&swindow_info[0], "</B>Device Name:<!B>\t\t%s", scst_dev);
    SAFE_ASPRINTF(&swindow_info[1], "</B>Device Handler:<!B>\t\t%s",
            scst_hndlr);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/threads_num",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    SAFE_ASPRINTF(&swindow_info[2], "</B>Number of Threads:<!B>\t%s", tmp_buff);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/handlers/%s/%s/threads_pool_type",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    SAFE_ASPRINTF(&swindow_info[3], "</B>Threads Pool Type:<!B>\t%s", tmp_buff);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/type",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    SAFE_ASPRINTF(&swindow_info[4], "</B>SCSI Type:<!B>\t\t%s", tmp_buff);
    SAFE_ASPRINTF(&swindow_info[5], " ");
    line_cnt = 6;

    /* Some extra attributes for certain device handlers */
    if (strcmp(scst_hndlr, "vcdrom") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        line_cnt = 7;

    } else if (strcmp(scst_hndlr, "vdisk_blockio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[7], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/nv_cache",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[8], "</B>NV Cache:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[9], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[10], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[11], "</B>Rotational:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/handlers/%s/%s/write_through",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[12], "</B>Write Through:<!B>\t%s",
                tmp_buff);
        line_cnt = 13;

    } else if (strcmp(scst_hndlr, "vdisk_fileio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[7], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/nv_cache",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[8], "</B>NV Cache:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[9], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[10], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[11], "</B>Rotational:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/handlers/%s/%s/write_through",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[12], "</B>Write Through:<!B>\t%s",
                tmp_buff);
        line_cnt = 13;

    } else if (strcmp(scst_hndlr, "vdisk_nullio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[6], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[7], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[8], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        SAFE_ASPRINTF(&swindow_info[9], "</B>Rotational:<!B>\t%s", tmp_buff);
        line_cnt = 10;
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_cnt < MAX_DEV_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_cnt], " ");
        line_cnt++;
    }
    if (line_cnt < MAX_DEV_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_cnt], CONTINUE_MSG);
        line_cnt++;
    }

    /* Set the scrolling window content */
    setCDKSwindowContents(dev_info, swindow_info, line_cnt);

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(dev_info, 'g');
    activateCDKSwindow(dev_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(dev_info);

    /* Done */
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_DEV_INFO_LINES; i++ ) {
        FREE_NULL(swindow_info[i]);
    }
    return;
}


/**
 * @brief Run the "Add Device" dialog.
 */
void addDeviceDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCROLL *dev_list = 0, *fio_type_list = 0;
    CDKFSELECT *file_select = 0;
    CDKSCREEN *dev_screen = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKLABEL *dev_info = 0;
    CDKENTRY *dev_name_field = 0;
    CDKITEMLIST *block_size = 0;
    CDKRADIO *write_through = 0, *nv_cache = 0, *read_only = 0,
            *removable = 0, *rotational = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    WINDOW *dev_window = 0;
    char attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            fileio_file[MAX_SYSFS_PATH_SIZE] = {0},
            fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0};
    char *scsi_disk = NULL, *scsi_chgr = NULL, *scsi_tape = NULL,
            *error_msg = NULL, *selected_file = NULL, *iso_file_name = NULL,
            *block_dev = NULL, *scroll_title = NULL, *fselect_title = NULL;
    char *dev_info_msg[ADD_DEV_INFO_LINES] = {NULL};
    int dev_window_lines = 0, dev_window_cols = 0, window_y = 0, window_x = 0,
            dev_choice = 0, temp_int = 0, i = 0, traverse_ret = 0,
            fio_type_choice = 0;
    boolean mounted = FALSE;

    /* Prompt for new device type */
    SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a SCST Device Handler\n",
            g_color_dialog_title[g_curr_theme]);
    dev_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 15, 30,
            scroll_title, g_scst_dev_types, g_scst_dev_types_size(), FALSE,
            g_color_dialog_select[g_curr_theme], TRUE, FALSE);
    if (!dev_list) {
        errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
        return;
    }
    setCDKScrollBoxAttribute(dev_list, g_color_dialog_box[g_curr_theme]);
    setCDKScrollBackgroundAttrib(dev_list, g_color_dialog_text[g_curr_theme]);
    dev_choice = activateCDKScroll(dev_list, 0);

    if (dev_list->exitType == vESCAPE_HIT) {
        /* We're done */
        destroyCDKScroll(dev_list);
        refreshCDKScreen(main_cdk_screen);
        FREE_NULL(scroll_title);
        return;

    } else if (dev_list->exitType == vNORMAL) {
        /* User made a choice, continue on */
        destroyCDKScroll(dev_list);
        refreshCDKScreen(main_cdk_screen);
        FREE_NULL(scroll_title);
    }

    /* Populate the CDK screen based on the handler */
    switch (dev_choice) {

        /* dev_disk */
        case 0:
            /* Get disk choice from user */
            if ((scsi_disk = getSCSIDiskChoice(main_cdk_screen)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/dev_disk/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", scsi_disk);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* dev_disk_perf */
        case 1:
            /* Get disk choice from user */
            if ((scsi_disk = getSCSIDiskChoice(main_cdk_screen)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/dev_disk_perf/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", scsi_disk);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* vcdrom */
        case 2:
            /* Create the file selector widget */
            SAFE_ASPRINTF(&fselect_title, "<C></%d/B>Choose an ISO File\n",
                    g_color_dialog_title[g_curr_theme]);
            file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER,
                    20, 40, fselect_title, "File: ",
                    g_color_dialog_input[g_curr_theme], '_' |
                    g_color_dialog_input[g_curr_theme],
                    A_REVERSE, "</N>", "</B>", "</N>", "</N>", TRUE, FALSE);
            if (!file_select) {
                errorDialog(main_cdk_screen, FSELECT_ERR_MSG, NULL);
                break;
            }
            setCDKFselectBoxAttribute(file_select,
                    g_color_dialog_box[g_curr_theme]);
            setCDKFselectBackgroundAttrib(file_select,
                    g_color_dialog_text[g_curr_theme]);
            setCDKFselectDirectory(file_select, "/");
            /* Activate the widget and let the user choose a file */
            selected_file = activateCDKFselect(file_select, 0);
            if (file_select->exitType == vESCAPE_HIT) {
                destroyCDKFselect(file_select);
                refreshCDKScreen(main_cdk_screen);
                break;
            } else if (file_select->exitType == vNORMAL) {
                /* Add the new virtual CDROM device (sysfs) */
                iso_file_name = strrchr(selected_file, '/') + 1;
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/vcdrom/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", iso_file_name);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                } else {
                    /* The device has been created, now set the ISO file */
                    snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                            "%s/handlers/vcdrom/%s/filename",
                            SYSFS_SCST_TGT, iso_file_name);
                    snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                            "%s", selected_file);
                    if ((temp_int = writeAttribute(attr_path,
                            attr_value)) != 0) {
                        SAFE_ASPRINTF(&error_msg,
                                "Couldn't update SCST device parameters: %s",
                                strerror(temp_int));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                    }
                }
                destroyCDKFselect(file_select);
                refreshCDKScreen(main_cdk_screen);
            }
            break;

        /* vdisk_blockio */
        case 3:
            /* Get block device choice from user */
            if ((block_dev = getBlockDevChoice(main_cdk_screen)) == NULL)
                break;

            /* New CDK screen */
            dev_window_lines = 18;
            dev_window_cols = 60;
            window_y = ((LINES / 2) - (dev_window_lines / 2));
            window_x = ((COLS / 2) - (dev_window_cols / 2));
            dev_window = newwin(dev_window_lines, dev_window_cols,
                    window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
                break;
            }
            boxWindow(dev_window, g_color_dialog_box[g_curr_theme]);
            wbkgd(dev_window, g_color_dialog_text[g_curr_theme]);
            wrefresh(dev_window);

            /* Information label */
            SAFE_ASPRINTF(&dev_info_msg[0],
                    "</%d/B>Adding new vdisk_blockio SCST device...",
                    g_color_dialog_title[g_curr_theme]);
            SAFE_ASPRINTF(&dev_info_msg[1], " ");
            SAFE_ASPRINTF(&dev_info_msg[2],
                    "</B>SCSI Block Device:<!B> %.35s", block_dev);
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 3, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(dev_info,
                    g_color_dialog_text[g_curr_theme]);

            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1),
                    (window_y + 5), NULL, "</B>SCST Device Name (no spaces): ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(dev_name_field,
                    g_color_dialog_input[g_curr_theme]);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen,
                    (window_x + 1), (window_y + 7),
                    "</B>Block Size", NULL, g_scst_bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, ITEM_LIST_ERR_MSG, NULL);
                break;
            }
            setCDKItemlistBackgroundAttrib(block_size,
                    g_color_dialog_text[g_curr_theme]);

            /* Write through widget (radio) */
            write_through = newCDKRadio(dev_screen, (window_x + 1),
                    (window_y + 11), NONE, 3, 10, "</B>Write Through",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!write_through) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(write_through,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(write_through, 0);

            /* NV cache widget (radio) */
            nv_cache = newCDKRadio(dev_screen, (window_x + 18), (window_y + 7),
                    NONE, 3, 10, "</B>NV Cache", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!nv_cache) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(nv_cache,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(nv_cache, 0);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 18),
                    (window_y + 11), NONE, 3, 10, "</B>Read Only",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(read_only,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 32), (window_y + 7),
                    NONE, 3, 10, "</B>Removable", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(removable,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(removable, 0);

            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 32),
                    (window_y + 11), NONE, 3, 10, "</B>Rotational",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(rotational,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 21),
                    (window_y + 16), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(ok_button,
                    g_color_dialog_input[g_curr_theme]);
            cancel_button = newCDKButton(dev_screen, (window_x + 31),
                    (window_y + 16), g_ok_cancel_msg[1], cancel_cb,
                    FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(cancel_button,
                    g_color_dialog_input[g_curr_theme]);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                        getCDKEntryValue(dev_name_field)))
                    break;

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/vdisk_blockio/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s filename=%s; blocksize=%s; "
                        "write_through=%d; nv_cache=%d; read_only=%d; "
                        "removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field), block_dev,
                        g_scst_bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(write_through),
                        getCDKRadioSelectedItem(nv_cache),
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* vdisk_fileio */
        case 4:
            /* Choose block device or file on a file system */
            SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a Back-End Type\n",
                    g_color_dialog_title[g_curr_theme]);
            fio_type_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE,
                    8, 26, scroll_title, g_fio_types, 2,
                    FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
            if (!fio_type_list) {
                errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
                break;
            }
            setCDKScrollBoxAttribute(fio_type_list,
                    g_color_dialog_box[g_curr_theme]);
            setCDKScrollBackgroundAttrib(fio_type_list,
                    g_color_dialog_text[g_curr_theme]);
            fio_type_choice = activateCDKScroll(fio_type_list, 0);

            /* Check exit from widget */
            if (fio_type_list->exitType == vESCAPE_HIT) {
                destroyCDKScroll(fio_type_list);
                refreshCDKScreen(main_cdk_screen);
                break;
            } else if (fio_type_list->exitType == vNORMAL) {
                destroyCDKScroll(fio_type_list);
                refreshCDKScreen(main_cdk_screen);

                if (fio_type_choice == 0) {
                    /* Have the user select a file system */
                    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type,
                            &mounted);
                    if (fs_name[0] == '\0')
                        return;

                    if (!mounted) {
                        errorDialog(main_cdk_screen,
                                "The selected file system is not mounted!",
                                NULL);
                        break;
                    }
                    /* Create the file selector widget */
                    SAFE_ASPRINTF(&fselect_title,
                            "<C></%d/B>Choose a Back-End File\n",
                            g_color_dialog_title[g_curr_theme]);
                    file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER,
                            20, 40, fselect_title, "File: ",
                            g_color_dialog_input[g_curr_theme], '_' |
                            g_color_dialog_input[g_curr_theme],
                            A_REVERSE, "</N>", "</B>", "</N>", "</N>",
                            TRUE, FALSE);
                    if (!file_select) {
                        errorDialog(main_cdk_screen, FSELECT_ERR_MSG, NULL);
                        break;
                    }
                    setCDKFselectBoxAttribute(file_select,
                            g_color_dialog_box[g_curr_theme]);
                    setCDKFselectBackgroundAttrib(file_select,
                            g_color_dialog_text[g_curr_theme]);
                    setCDKFselectDirectory(file_select, fs_path);
                    /* Activate the widget and let the user choose a file */
                    selected_file = activateCDKFselect(file_select, 0);
                    if (file_select->exitType == vESCAPE_HIT) {
                        destroyCDKFselect(file_select);
                        refreshCDKScreen(main_cdk_screen);
                        break;
                    } else if (file_select->exitType == vNORMAL) {
                        strncpy(fileio_file, selected_file,
                                MAX_SYSFS_PATH_SIZE);
                        destroyCDKFselect(file_select);
                        refreshCDKScreen(main_cdk_screen);
                    }

                } else {
                    /* Get block device choice from user */
                    if ((block_dev =
                            getBlockDevChoice(main_cdk_screen)) == NULL)
                        break;
                    else
                        strncpy(fileio_file, block_dev, MAX_SYSFS_PATH_SIZE);
                }
            }

            /* New CDK screen */
            dev_window_lines = 18;
            dev_window_cols = 60;
            window_y = ((LINES / 2) - (dev_window_lines / 2));
            window_x = ((COLS / 2) - (dev_window_cols / 2));
            dev_window = newwin(dev_window_lines, dev_window_cols,
                    window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
                break;
            }
            boxWindow(dev_window, g_color_dialog_box[g_curr_theme]);
            wbkgd(dev_window, g_color_dialog_text[g_curr_theme]);
            wrefresh(dev_window);

            /* Information label */
            SAFE_ASPRINTF(&dev_info_msg[0],
                    "</%d/B>Adding new vdisk_fileio SCST device...",
                    g_color_dialog_title[g_curr_theme]);
            SAFE_ASPRINTF(&dev_info_msg[1], " ");
            SAFE_ASPRINTF(&dev_info_msg[2],
                    "</B>Back-End File/Device:<!B> %.35s", fileio_file);
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 3, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(dev_info,
                    g_color_dialog_text[g_curr_theme]);

            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1),
                    (window_y + 5), NULL, "</B>SCST Device Name (no spaces): ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(dev_name_field,
                    g_color_dialog_input[g_curr_theme]);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen, (window_x + 1),
                    (window_y + 7), "</B>Block Size", NULL,
                    g_scst_bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, ITEM_LIST_ERR_MSG, NULL);
                break;
            }
            setCDKItemlistBackgroundAttrib(block_size,
                    g_color_dialog_text[g_curr_theme]);

            /* Write through widget (radio) */
            write_through = newCDKRadio(dev_screen, (window_x + 1),
                    (window_y + 11), NONE, 3, 10, "</B>Write Through",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!write_through) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(write_through,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(write_through, 0);

            /* NV cache widget (radio) */
            nv_cache = newCDKRadio(dev_screen, (window_x + 18), (window_y + 7),
                    NONE, 3, 10, "</B>NV Cache", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!nv_cache) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(nv_cache,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(nv_cache, 0);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 18),
                    (window_y + 11), NONE, 3, 10, "</B>Read Only",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(read_only,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 32), (window_y + 7),
                    NONE, 3, 10, "</B>Removable", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(removable,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(removable, 0);

            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 32),
                    (window_y + 11), NONE, 3, 10, "</B>Rotational",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(rotational,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 21),
                    (window_y + 16), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(ok_button,
                    g_color_dialog_input[g_curr_theme]);
            cancel_button = newCDKButton(dev_screen, (window_x + 31),
                    (window_y + 16), g_ok_cancel_msg[1], cancel_cb,
                    FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(cancel_button,
                    g_color_dialog_input[g_curr_theme]);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                        getCDKEntryValue(dev_name_field)))
                    break;

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/vdisk_fileio/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s filename=%s; blocksize=%s; "
                        "write_through=%d; nv_cache=%d; read_only=%d; "
                        "removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field), fileio_file,
                        g_scst_bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(write_through),
                        getCDKRadioSelectedItem(nv_cache),
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* vdisk_nullio */
        case 5:
            /* New CDK screen */
            dev_window_lines = 18;
            dev_window_cols = 50;
            window_y = ((LINES / 2) - (dev_window_lines / 2));
            window_x = ((COLS / 2) - (dev_window_cols / 2));
            dev_window = newwin(dev_window_lines, dev_window_cols,
                    window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
                break;
            }
            boxWindow(dev_window, g_color_dialog_box[g_curr_theme]);
            wbkgd(dev_window, g_color_dialog_text[g_curr_theme]);
            wrefresh(dev_window);

            /* Information label */
            SAFE_ASPRINTF(&dev_info_msg[0],
                    "</%d/B>Adding new vdisk_nullio SCST device...",
                    g_color_dialog_title[g_curr_theme]);
            SAFE_ASPRINTF(&dev_info_msg[1], " ");
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 2, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                break;
            }
            setCDKLabelBackgroundAttrib(dev_info,
                    g_color_dialog_text[g_curr_theme]);

            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1),
                    (window_y + 3), NULL, "</B>SCST Device Name (no spaces): ",
                    g_color_dialog_select[g_curr_theme],
                    '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
                break;
            }
            setCDKEntryBoxAttribute(dev_name_field,
                    g_color_dialog_input[g_curr_theme]);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen, (window_x + 1),
                    (window_y + 5), "</B>Block Size", NULL,
                    g_scst_bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, ITEM_LIST_ERR_MSG, NULL);
                break;
            }
            setCDKItemlistBackgroundAttrib(block_size,
                    g_color_dialog_text[g_curr_theme]);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 1), (window_y + 9),
                    NONE, 3, 10, "</B>Read Only", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(read_only,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 18), (window_y + 5),
                    NONE, 3, 10, "</B>Removable", g_no_yes_opts, 2,
                    '#' | g_color_dialog_select[g_curr_theme], 1,
                    g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(removable,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(removable, 0);

            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 18),
                    (window_y + 9), NONE, 3, 10, "</B>Rotational",
                    g_no_yes_opts, 2, '#' | g_color_dialog_select[g_curr_theme],
                    1, g_color_dialog_select[g_curr_theme], FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
                break;
            }
            setCDKRadioBackgroundAttrib(rotational,
                    g_color_dialog_text[g_curr_theme]);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 16),
                    (window_y + 16), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(ok_button,
                    g_color_dialog_input[g_curr_theme]);
            cancel_button = newCDKButton(dev_screen, (window_x + 26),
                    (window_y + 16), g_ok_cancel_msg[1], cancel_cb,
                    FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
                break;
            }
            setCDKButtonBackgroundAttrib(cancel_button,
                    g_color_dialog_input[g_curr_theme]);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                        getCDKEntryValue(dev_name_field)))
                    break;

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/vdisk_nullio/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s blocksize=%s; read_only=%d; "
                        "removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field),
                        g_scst_bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* dev_changer */
        case 6:
            /* Get changer choice from user */
            if ((scsi_chgr = getSCSIDevChoice(main_cdk_screen,
                    SCSI_CHANGER_TYPE)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/dev_changer/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", scsi_chgr);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* dev_tape */
        case 7:
            /* Get tape choice from user */
            if ((scsi_tape = getSCSIDevChoice(main_cdk_screen,
                    SCSI_TAPE_TYPE)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/dev_tape/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", scsi_tape);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        /* dev_tape_perf */
        case 8:
            /* Get disk choice from user */
            if ((scsi_tape = getSCSIDevChoice(main_cdk_screen,
                    SCSI_TAPE_TYPE)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/handlers/dev_tape_perf/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s", scsi_tape);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't add SCST device: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
            }
            break;

        default:
            DEBUG_LOG(DEFAULT_CASE_HIT);
            break;
    }

    /* All done */
    for (i = 0; i < ADD_DEV_INFO_LINES; i++)
        FREE_NULL(dev_info_msg[i]);
    if (dev_screen != NULL) {
        destroyCDKScreenObjects(dev_screen);
        destroyCDKScreen(dev_screen);
        delwin(dev_window);
    }
    FREE_NULL(scroll_title);
    FREE_NULL(fselect_title);
    /* Using the file selector widget changes the CWD -- fix it */
    if ((chdir(getenv("HOME"))) == -1) {
        SAFE_ASPRINTF(&error_msg, "chdir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    }
    return;
}


/**
 * @brief Run the "Delete Device" dialog.
 */
void remDeviceDialog(CDKSCREEN *main_cdk_screen) {
    char scst_dev[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_hndlr[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST device to delete */
    getSCSTDevChoice(main_cdk_screen, scst_dev, scst_hndlr);
    if (scst_dev[0] == '\0' || scst_hndlr[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg, "SCST device '%s' (%s)?", scst_dev, scst_hndlr);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you want to delete",
            confirm_msg);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST device */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/mgmt",
                SYSFS_SCST_TGT, scst_hndlr);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del_device %s", scst_dev);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Couldn't delete SCST device: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Map to Group" dialog.
 */
void mapDeviceDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCREEN *map_screen = 0;
    CDKRADIO *read_only = 0;
    CDKSCALE *lun = 0;
    CDKLABEL *map_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    WINDOW *map_window = 0;
    char scst_dev[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_hndlr[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            group_name[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    char *map_info_msg[MAP_DEV_INFO_LINES] = {NULL};
    int map_window_lines = 0, map_window_cols = 0, window_y = 0, window_x = 0,
            temp_int = 0, i = 0, traverse_ret = 0;

    /* Have the user choose a SCST device to map */
    getSCSTDevChoice(main_cdk_screen, scst_dev, scst_hndlr);
    if (scst_dev[0] == '\0' || scst_hndlr[0] == '\0')
        return;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    /* New CDK screen */
    map_window_lines = 18;
    map_window_cols = 50;
    window_y = ((LINES / 2) - (map_window_lines / 2));
    window_x = ((COLS / 2) - (map_window_cols / 2));
    map_window = newwin(map_window_lines, map_window_cols, window_y, window_x);
    if (map_window == NULL) {
        errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
        return;
    }
    map_screen = initCDKScreen(map_window);
    if (map_screen == NULL) {
        errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
        return;
    }
    boxWindow(map_window, g_color_dialog_box[g_curr_theme]);
    wbkgd(map_window, g_color_dialog_text[g_curr_theme]);
    wrefresh(map_window);

    while (1) {
        /* Information label */
        SAFE_ASPRINTF(&map_info_msg[0], "</%d/B>Mapping SCST device...",
                g_color_dialog_title[g_curr_theme]);
        SAFE_ASPRINTF(&map_info_msg[1], " ");
        SAFE_ASPRINTF(&map_info_msg[2], "</B>Device:<!B>\t\t%s", scst_dev);
        SAFE_ASPRINTF(&map_info_msg[3], "</B>Handler:<!B>\t%s", scst_hndlr);
        SAFE_ASPRINTF(&map_info_msg[4], " ");
        SAFE_ASPRINTF(&map_info_msg[5], "</B>Target:<!B>\t%s", scst_tgt);
        SAFE_ASPRINTF(&map_info_msg[6], "</B>Driver:<!B>\t%s", tgt_driver);
        SAFE_ASPRINTF(&map_info_msg[7], "</B>Group:<!B>\t%s", group_name);
        map_info = newCDKLabel(map_screen, (window_x + 1), (window_y + 1),
                map_info_msg, MAP_DEV_INFO_LINES, FALSE, FALSE);
        if (!map_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(map_info,
                g_color_dialog_text[g_curr_theme]);

        /* LUN widget (scale) */
        lun = newCDKScale(map_screen, (window_x + 1), (window_y + 10),
                NULL, "</B>Logical Unit Number (LUN)",
                g_color_dialog_select[g_curr_theme],
                5, 0, MIN_SCST_LUN_VAL, MAX_SCST_LUN_VAL, 1, 10, FALSE, FALSE);
        if (!lun) {
            errorDialog(main_cdk_screen, SCALE_ERR_MSG, NULL);
            break;
        }
        setCDKScaleBackgroundAttrib(lun, g_color_dialog_text[g_curr_theme]);

        /* Read only widget (radio) */
        read_only = newCDKRadio(map_screen, (window_x + 1), (window_y + 12),
                NONE, 3, 10, "</B>Read Only", g_no_yes_opts, 2,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!read_only) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(read_only,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(read_only, 0);

        /* Buttons */
        ok_button = newCDKButton(map_screen, (window_x + 16), (window_y + 16),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(map_screen, (window_x + 26),
                (window_y + 16), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(map_screen);
        traverse_ret = traverseCDKScreen(map_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Add the new LUN (map device) */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/targets/%s/%s/ini_groups/%s/luns/mgmt",
                    SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add %s %d read_only=%d",
                    scst_dev, getCDKScaleValue(lun),
                    getCDKRadioSelectedItem(read_only));
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg, "Couldn't map SCST device: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* All done */
    for (i = 0; i < MAP_DEV_INFO_LINES; i++)
        FREE_NULL(map_info_msg[i]);
    if (map_screen != NULL) {
        destroyCDKScreenObjects(map_screen);
        destroyCDKScreen(map_screen);
        delwin(map_window);
    }
    return;
}


/**
 * @brief Run the "Unmap from Group" dialog.
 */
void unmapDeviceDialog(CDKSCREEN *main_cdk_screen) {
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            group_name[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    int temp_int = 0, lun = 0;
    boolean confirm = FALSE;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    /* Now the user picks a LUN */
    lun = getSCSTLUNChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (lun == -1)
        return;

    /* Get a final confirmation from user before removing the LUN mapping */
    SAFE_ASPRINTF(&confirm_msg, "SCST LUN %d from group '%s'?", lun,
            group_name);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you want to unmap",
            confirm_msg);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Remove the specified SCST LUN */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/luns/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %d", lun);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Couldn't remove SCST LUN: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* All done */
    return;
}


/**
 * @brief Run the "LUN/Group Layout" dialog.
 */
void lunLayoutDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *lun_info = 0;
    char *swindow_title = NULL;
    char *swindow_info[MAX_LUN_LAYOUT_LINES] = {NULL};
    int i = 0, line_pos = 0, dev_path_size = 0, driver_cnt = 0;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            link_path[MAX_SYSFS_PATH_SIZE] = {0},
            dev_path[MAX_SYSFS_PATH_SIZE] = {0};
    char tgt_drivers[MAX_SCST_DRIVERS][MISC_STRING_LEN] = {{0}, {0}};
    DIR *tgt_dir_stream = NULL, *group_dir_stream = NULL,
            *init_dir_stream = NULL, *lun_dir_stream = NULL;
    struct dirent *tgt_dir_entry = NULL, *group_dir_entry = NULL,
            *init_dir_entry = NULL, *lun_dir_entry = NULL;

    /* Fill the array with current SCST target drivers */
    if (!listSCSTTgtDrivers(tgt_drivers, &driver_cnt)) {
        errorDialog(main_cdk_screen, TGT_DRIVERS_ERR, NULL);
        return;
    }

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&swindow_title, "<C></%d/B>SCST LUN/Group Layout\n",
            g_color_dialog_title[g_curr_theme]);
    lun_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (LUN_LAYOUT_ROWS + 2), (LUN_LAYOUT_COLS + 2),
            swindow_title, MAX_LUN_LAYOUT_LINES, TRUE, FALSE);
    if (!lun_info) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(lun_info, g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(lun_info, g_color_dialog_box[g_curr_theme]);

    /* Loop over each target driver type */
    line_pos = 0;
    for (i = 0; i < driver_cnt; i++) {
        /* Loop over each target for current driver type */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s",
                SYSFS_SCST_TGT, tgt_drivers[i]);
        if ((tgt_dir_stream = opendir(dir_name)) == NULL) {
            if (line_pos < MAX_LUN_LAYOUT_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "opendir(): %s",
                        strerror(errno));
                line_pos++;
            }
            break;
        }
        while ((tgt_dir_entry = readdir(tgt_dir_stream)) != NULL) {
            /* The target names are directories; skip '.' and '..' */
            if ((tgt_dir_entry->d_type == DT_DIR) &&
                    (strcmp(tgt_dir_entry->d_name, ".") != 0) &&
                    (strcmp(tgt_dir_entry->d_name, "..") != 0)) {
                if (line_pos < MAX_LUN_LAYOUT_LINES) {
                    SAFE_ASPRINTF(&swindow_info[line_pos],
                            "</B>Target:<!B> %s (%s)",
                            tgt_dir_entry->d_name, tgt_drivers[i]);
                    line_pos++;
                }
                /* Loop over each security group for the current target */
                snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/ini_groups", SYSFS_SCST_TGT,
                        tgt_drivers[i], tgt_dir_entry->d_name);
                if ((group_dir_stream = opendir(dir_name)) == NULL) {
                    if (line_pos < MAX_LUN_LAYOUT_LINES) {
                        SAFE_ASPRINTF(&swindow_info[line_pos], "opendir(): %s",
                                strerror(errno));
                        line_pos++;
                    }
                    break;
                }
                while ((group_dir_entry = readdir(group_dir_stream)) != NULL) {
                    /* The group names are directories; skip '.' and '..' */
                    if ((group_dir_entry->d_type == DT_DIR) &&
                            (strcmp(group_dir_entry->d_name, ".") != 0) &&
                            (strcmp(group_dir_entry->d_name, "..") != 0)) {
                        if (line_pos < MAX_LUN_LAYOUT_LINES) {
                            SAFE_ASPRINTF(&swindow_info[line_pos],
                                    "\t</B>Group:<!B> %s",
                                    group_dir_entry->d_name);
                            line_pos++;
                        }

                        /* Loop over each initiator for the current group */
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/targets/%s/%s/ini_groups/%s/initiators",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name, group_dir_entry->d_name);
                        if ((init_dir_stream = opendir(
                                dir_name)) == NULL) {
                            if (line_pos < MAX_LUN_LAYOUT_LINES) {
                                SAFE_ASPRINTF(&swindow_info[line_pos],
                                        "opendir(): %s", strerror(errno));
                                line_pos++;
                            }
                            break;
                        }
                        while ((init_dir_entry = readdir(init_dir_stream)) !=
                                NULL) {
                            /* The initiators are files; skip 'mgmt' */
                            if ((init_dir_entry->d_type == DT_REG) &&
                                    (strcmp(init_dir_entry->d_name,
                                    "mgmt") != 0)) {
                                if (line_pos < MAX_LUN_LAYOUT_LINES) {
                                    SAFE_ASPRINTF(&swindow_info[line_pos],
                                            "\t\t</B>Initiator:<!B> %s",
                                            init_dir_entry->d_name);
                                    line_pos++;
                                }
                            }
                        }
                        closedir(init_dir_stream);

                        /* Loop over each LUN for the current group */
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/targets/%s/%s/ini_groups/%s/luns",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name, group_dir_entry->d_name);
                        if ((lun_dir_stream = opendir(
                                dir_name)) == NULL) {
                            if (line_pos < MAX_LUN_LAYOUT_LINES) {
                                SAFE_ASPRINTF(&swindow_info[line_pos],
                                        "opendir(): %s", strerror(errno));
                                line_pos++;
                            }
                            break;
                        }
                        while ((lun_dir_entry = readdir(lun_dir_stream)) !=
                                NULL) {
                            /* The LUNs are directories; skip '.' and '..' */
                            if ((lun_dir_entry->d_type == DT_DIR) &&
                                    (strcmp(lun_dir_entry->d_name,
                                    ".") != 0) &&
                                    (strcmp(lun_dir_entry->d_name,
                                    "..") != 0)) {
                                /* We need to get the device name (link) */
                                snprintf(link_path, MAX_SYSFS_PATH_SIZE,
                                        "%s/targets/%s/%s/ini_groups"
                                        "/%s/luns/%s/device", SYSFS_SCST_TGT,
                                        tgt_drivers[i], tgt_dir_entry->d_name,
                                        group_dir_entry->d_name,
                                        lun_dir_entry->d_name);
                                /* Read the link to get device name
                                 * (doesn't append null byte) */
                                dev_path_size = readlink(link_path, dev_path,
                                        MAX_SYSFS_PATH_SIZE);
                                if (dev_path_size < MAX_SYSFS_PATH_SIZE)
                                    *(dev_path + dev_path_size) = '\0';
                                if (line_pos < MAX_LUN_LAYOUT_LINES) {
                                    SAFE_ASPRINTF(&swindow_info[line_pos],
                                            "\t\t</B>LUN:<!B> %s (%s)",
                                            lun_dir_entry->d_name,
                                            (strrchr(dev_path, '/') + 1));
                                    line_pos++;
                                }
                            }
                        }
                        closedir(lun_dir_stream);
                    }
                }
                closedir(group_dir_stream);
                /* Print a blank line to separate targets */
                if (line_pos < MAX_LUN_LAYOUT_LINES) {
                    SAFE_ASPRINTF(&swindow_info[line_pos], " ");
                    line_pos++;
                }
            }
        }
        closedir(tgt_dir_stream);
    }


    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_LUN_LAYOUT_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], " ");
        line_pos++;
    }
    if (line_pos < MAX_LUN_LAYOUT_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;
    }

    /* Set the scrolling window content */
    setCDKSwindowContents(lun_info, swindow_info, line_pos);

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(lun_info, 'g');
    activateCDKSwindow(lun_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(lun_info);

    /* Done */
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_LUN_LAYOUT_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}
