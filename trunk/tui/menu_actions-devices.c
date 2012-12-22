/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"

/*
 * Run the Device Information dialog
 */
void devInfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *dev_info = 0;
    char scst_dev[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_hndlr[MAX_SYSFS_ATTR_SIZE] = {0},
            dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char *swindow_info[MAX_DEV_INFO_LINES] = {NULL};
    int i = 0, line_cnt = 0;
    
    /* Have the user choose a SCST device */
    getSCSTDevChoice(main_cdk_screen, scst_dev, scst_hndlr);
    if (scst_dev[0] == '\0' || scst_hndlr[0] == '\0')
        return;
    
    /* Setup scrolling window widget */
    dev_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            DEV_INFO_ROWS+2, DEV_INFO_COLS+2,
            "<C></31/B>SCST Device Information:\n",
            MAX_DEV_INFO_LINES, TRUE, FALSE);
    if (!dev_info) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(dev_info, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(dev_info, COLOR_DIALOG_BOX);

    /* Add device information (common attributes) */
    asprintf(&swindow_info[0], "</B>Device Name:<!B>\t\t%s", scst_dev);
    asprintf(&swindow_info[1], "</B>Device Handler:<!B>\t\t%s", scst_hndlr);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/threads_num",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    asprintf(&swindow_info[2], "</B>Number of Threads:<!B>\t%s", tmp_buff);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/threads_pool_type",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    asprintf(&swindow_info[3], "</B>Threads Pool Type:<!B>\t%s", tmp_buff);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/type",
            SYSFS_SCST_TGT, scst_hndlr, scst_dev);
    readAttribute(dir_name, tmp_buff);
    asprintf(&swindow_info[4], "</B>SCSI Type:<!B>\t\t%s", tmp_buff);
    asprintf(&swindow_info[5], " ");
    line_cnt = 6;

    /* Some extra attributes for certain device handlers */
    if (strcmp(scst_hndlr, "vcdrom") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        line_cnt = 7;
        
    } else if (strcmp(scst_hndlr, "vdisk_blockio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[7], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/nv_cache",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[8], "</B>NV Cache:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[9], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[10], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[11], "</B>Rotational:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/write_through",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[12], "</B>Write Through:<!B>\t%s", tmp_buff);
        line_cnt = 13;
        
    } else if (strcmp(scst_hndlr, "vdisk_fileio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/filename",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[6], "</B>Filename:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[7], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/nv_cache",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[8], "</B>NV Cache:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[9], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[10], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[11], "</B>Rotational:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/write_through",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[12], "</B>Write Through:<!B>\t%s", tmp_buff);
        line_cnt = 13;
        
    } else if (strcmp(scst_hndlr, "vdisk_nullio") == 0) {
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/blocksize",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[6], "</B>Block Size:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/read_only",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[7], "</B>Read Only:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/removable",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[8], "</B>Removable:<!B>\t%s", tmp_buff);
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/%s/rotational",
                SYSFS_SCST_TGT, scst_hndlr, scst_dev);
        readAttribute(dir_name, tmp_buff);
        asprintf(&swindow_info[9], "</B>Rotational:<!B>\t%s", tmp_buff);
        line_cnt = 10;
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_cnt < MAX_DEV_INFO_LINES) {
        asprintf(&swindow_info[line_cnt], " ");
        line_cnt++;
    }
    if (line_cnt < MAX_DEV_INFO_LINES) {
        asprintf(&swindow_info[line_cnt], CONTINUE_MSG);
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
    for (i = 0; i < MAX_DEV_INFO_LINES; i++ ) {
        freeChar(swindow_info[i]);
    }
    return;
}


/*
 * Run the Add Device dialog
 */
void addDeviceDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCROLL *dev_list = 0;
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
    static char *scst_dev_types[] = {"<C>dev_disk", "<C>dev_disk_perf",
    "<C>vcdrom", "<C>vdisk_blockio", "<C>vdisk_fileio", "<C>vdisk_nullio"},
            *bs_list[] = {"512", "1024", "2048", "4096", "8192"},
            *no_yes[] = {"No (0)", "Yes (1)"};
    char attr_path[MAX_SYSFS_PATH_SIZE] = {0}, attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            fileio_file[MAX_SYSFS_ATTR_SIZE] = {0}, temp_str[SCST_DEV_NAME_LEN] = {0},
            device_id[MAX_SYSFS_ATTR_SIZE] = {0}, fs_name[MAX_FS_ATTR_LEN] = {0},
            fs_path[MAX_FS_ATTR_LEN] = {0}, fs_type[MAX_FS_ATTR_LEN] = {0},
            real_blk_dev[MAX_SYSFS_PATH_SIZE] = {0};
    char *scsi_disk = NULL, *error_msg = NULL, *selected_file = NULL,
            *iso_file_name = NULL, *dev_id_ptr = NULL, *cmd_str = NULL,
            *block_dev = NULL;
    char *dev_info_msg[ADD_DEV_INFO_LINES] = {NULL};
    int dev_window_lines = 0, dev_window_cols = 0, window_y = 0, window_x = 0,
            dev_choice = 0, temp_int = 0, i = 0, traverse_ret = 0,
            exit_stat = 0, ret_val = 0;
    FILE *sg_vpd_cmd = NULL;
    boolean mounted = FALSE;

    /* Prompt for new device type */
    dev_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 12, 30,
            "<C></31/B>Choose a SCST device handler:\n", scst_dev_types, 6,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!dev_list) {
        errorDialog(main_cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(dev_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(dev_list, COLOR_DIALOG_TEXT);
    dev_choice = activateCDKScroll(dev_list, 0);

    if (dev_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(dev_list);
        refreshCDKScreen(main_cdk_screen);
        goto cleanup;

    } else if (dev_list->exitType == vNORMAL) {
        destroyCDKScroll(dev_list);
        refreshCDKScreen(main_cdk_screen);
    }

    /* Populate the CDK screen based on the handler */
    switch (dev_choice) {

        /* dev_disk */
        case 0:
            /* Get disk choice from user */
            if ((scsi_disk = getSCSIDiskChoice(main_cdk_screen)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/dev_disk/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add_device %s", scsi_disk);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }
            break;

        /* dev_disk_perf */
        case 1:
            /* Get disk choice from user */
            if ((scsi_disk = getSCSIDiskChoice(main_cdk_screen)) != NULL) {
                /* Set the sysfs path + attribute value then write it */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/dev_disk_perf/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add_device %s", scsi_disk);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }
            break;

        /* vcdrom */
        case 2:
            /* Create the file selector widget */
            file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER, 20, 40,
                    "<C></31/B>Choose an ISO file:\n", "File: ",
                    COLOR_DIALOG_INPUT, '_' | COLOR_DIALOG_INPUT,
                    A_REVERSE, "</N>", "</B>", "</N>", "</N>", TRUE, FALSE);
            if (!file_select) {
                errorDialog(main_cdk_screen, "Couldn't create file selector widget!", NULL);
                break;
            }
            setCDKFselectBoxAttribute(file_select, COLOR_DIALOG_BOX);
            setCDKFselectBackgroundAttrib(file_select, COLOR_DIALOG_TEXT);
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
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/vcdrom/mgmt", SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add_device %s", iso_file_name);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                } else {
                    /* The device has been created, now set the ISO file */
                    snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/vcdrom/%s/filename",
                            SYSFS_SCST_TGT, iso_file_name);
                    snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "%s", selected_file);
                    if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                        asprintf(&error_msg, "Couldn't update SCST device parameters: %s",
                                strerror(temp_int));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        freeChar(error_msg);
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

            if ((strstr(block_dev, "/dev/sd")) != NULL) {
                /* Get a unique ID for the device using the sg_vpd utility; this
                 * is then used for the device node link that exists in /dev */
                asprintf(&cmd_str, "%s -i -q %s 2>&1", SG_VPD_BIN, block_dev);
                sg_vpd_cmd = popen(cmd_str, "r");
                while (fgets(device_id, sizeof (device_id), sg_vpd_cmd) != NULL) {
                    if (strstr(device_id, "0x")) {
                        dev_id_ptr = strStrip(device_id);
                    }
                }
                if ((exit_stat = pclose(sg_vpd_cmd)) == -1) {
                    ret_val = -1;
                } else {
                    if (WIFEXITED(exit_stat))
                        ret_val = WEXITSTATUS(exit_stat);
                    else
                        ret_val = -1;
                }
                freeChar(cmd_str);
                if (ret_val != 0) {
                    asprintf(&error_msg, "The %s command exited with %d.", SG_VPD_BIN, ret_val);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    break;
                }
                snprintf(real_blk_dev, MAX_SYSFS_PATH_SIZE, "/dev/disk-by-id/%s", dev_id_ptr);
            } else {
                /* Not a normal SCSI disk block device (md, dm, etc.) */
                snprintf(real_blk_dev, MAX_SYSFS_PATH_SIZE, "%s", block_dev);
            }

            /* New CDK screen */
            dev_window_lines = 18;
            dev_window_cols = 60;
            window_y = ((LINES / 2) - (dev_window_lines / 2));
            window_x = ((COLS / 2) - (dev_window_cols / 2));
            dev_window = newwin(dev_window_lines, dev_window_cols, window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
                break;
            }
            boxWindow(dev_window, COLOR_DIALOG_BOX);
            wbkgd(dev_window, COLOR_DIALOG_TEXT);
            wrefresh(dev_window);

            /* Information label */
            asprintf(&dev_info_msg[0], "</31/B>Adding new vdisk_blockio SCST device...");
            asprintf(&dev_info_msg[1], " ");
            asprintf(&dev_info_msg[2], "SCSI Block Device: %.35s", real_blk_dev);
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 3, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
                goto cleanup;
            }
            setCDKLabelBackgroundAttrib(dev_info, COLOR_DIALOG_TEXT);

            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1), (window_y + 5),
                    NULL, "</B>SCST device name (no spaces): ",
                    COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
                goto cleanup;
            }
            setCDKEntryBoxAttribute(dev_name_field, COLOR_DIALOG_INPUT);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen, (window_x + 1), (window_y + 7),
                    "</B>Block Size", NULL, bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, "Couldn't create item list widget!", NULL);
                goto cleanup;
            }
            setCDKItemlistBackgroundAttrib(block_size, COLOR_DIALOG_TEXT);

            /* Write through widget (radio) */
            write_through = newCDKRadio(dev_screen, (window_x + 1), (window_y + 11),
                    NONE, 3, 10, "</B>Write Through", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!write_through) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(write_through, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(write_through, 0);

            /* NV cache widget (radio) */
            nv_cache = newCDKRadio(dev_screen, (window_x + 18), (window_y + 7),
                    NONE, 3, 10, "</B>NV Cache", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!nv_cache) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(nv_cache, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(nv_cache, 0);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 18), (window_y + 11),
                    NONE, 3, 10, "</B>Read Only", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(read_only, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 32), (window_y + 7),
                    NONE, 3, 10, "</B>Removable", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(removable, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(removable, 0);

            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 32), (window_y + 11),
                    NONE, 3, 10, "</B>Rotational", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(rotational, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 21), (window_y + 16),
                    "</B>   OK   ", ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
            cancel_button = newCDKButton(dev_screen, (window_x + 31), (window_y + 16),
                    "</B> Cancel ", cancel_cb, FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                strncpy(temp_str, getCDKEntryValue(dev_name_field), SCST_DEV_NAME_LEN);
                i = 0;
                while (temp_str[i] != '\0') {
                    /* If the user didn't input an acceptable name, then cancel out */
                    if (!VALID_NAME_CHAR(temp_str[i])) {
                        errorDialog(main_cdk_screen,
                                "The name entry field contains invalid characters!",
                                VALID_NAME_CHAR_MSG);
                        goto cleanup;
                    }
                    i++;
                }
                /* User didn't provide a device name */
                if (i == 0) {
                    errorDialog(main_cdk_screen, "The device name field cannot be empty!", NULL);
                    goto cleanup;
                }

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/vdisk_blockio/mgmt",
                        SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s filename=%s; blocksize=%s; write_through=%d; nv_cache=%d; read_only=%d; removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field), real_blk_dev,
                        bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(write_through),
                        getCDKRadioSelectedItem(nv_cache),
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }
            break;

        /* vdisk_fileio */
        case 4:
            /* Have the user select a file system */
            getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
            if (fs_name[0] == '\0')
                return;

            if (!mounted) {
                errorDialog(main_cdk_screen, "The selected file system is not mounted!", NULL);
                break;
            }

            /* Create the file selector widget */
            file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER, 20, 40,
                    "<C></31/B>Choose a back-end file:\n", "File: ",
                    COLOR_DIALOG_INPUT, '_' | COLOR_DIALOG_INPUT,
                    A_REVERSE, "</N>", "</B>", "</N>", "</N>", TRUE, FALSE);
            if (!file_select) {
                errorDialog(main_cdk_screen, "Couldn't create file selector widget!", NULL);
                break;
            }
            setCDKFselectBoxAttribute(file_select, COLOR_DIALOG_BOX);
            setCDKFselectBackgroundAttrib(file_select, COLOR_DIALOG_TEXT);
            setCDKFselectDirectory(file_select, fs_path);

            /* Activate the widget and let the user choose a file */
            selected_file = activateCDKFselect(file_select, 0);
            if (file_select->exitType == vESCAPE_HIT) {
                destroyCDKFselect(file_select);
                refreshCDKScreen(main_cdk_screen);
                break;
            } else if (file_select->exitType == vNORMAL) {
                strncpy(fileio_file, selected_file, MAX_SYSFS_ATTR_SIZE);
                destroyCDKFselect(file_select);
                refreshCDKScreen(main_cdk_screen);
            }

            /* New CDK screen */
            dev_window_lines = 18;
            dev_window_cols = 60;
            window_y = ((LINES / 2) - (dev_window_lines / 2));
            window_x = ((COLS / 2) - (dev_window_cols / 2));
            dev_window = newwin(dev_window_lines, dev_window_cols, window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
                break;
            }
            boxWindow(dev_window, COLOR_DIALOG_BOX);
            wbkgd(dev_window, COLOR_DIALOG_TEXT);
            wrefresh(dev_window);

            /* Information label */
            asprintf(&dev_info_msg[0], "</31/B>Adding new vdisk_fileio SCST device...");
            asprintf(&dev_info_msg[1], " ");
            asprintf(&dev_info_msg[2], "Virtual Disk File: %.35s", fileio_file);
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 3, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
                goto cleanup;
            }
            setCDKLabelBackgroundAttrib(dev_info, COLOR_DIALOG_TEXT);

            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1), (window_y + 5),
                    NULL, "</B>SCST device name (no spaces): ",
                    COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
                goto cleanup;
            }
            setCDKEntryBoxAttribute(dev_name_field, COLOR_DIALOG_INPUT);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen, (window_x + 1), (window_y + 7),
                    "</B>Block Size", NULL, bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, "Couldn't create item list widget!", NULL);
                goto cleanup;
            }
            setCDKItemlistBackgroundAttrib(block_size, COLOR_DIALOG_TEXT);

            /* Write through widget (radio) */
            write_through = newCDKRadio(dev_screen, (window_x + 1), (window_y + 11),
                    NONE, 3, 10, "</B>Write Through", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!write_through) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(write_through, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(write_through, 0);

            /* NV cache widget (radio) */
            nv_cache = newCDKRadio(dev_screen, (window_x + 18), (window_y + 7),
                    NONE, 3, 10, "</B>NV Cache", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!nv_cache) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(nv_cache, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(nv_cache, 0);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 18), (window_y + 11),
                    NONE, 3, 10, "</B>Read Only", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(read_only, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 32), (window_y + 7),
                    NONE, 3, 10, "</B>Removable", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(removable, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(removable, 0);

            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 32), (window_y + 11),
                    NONE, 3, 10, "</B>Rotational", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(rotational, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 21), (window_y + 16),
                    "</B>   OK   ", ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
            cancel_button = newCDKButton(dev_screen, (window_x + 31), (window_y + 16),
                    "</B> Cancel ", cancel_cb, FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                strncpy(temp_str, getCDKEntryValue(dev_name_field), SCST_DEV_NAME_LEN);
                i = 0;
                while (temp_str[i] != '\0') {
                    /* If the user didn't input an acceptable name, then cancel out */
                    if (!VALID_NAME_CHAR(temp_str[i])) {
                        errorDialog(main_cdk_screen,
                                "The name entry field contains invalid characters!",
                                VALID_NAME_CHAR_MSG);
                        goto cleanup;
                    }
                    i++;
                }
                /* User didn't provide a device name */
                if (i == 0) {
                    errorDialog(main_cdk_screen, "The device name field cannot be empty!", NULL);
                    goto cleanup;
                }

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/vdisk_fileio/mgmt",
                        SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s filename=%s; blocksize=%s; write_through=%d; nv_cache=%d; read_only=%d; removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field), fileio_file,
                        bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(write_through),
                        getCDKRadioSelectedItem(nv_cache),
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
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
            dev_window = newwin(dev_window_lines, dev_window_cols, window_y, window_x);
            if (dev_window == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
                break;
            }
            dev_screen = initCDKScreen(dev_window);
            if (dev_screen == NULL) {
                errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
                break;
            }
            boxWindow(dev_window, COLOR_DIALOG_BOX);
            wbkgd(dev_window, COLOR_DIALOG_TEXT);
            wrefresh(dev_window);

            /* Information label */
            asprintf(&dev_info_msg[0], "</31/B>Adding new vdisk_nullio SCST device...");
            asprintf(&dev_info_msg[1], " ");
            dev_info = newCDKLabel(dev_screen, (window_x + 1), (window_y + 1),
                    dev_info_msg, 2, FALSE, FALSE);
            if (!dev_info) {
                errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
                goto cleanup;
            }
            setCDKLabelBackgroundAttrib(dev_info, COLOR_DIALOG_TEXT);
            
            /* Device name widget (entry) */
            dev_name_field = newCDKEntry(dev_screen, (window_x + 1), (window_y + 3),
                    NULL, "</B>SCST device name (no spaces): ",
                    COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                    SCST_DEV_NAME_LEN, 0, SCST_DEV_NAME_LEN, FALSE, FALSE);
            if (!dev_name_field) {
                errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
                goto cleanup;
            }
            setCDKEntryBoxAttribute(dev_name_field, COLOR_DIALOG_INPUT);

            /* Block size widget (item list) */
            block_size = newCDKItemlist(dev_screen, (window_x + 1), (window_y + 5),
                    "</B>Block Size", NULL, bs_list, 5, 0, FALSE, FALSE);
            if (!block_size) {
                errorDialog(main_cdk_screen, "Couldn't create item list widget!", NULL);
                goto cleanup;
            }
            setCDKItemlistBackgroundAttrib(block_size, COLOR_DIALOG_TEXT);

            /* Read only widget (radio) */
            read_only = newCDKRadio(dev_screen, (window_x + 1), (window_y + 9),
                    NONE, 3, 10, "</B>Read Only", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!read_only) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(read_only, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(read_only, 0);

            /* Removable widget (radio) */
            removable = newCDKRadio(dev_screen, (window_x + 18), (window_y + 5),
                    NONE, 3, 10, "</B>Removable", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!removable) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(removable, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(removable, 0);
            
            /* Rotational widget (radio) */
            rotational = newCDKRadio(dev_screen, (window_x + 18), (window_y + 9),
                    NONE, 3, 10, "</B>Rotational", no_yes, 2,
                    '#' | COLOR_DIALOG_SELECT, 1,
                    COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!rotational) {
                errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
                goto cleanup;
            }
            setCDKRadioBackgroundAttrib(rotational, COLOR_DIALOG_TEXT);
            setCDKRadioCurrentItem(rotational, 1);

            /* Buttons */
            ok_button = newCDKButton(dev_screen, (window_x + 16), (window_y + 16),
                    "</B>   OK   ", ok_cb, FALSE, FALSE);
            if (!ok_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
            cancel_button = newCDKButton(dev_screen, (window_x + 26), (window_y + 16),
                    "</B> Cancel ", cancel_cb, FALSE, FALSE);
            if (!cancel_button) {
                errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
                goto cleanup;
            }
            setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

            /* Allow user to traverse the screen */
            refreshCDKScreen(dev_screen);
            traverse_ret = traverseCDKScreen(dev_screen);

            /* User hit 'OK' button */
            if (traverse_ret == 1) {
                /* Turn the cursor off (pretty) */
                curs_set(0);

                /* Check device name (field entry) */
                strncpy(temp_str, getCDKEntryValue(dev_name_field), SCST_DEV_NAME_LEN);
                i = 0;
                while (temp_str[i] != '\0') {
                    /* If the user didn't input an acceptable name, then cancel out */
                    if (!VALID_NAME_CHAR(temp_str[i])) {
                        errorDialog(main_cdk_screen,
                                "The name entry field contains invalid characters!",
                                VALID_NAME_CHAR_MSG);
                        goto cleanup;
                    }
                    i++;
                }
                /* User didn't provide a device name */
                if (i == 0) {
                    errorDialog(main_cdk_screen, "The device name field cannot be empty!", NULL);
                    goto cleanup;
                }

                /* Add the new device */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/vdisk_nullio/mgmt",
                        SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                        "add_device %s blocksize=%s; read_only=%d; removable=%d; rotational=%d",
                        getCDKEntryValue(dev_name_field),
                        bs_list[getCDKItemlistCurrentItem(block_size)],
                        getCDKRadioSelectedItem(read_only),
                        getCDKRadioSelectedItem(removable),
                        getCDKRadioSelectedItem(rotational));
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't add SCST device: %s", strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }
            break;
            
        default:
            errorDialog(main_cdk_screen, "Default case reached in switch statement?", NULL);
            break;
    }

    /* All done */
    cleanup:
    for (i = 0; i < ADD_DEV_INFO_LINES; i++) {
        freeChar(dev_info_msg[i]);
    }
    if (dev_screen != NULL) {
        destroyCDKScreenObjects(dev_screen);
        destroyCDKScreen(dev_screen);
        delwin(dev_window);
    }
    /* Using the file selector widget changes the CWD -- fix it */
    if ((chdir(getenv("HOME"))) == -1) {
        asprintf(&error_msg, "chdir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    }
    return;
}


/*
 * Run the Delete Device dialog
 */
void delDeviceDialog(CDKSCREEN *main_cdk_screen) {
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
    asprintf(&confirm_msg, "Are you sure you want to delete SCST device %s (%s)?",
            scst_dev, scst_hndlr);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    freeChar(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST device */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s/mgmt",
                SYSFS_SCST_TGT, scst_hndlr);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del_device %s", scst_dev);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't delete SCST device: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    return;
}


/*
 * Run the Map to Group dialog
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
    static char *no_yes[] = {"No (0)", "Yes (1)"};
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
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        return;
    }
    map_screen = initCDKScreen(map_window);
    if (map_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        return;
    }
    boxWindow(map_window, COLOR_DIALOG_BOX);
    wbkgd(map_window, COLOR_DIALOG_TEXT);
    wrefresh(map_window);

    /* Information label */
    asprintf(&map_info_msg[0], "</31/B>Mapping SCST device...");
    asprintf(&map_info_msg[1], " ");
    asprintf(&map_info_msg[2], "Device: %s", scst_dev);
    asprintf(&map_info_msg[3], "Handler: %s", scst_hndlr);
    asprintf(&map_info_msg[4], " ");
    asprintf(&map_info_msg[5], "Target: %s", scst_tgt);
    asprintf(&map_info_msg[6], "Driver: %s", tgt_driver);
    asprintf(&map_info_msg[7], "Group: %s", group_name);
    map_info = newCDKLabel(map_screen, (window_x + 1), (window_y + 1),
            map_info_msg, MAP_DEV_INFO_LINES, FALSE, FALSE);
    if (!map_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(map_info, COLOR_DIALOG_TEXT);

    /* LUN widget (scale) */
    lun = newCDKScale(map_screen, (window_x + 1), (window_y + 10),
            NULL, "</B>Logical Unit Number (LUN)", COLOR_DIALOG_SELECT,
            5, 0, 0, 255, 1, 1, FALSE, FALSE);
    if (!lun) {
        errorDialog(main_cdk_screen, "Couldn't create scale widget!", NULL);
        goto cleanup;
    }
    setCDKScaleBackgroundAttrib(lun, COLOR_DIALOG_TEXT);
    
    /* Read only widget (radio) */
    read_only = newCDKRadio(map_screen, (window_x + 1), (window_y + 12),
            NONE, 3, 10, "</B>Read Only", no_yes, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!read_only) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(read_only, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(read_only, 0);

    /* Buttons */
    ok_button = newCDKButton(map_screen, (window_x + 16), (window_y + 16),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(map_screen, (window_x + 26), (window_y + 16),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(map_screen);
    traverse_ret = traverseCDKScreen(map_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Add the new LUN (map device) */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/luns/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add %s %d read_only=%d",
                scst_dev, getCDKScaleValue(lun), getCDKRadioSelectedItem(read_only));
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't map SCST device: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* All done */
    cleanup:
    for (i = 0; i < MAP_DEV_INFO_LINES; i++) {
        freeChar(map_info_msg[i]);
    }
    if (map_screen != NULL) {
        destroyCDKScreenObjects(map_screen);
        destroyCDKScreen(map_screen);
        delwin(map_window);
    }
    return;
}


/*
 * Run the Unmap from Group dialog
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
    asprintf(&confirm_msg, "Are you sure you want to unmap SCST LUN %d (Group: %s)?",
            lun, group_name);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    freeChar(confirm_msg);
    if (confirm) {
        /* Remove the specified SCST LUN */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/luns/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %d", lun);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't remove SCST LUN: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* All done */
    return;
}