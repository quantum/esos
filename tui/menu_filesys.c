/**
 * @file menu_filesys.c
 * @brief Contains the menu actions for the 'File Systems' menu.
 * @author Copyright (c) 2012-2017 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <parted/parted.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <sys/param.h>
#include <limits.h>
#include <blkid/blkid.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Create File System" dialog.
 */
void createFSDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *fs_window = 0;
    CDKSCREEN *fs_screen = 0;
    CDKLABEL *add_fs_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *fs_label = 0;
    CDKRADIO *fs_type = 0, *add_part = 0;
    CDKSWINDOW *make_fs_info = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char fs_label_buff[MAX_FS_LABEL] = {0}, mkfs_cmd[MAX_SHELL_CMD_LEN] = {0},
            mount_cmd[MAX_SHELL_CMD_LEN] = {0},
            new_mnt_point[MAX_FS_ATTR_LEN] = {0},
            new_blk_dev_node[MAX_FS_ATTR_LEN] = {0},
            real_blk_dev_node[MAX_SYSFS_PATH_SIZE] = {0};
    char *block_dev = NULL, *error_msg = NULL, *confirm_msg = NULL,
            *dev_node = NULL, *device_size = NULL, *tmp_str_ptr = NULL,
            *swindow_title = NULL;
    char *fs_dialog_msg[MAX_FS_DIALOG_INFO_LINES] = {NULL},
            *swindow_info[MAX_MAKE_FS_INFO_LINES] = {NULL};
    FILE *fstab_file = NULL, *new_fstab_file = NULL;
    struct mntent *fstab_entry = NULL,
            addtl_fstab_entry; /* Not a pointer */
    int temp_int = 0, window_y = 0, window_x = 0, info_line_cnt = 0,
            traverse_ret = 0, i = 0, exit_stat = 0, ret_val = 0,
            fs_window_lines = 0, fs_window_cols = 0;
    PedDevice *device = NULL;
    PedDiskType *disk_type = NULL;
    PedDisk *disk = NULL;
    PedPartition *partition = NULL;
    PedFileSystemType *file_system_type = NULL;
    PedConstraint *start_constraint = NULL, *end_constraint = NULL,
             *final_constraint = NULL;
    boolean confirm = FALSE, question = FALSE, finished = FALSE;

    /* Get block device choice from user */
    if ((block_dev = getBlockDevChoice(main_cdk_screen)) == NULL)
        return;

    while (1) {
        if (strstr(block_dev, "/dev/disk-by-id") != NULL) {
            /* If its a SCSI disk, we need the real block device node */
            if (readlink(block_dev, real_blk_dev_node,
                    MAX_SYSFS_PATH_SIZE) == -1) {
                SAFE_ASPRINTF(&error_msg, "readlink(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        } else {
            /* Its not, so use what we have */
            snprintf(real_blk_dev_node, MAX_SYSFS_PATH_SIZE, "%s", block_dev);
        }

        /* Open the file system tab file */
        if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
            SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }

        /* Loop over fstab entries and check if our chosen block device from
         * above matches; this check obviously isn't 100% fail-safe */
        while ((fstab_entry = getmntent(fstab_file)) != NULL) {
            /* We consider the FS device/name from the fstab file the
             * haystack since these entries would typically have a
             * number at the end for the partition */
            if (strstr(fstab_entry->mnt_fsname, real_blk_dev_node) != NULL) {
                errorDialog(main_cdk_screen, "It appears the selected block "
                        "device already has a fstab entry.", NULL);
                endmntent(fstab_file);
                return;
            } else if ((strstr(fstab_entry->mnt_fsname, "LABEL=") != NULL) ||
                    (strstr(fstab_entry->mnt_fsname, "UUID=") != NULL)) {
                /* Find the device node for the given file system */
                if ((dev_node = blkid_get_devname(NULL,
                        fstab_entry->mnt_fsname, NULL)) != NULL) {
                    if ((strstr(dev_node, real_blk_dev_node) != NULL)) {
                        errorDialog(main_cdk_screen, "It appears the selected "
                                "block device already has a fstab entry.",
                                NULL);
                        endmntent(fstab_file);
                        return;
                    }
                }
            }
        }

        /* Close the fstab file */
        endmntent(fstab_file);

        /* Setup a new CDK screen for required input */
        fs_window_lines = 21;
        fs_window_cols = 70;
        window_y = ((LINES / 2) - (fs_window_lines / 2));
        window_x = ((COLS / 2) - (fs_window_cols / 2));
        fs_window = newwin(fs_window_lines, fs_window_cols,
                window_y, window_x);
        if (fs_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        fs_screen = initCDKScreen(fs_window);
        if (fs_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(fs_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(fs_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(fs_window);

        /* Grab the device information */
        if ((device = ped_device_get(real_blk_dev_node)) == NULL) {
            errorDialog(main_cdk_screen,
                    "Calling ped_device_get() failed.", NULL);
            break;
        }
        if ((device_size = ped_unit_format_byte(device,
                device->length * device->sector_size)) == NULL) {
            errorDialog(main_cdk_screen,
                    "Calling ped_unit_format_byte() failed.", NULL);
            break;
        }
        /* Its okay if this one returns NULL (eg, no disk label) */
        if ((disk_type = ped_disk_probe(device)) != NULL) {
            /* We only read the disk label if it actually has one */
            if ((disk = ped_disk_new(device)) == NULL) {
                errorDialog(main_cdk_screen, "Calling ped_disk_new() failed.",
                        NULL);
                break;
            }
        }

        /* Information label */
        SAFE_ASPRINTF(&fs_dialog_msg[0],
                "</%d/B>Creating new file system (on block device %.20s)...",
                g_color_dialog_title[g_curr_theme], real_blk_dev_node);
        /* Using asprintf() for a blank space makes it
         * easier on clean-up (free) */
        SAFE_ASPRINTF(&fs_dialog_msg[1], " ");
        SAFE_ASPRINTF(&fs_dialog_msg[2],
                "</B>Model:<!B> %-20.20s </B>Transport:<!B>  %s",
                device->model, g_transports[device->type]);
        SAFE_ASPRINTF(&fs_dialog_msg[3],
                "</B>Size:<!B>  %-20.20s </B>Disk label:<!B> %s",
                device_size, (disk_type) ? disk_type->name : "none");
        SAFE_ASPRINTF(&fs_dialog_msg[4],
                "</B>Sector size (logical/physical):<!B>\t%lldB/%lldB",
                device->sector_size, device->phys_sector_size);
        SAFE_ASPRINTF(&fs_dialog_msg[5], " ");
        /* Add partition information (if any) */
        info_line_cnt = 6;
        if (disk && (ped_disk_get_last_partition_num(disk) > 0)) {
            SAFE_ASPRINTF(&fs_dialog_msg[info_line_cnt],
                    "</B>Current layout: %5s %12s %12s %15s<!B>",
                    "No.", "Start", "Size", "FS Type");
            info_line_cnt++;
            for (partition = ped_disk_next_partition(disk, NULL);
                    partition && (info_line_cnt < MAX_FS_DIALOG_INFO_LINES);
                    partition = ped_disk_next_partition(disk, partition)) {
                if (partition->num < 0)
                    continue;
                SAFE_ASPRINTF(&fs_dialog_msg[info_line_cnt],
                        "\t\t%5d %12lld %12lld %15.15s",
                        partition->num, partition->geom.start,
                        partition->geom.length,
                        (partition->fs_type) ?
                        partition->fs_type->name : "unknown");
                info_line_cnt++;
            }
        } else {
            SAFE_ASPRINTF(&fs_dialog_msg[info_line_cnt],
                    "</B><No partitions found.><!B>");
            info_line_cnt++;
        }
        add_fs_info = newCDKLabel(fs_screen, (window_x + 1), (window_y + 1),
                fs_dialog_msg, info_line_cnt, FALSE, FALSE);
        if (!add_fs_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(add_fs_info,
                g_color_dialog_text[g_curr_theme]);

        /* Clean up the libparted stuff */
        FREE_NULL(device_size);
        if (disk != NULL)
            ped_disk_destroy(disk);
        ped_device_destroy(device);

        /* FS label field */
        fs_label = newCDKEntry(fs_screen, (window_x + 1), (window_y + 13),
                "</B>File System Label", NULL,
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vLMIXED,
                MAX_FS_LABEL, 0, MAX_FS_LABEL, FALSE, FALSE);
        if (!fs_label) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(fs_label, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(fs_label,
                    g_color_dialog_text[g_curr_theme]);

        /* FS type radio list */
        fs_type = newCDKRadio(fs_screen, (window_x + 22), (window_y + 13),
                NONE, 5, 10, "</B>File System Type", g_fs_type_opts, 4,
                '#' | g_color_dialog_select[g_curr_theme], 0,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!fs_type) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(fs_type, g_color_dialog_text[g_curr_theme]);

        /* Partition yes/no widget (radio) */
        add_part = newCDKRadio(fs_screen, (window_x + 42), (window_y + 13),
                NONE, 3, 10, "</B>Partition Device", g_no_yes_opts, 2,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!add_part) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(add_part,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(add_part, 1);

        /* Buttons */
        ok_button = newCDKButton(fs_screen, (window_x + 26), (window_y + 19),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(fs_screen, (window_x + 36),
                (window_y + 19), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(fs_screen);
        traverse_ret = traverseCDKScreen(fs_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check FS label value (field entry) */
            strncpy(fs_label_buff, getCDKEntryValue(fs_label), MAX_FS_LABEL);
            if (!checkInputStr(main_cdk_screen, NAME_CHARS, fs_label_buff))
                break;

            /* We should probably also use the findfs tool to see if any
             * devices actually use the given file system label, but for
             * now we only consider the fstab file as the source of truth */
            if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
                SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
            while ((fstab_entry = getmntent(fstab_file)) != NULL) {
                if (strstr(fstab_entry->mnt_fsname, "LABEL=") != NULL) {
                    /* The strchr function returns NULL if not found */
                    if ((tmp_str_ptr = strchr(fstab_entry->mnt_fsname,
                            '=')) != NULL) {
                        tmp_str_ptr++;
                        if (strcmp(tmp_str_ptr, fs_label_buff) == 0) {
                            errorDialog(main_cdk_screen, "It appears this file "
                                    "system label is already in use!", NULL);
                            endmntent(fstab_file);
                            finished = TRUE;
                            break;
                        }
                    }
                }
            }
            endmntent(fstab_file);
            if (finished)
                break;

            /* Get FS type choice */
            temp_int = getCDKRadioSelectedItem(fs_type);

            /* Get confirmation before applying block device changes */
            SAFE_ASPRINTF(&confirm_msg,
                    "You are about to write a new file system to '%s';",
                    real_blk_dev_node);
            confirm = confirmDialog(main_cdk_screen, confirm_msg,
                    "this will destroy all data on the device. Are you sure?");
            FREE_NULL(confirm_msg);
            if (confirm) {
                /* A scroll window to show progress */
                SAFE_ASPRINTF(&swindow_title,
                        "<C></%d/B>Setting up new file system...\n",
                        g_color_dialog_title[g_curr_theme]);
                make_fs_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                        MAKE_FS_INFO_ROWS + 2, MAKE_FS_INFO_COLS + 2,
                        swindow_title, MAX_MAKE_FS_INFO_LINES, TRUE, FALSE);
                if (!make_fs_info) {
                    errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
                    return;
                }
                setCDKSwindowBackgroundAttrib(make_fs_info,
                        g_color_dialog_text[g_curr_theme]);
                setCDKSwindowBoxAttribute(make_fs_info,
                        g_color_dialog_box[g_curr_theme]);
                drawCDKSwindow(make_fs_info, TRUE);
                i = 0;

                if (getCDKRadioSelectedItem(add_part) == 1) {
                    /* Setup the new partition (if chosen) */
                    if (i < MAX_MAKE_FS_INFO_LINES) {
                        SAFE_ASPRINTF(&swindow_info[i],
                                "<C>Creating new disk label and partition...");
                        addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                        i++;
                    }

                    /* Get the device and disk / file system type */
                    if ((device = ped_device_get(real_blk_dev_node)) == NULL) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_device_get() failed.", NULL);
                        break;
                    }
                    disk_type = ped_disk_type_get("gpt");
                    if ((disk = ped_disk_new_fresh(device,
                            disk_type)) == NULL) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_disk_new_fresh() failed.", NULL);
                        break;
                    }
                    file_system_type =
                            ped_file_system_type_get(g_fs_type_opts[temp_int]);
                    if ((partition = ped_partition_new(disk,
                            PED_PARTITION_NORMAL, file_system_type,
                            0, (device->length - 1))) == NULL) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_partition_new() failed.", NULL);
                        break;
                    }

                    /* Get the new partition size constraints (start/end) */
                    // TODO: These need to be checked for errors.
                    start_constraint =
                            ped_device_get_optimal_aligned_constraint(device);
                    assert(start_constraint != NULL);
                    end_constraint = ped_constraint_new_from_max(
                            &partition->geom);
                    assert(end_constraint != NULL);
                    final_constraint = ped_constraint_intersect(
                            start_constraint, end_constraint);
                    assert(final_constraint != NULL);
                    ped_constraint_destroy(start_constraint);
                    ped_constraint_destroy(end_constraint);

                    /* Add the disk partition */
                    if (ped_disk_add_partition(disk, partition,
                            final_constraint) == 0) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_disk_add_partition() failed.",
                                NULL);
                        break;
                    }

                    /* Commit the new partition */
                    if (ped_disk_commit_to_dev(disk) == 0) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_disk_commit_to_dev() failed.",
                                NULL);
                        break;
                    }
                    if (ped_disk_commit_to_os(disk) == 0) {
                        errorDialog(main_cdk_screen,
                                "Calling ped_disk_commit_to_os() failed.",
                                NULL);
                        break;
                    }

                    /* Set the full path to the block device partition */
                    snprintf(new_blk_dev_node, MAX_FS_ATTR_LEN, "%s",
                            ped_partition_get_path(partition));
                    ped_constraint_destroy(final_constraint);
                    ped_disk_destroy(disk);

                } else {
                    /* Don't partition */
                    snprintf(new_blk_dev_node, MAX_FS_ATTR_LEN, "%s",
                            real_blk_dev_node);
                }

                /* Create the file system */
                if (i < MAX_MAKE_FS_INFO_LINES) {
                    SAFE_ASPRINTF(&swindow_info[i],
                            "<C>Creating new %s file system...",
                            g_fs_type_opts[temp_int]);
                    addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                    i++;
                }
                snprintf(mkfs_cmd, MAX_SHELL_CMD_LEN,
                        "mkfs.%s -L %s -%s %s > /dev/null 2>&1",
                        g_fs_type_opts[temp_int], fs_label_buff,
                        (((strcmp(g_fs_type_opts[temp_int], "btrfs") == 0) ||
                        (strcmp(g_fs_type_opts[temp_int],
                        "xfs") == 0)) ? "f" : "F"),
                        new_blk_dev_node);
                ret_val = system(mkfs_cmd);
                if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                    SAFE_ASPRINTF(&error_msg,
                            "Running '%s' failed; exited with %d.",
                            mkfs_cmd, exit_stat);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }

                /* Add the new file system entry to the fstab file */
                if (i < MAX_MAKE_FS_INFO_LINES) {
                    SAFE_ASPRINTF(&swindow_info[i],
                            "<C>Adding new entry to fstab file...");
                    addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                    i++;
                }
                /* Open the original file system tab file */
                if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, "setmntent(): %s",
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                /* Open the new/temporary file system tab file */
                if ((new_fstab_file = setmntent(FSTAB_TMP, "w+")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, "setmntent(): %s",
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                /* Loop over the original fstab file, and add
                 * each entry to the new file */
                while ((fstab_entry = getmntent(fstab_file)) != NULL) {
                    addmntent(new_fstab_file, fstab_entry);
                }
                /* New fstab entry */
                SAFE_ASPRINTF(&addtl_fstab_entry.mnt_fsname, "LABEL=%s",
                        fs_label_buff);
                SAFE_ASPRINTF(&addtl_fstab_entry.mnt_dir, "%s/%s",
                        VDISK_MNT_BASE, fs_label_buff);
                SAFE_ASPRINTF(&addtl_fstab_entry.mnt_type, "%s",
                        g_fs_type_opts[temp_int]);
                SAFE_ASPRINTF(&addtl_fstab_entry.mnt_opts, "defaults");
                addtl_fstab_entry.mnt_freq = 1;
                addtl_fstab_entry.mnt_passno = 1;
                addmntent(new_fstab_file, &addtl_fstab_entry);
                fflush(new_fstab_file);
                endmntent(new_fstab_file);
                endmntent(fstab_file);
                FREE_NULL(addtl_fstab_entry.mnt_fsname);
                FREE_NULL(addtl_fstab_entry.mnt_dir);
                FREE_NULL(addtl_fstab_entry.mnt_type);
                FREE_NULL(addtl_fstab_entry.mnt_opts);
                if ((rename(FSTAB_TMP, FSTAB)) == -1) {
                    SAFE_ASPRINTF(&error_msg, "rename(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                /* Make the mount point directory */
                snprintf(new_mnt_point, MAX_FS_ATTR_LEN, "%s/%s",
                        VDISK_MNT_BASE, fs_label_buff);
                if ((mkdir(new_mnt_point,
                        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) ==
                        -1) {
                    SAFE_ASPRINTF(&error_msg, "mkdir(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }

                /* Clean up the screen */
                destroyCDKSwindow(make_fs_info);
                refreshCDKScreen(main_cdk_screen);

                /* Ask if user would like to mount the new FS */
                question = questionDialog(main_cdk_screen,
                        "Would you like to mount the new file system now?",
                        NULL);
                if (question) {
                    /* Run mount */
                    snprintf(mount_cmd, MAX_SHELL_CMD_LEN,
                            "%s %s > /dev/null 2>&1", MOUNT_BIN, new_mnt_point);
                    ret_val = system(mount_cmd);
                    if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                        SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                                MOUNT_BIN, exit_stat);
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        break;
                    }
                }
            }
        }
        break;
    }

    /* All done */
    destroyCDKSwindow(make_fs_info);
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_MAKE_FS_INFO_LINES; i++)
        FREE_NULL(swindow_info[i]);
    for (i = 0; i < info_line_cnt; i++)
        FREE_NULL(fs_dialog_msg[i]);
    if (fs_screen != NULL) {
        destroyCDKScreenObjects(fs_screen);
        destroyCDKScreen(fs_screen);
    }
    delwin(fs_window);
    return;
}


/**
 * @brief Run the "Remove File System" dialog.
 */
void removeFSDialog(CDKSCREEN *main_cdk_screen) {
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0},
            umount_cmd[MAX_SHELL_CMD_LEN] = {0};
    char *confirm_msg = NULL, *error_msg = NULL;
    boolean mounted = FALSE, question = FALSE, confirm = FALSE;
    FILE *fstab_file = NULL, *new_fstab_file = NULL;
    struct mntent *fstab_entry = NULL;
    int ret_val = 0, exit_stat = 0;

    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    /* If the selected file system is mounted, ask to try un-mounting it */
    if (mounted) {
        question = questionDialog(main_cdk_screen, "It appears that file "
                "system is mounted; would you like to try un-mounting",
                "it now? The file system must be "
                "un-mounted before proceeding.");
        if (question) {
            /* Run umount */
            snprintf(umount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                    UMOUNT_BIN, fs_path);
            ret_val = system(umount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, UMOUNT_BIN,
                        exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                return;
            }
        } else {
            return;
        }
    }

    /* Get confirmation before removing the file system */
    SAFE_ASPRINTF(&confirm_msg, "'%s' file system?", fs_name);
    confirm = confirmDialog(main_cdk_screen,
            "Are you sure you would like to remove the", confirm_msg);
    FREE_NULL(confirm_msg);

    /* Remove file system entry from fstab file */
    if (confirm) {
        /* Open the original file system tab file */
        if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
            SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }
        /* Open the new/temporary file system tab file */
        if ((new_fstab_file = setmntent(FSTAB_TMP, "w+")) == NULL) {
            SAFE_ASPRINTF(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }
        /* Loop over the original fstab file, and skip the entry
         * we are removing when writing out the new file */
        while ((fstab_entry = getmntent(fstab_file)) != NULL) {
            if (strcmp(fs_name, fstab_entry->mnt_fsname) != 0) {
                addmntent(new_fstab_file, fstab_entry);
            }
        }
        fflush(new_fstab_file);
        endmntent(new_fstab_file);
        endmntent(fstab_file);
        if ((rename(FSTAB_TMP, FSTAB)) == -1) {
            SAFE_ASPRINTF(&error_msg, "rename(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }

        /* Remove the mount point directory */
        if ((rmdir(fs_path)) == -1) {
            SAFE_ASPRINTF(&error_msg, "rmdir(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            return;
        }

        // TODO: Should we also remove/erase the file system label from disk?
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Virtual Disk File" dialog.
 */
void addVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *vdisk_window = 0;
    CDKSCREEN *vdisk_screen = 0;
    CDKLABEL *vdisk_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *vdisk_name = 0, *vdisk_size = 0;
    CDKHISTOGRAM *vdisk_progress = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0}, mount_cmd[MAX_SHELL_CMD_LEN] = {0},
            vdisk_name_buff[MAX_VDISK_NAME_LEN] = {0},
            gib_free_str[MISC_STRING_LEN] = {0},
            gib_total_str[MISC_STRING_LEN] = {0},
            zero_buff[VDISK_WRITE_SIZE] = {0},
            new_vdisk_file[MAX_VDISK_PATH_LEN] = {0};
    char *error_msg = NULL, *histogram_title = NULL;
    char *vdisk_dialog_msg[ADD_VDISK_INFO_LINES] = {NULL};
    boolean mounted = FALSE, question = FALSE, finished = FALSE;
    struct statvfs *fs_info = NULL;
    int window_y = 0, window_x = 0, traverse_ret = 0, i = 0, exit_stat = 0,
            ret_val = 0, vdisk_size_int = 0, new_vdisk_fd = 0,
            vdisk_window_lines = 0, vdisk_window_cols = 0;
    long long bytes_free = 0ll, bytes_total = 0ll, new_vdisk_bytes = 0ll,
            new_vdisk_mib = 0ll;
    off_t position = 0;
    ssize_t bytes_written = 0, write_length = 0;

    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    if (!mounted) {
        question = questionDialog(main_cdk_screen,
                NOT_MOUNTED_1, NOT_MOUNTED_2);
        if (question) {
            /* Run mount */
            snprintf(mount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                    MOUNT_BIN, fs_path);
            ret_val = system(mount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MOUNT_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                return;
            }
        } else {
            return;
        }
    }

    while (1) {
        /* Setup a new small CDK screen for virtual disk information */
        vdisk_window_lines = 12;
        vdisk_window_cols = 70;
        window_y = ((LINES / 2) - (vdisk_window_lines / 2));
        window_x = ((COLS / 2) - (vdisk_window_cols / 2));
        vdisk_window = newwin(vdisk_window_lines, vdisk_window_cols,
                window_y, window_x);
        if (vdisk_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        vdisk_screen = initCDKScreen(vdisk_window);
        if (vdisk_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(vdisk_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(vdisk_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(vdisk_window);

        /* Get the file system information */
        if (!(fs_info = (struct statvfs *) malloc(sizeof (struct statvfs)))) {
            errorDialog(main_cdk_screen, "Calling malloc() failed.", NULL);
            break;
        }
        if ((statvfs(fs_path, fs_info)) == -1) {
            SAFE_ASPRINTF(&error_msg, "statvfs(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
        bytes_free = fs_info->f_bavail * fs_info->f_bsize;
        snprintf(gib_free_str, MISC_STRING_LEN, "%lld GiB",
                (bytes_free / GIBIBYTE_SIZE));
        bytes_total = fs_info->f_blocks * fs_info->f_bsize;
        snprintf(gib_total_str, MISC_STRING_LEN, "%lld GiB",
                (bytes_total / GIBIBYTE_SIZE));

        /* Fill the information label */
        SAFE_ASPRINTF(&vdisk_dialog_msg[0],
                "</%d/B>Adding new virtual disk file...",
                g_color_dialog_title[g_curr_theme]);
        /* Using asprintf() for a blank space makes
         * it easier on clean-up (free) */
        SAFE_ASPRINTF(&vdisk_dialog_msg[1], " ");
        SAFE_ASPRINTF(&vdisk_dialog_msg[2],
                "</B>File System:<!B>\t%-20.20s </B>Type:<!B>\t\t%s",
                fs_path, fs_type);
        SAFE_ASPRINTF(&vdisk_dialog_msg[3],
                "</B>Size:<!B>\t\t%-20.20s </B>Available Space:<!B>\t%s",
                gib_total_str, gib_free_str);
        FREE_NULL(fs_info);
        vdisk_label = newCDKLabel(vdisk_screen, (window_x + 1), (window_y + 1),
                vdisk_dialog_msg, ADD_VDISK_INFO_LINES, FALSE, FALSE);
        if (!vdisk_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(vdisk_label,
                g_color_dialog_text[g_curr_theme]);

        /* Virtual disk file name */
        vdisk_name = newCDKEntry(vdisk_screen, (window_x + 1), (window_y + 6),
                "</B>Virtual Disk File Name", NULL,
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vLMIXED,
                20, 0, MAX_VDISK_NAME_LEN, FALSE, FALSE);
        if (!vdisk_name) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(vdisk_name, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(vdisk_name,
                    g_color_dialog_text[g_curr_theme]);

        /* Virtual disk size */
        vdisk_size = newCDKEntry(vdisk_screen, (window_x + 30), (window_y + 6),
                "</B>Virtual Disk Size (GiB)", NULL,
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vINT,
                12, 0, 12, FALSE, FALSE);
        if (!vdisk_size) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(vdisk_size, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(vdisk_size,
                    g_color_dialog_text[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(vdisk_screen, (window_x + 26), (window_y + 10),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(vdisk_screen, (window_x + 36),
                (window_y + 10), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(vdisk_screen);
        traverse_ret = traverseCDKScreen(vdisk_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check virtual disk name value (field entry) */
            strncpy(vdisk_name_buff, getCDKEntryValue(vdisk_name),
                    MAX_VDISK_NAME_LEN);
            if (!checkInputStr(main_cdk_screen, NAME_CHARS, vdisk_name_buff))
                break;

            /* Check virtual disk size value (field entry) */
            vdisk_size_int = atoi(getCDKEntryValue(vdisk_size));
            if (vdisk_size_int <= 0) {
                errorDialog(main_cdk_screen,
                        "The size value must be greater than zero.", NULL);
                break;
            }
            new_vdisk_bytes = vdisk_size_int * GIBIBYTE_SIZE;
            if (new_vdisk_bytes > bytes_free) {
                errorDialog(main_cdk_screen,
                        "The given size is greater than the available space!",
                        NULL);
                break;
            }

            /* Check if the new (potential) virtual disk file exists already */
            snprintf(new_vdisk_file, MAX_VDISK_PATH_LEN, "%s/%s",
                    fs_path, vdisk_name_buff);
            if (access(new_vdisk_file, F_OK) != -1) {
                SAFE_ASPRINTF(&error_msg, "It appears the '%s'",
                        new_vdisk_file);
                errorDialog(main_cdk_screen, error_msg, "file already exists!");
                FREE_NULL(error_msg);
                break;
            }

            /* Clean up the screen */
            destroyCDKScreenObjects(vdisk_screen);
            destroyCDKScreen(vdisk_screen);
            vdisk_screen = NULL;
            delwin(vdisk_window);
            refreshCDKScreen(main_cdk_screen);

            /* Make a new histogram widget to display the virtual
             * disk creation progress */
            SAFE_ASPRINTF(&histogram_title,
                    "<C></%d/B>Writing new virtual disk file (units = MiB):\n",
                    g_color_dialog_title[g_curr_theme]);
            vdisk_progress = newCDKHistogram(main_cdk_screen, CENTER, CENTER,
                    1, 50, HORIZONTAL, histogram_title, TRUE, FALSE);
            if (!vdisk_progress) {
                errorDialog(main_cdk_screen, HISTOGRAM_ERR_MSG, NULL);
                break;
            }
            setCDKScrollBoxAttribute(vdisk_progress,
                    g_color_dialog_box[g_curr_theme]);
            setCDKScrollBackgroundAttrib(vdisk_progress,
                    g_color_dialog_text[g_curr_theme]);
            drawCDKHistogram(vdisk_progress, TRUE);

            /* We'll use mebibyte as our unit for the histogram widget; need to
             * make sure an int can handle it, which is used by the
             * histogram widget */
            new_vdisk_mib = new_vdisk_bytes / MEBIBYTE_SIZE;
            if (new_vdisk_mib > INT_MAX) {
                errorDialog(main_cdk_screen, "An integer will not hold "
                        "the virtual", "disk size (MiB) on this system!");
                break;
            }

            /* Open our (new) virtual disk file */
            if ((new_vdisk_fd = open(new_vdisk_file,
                    O_WRONLY | O_CREAT, 0666)) == -1) {
                SAFE_ASPRINTF(&error_msg, "open(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }

            /* Zero-out the new virtual disk file to the
             * length specified (size) */
            memset(zero_buff, 0, VDISK_WRITE_SIZE);
            for (position = 0; position < new_vdisk_bytes;
                    position += write_length) {
                write_length = MIN((new_vdisk_bytes - position),
                        VDISK_WRITE_SIZE);
                /* Use fallocate() for modern file systems, and
                 * write() for others */
                if ((strcmp(fs_type, "xfs") == 0) ||
                        (strcmp(fs_type, "ext4") == 0) ||
                        (strcmp(fs_type, "btrfs") == 0)) {
                    if (fallocate(new_vdisk_fd, 0, position,
                            write_length) == -1) {
                        SAFE_ASPRINTF(&error_msg, "fallocate(): %s",
                                strerror(errno));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        close(new_vdisk_fd);
                        finished = TRUE;
                        break;
                    }
                } else {
                    bytes_written = write(new_vdisk_fd, zero_buff,
                            write_length);
                    if (bytes_written == -1) {
                        SAFE_ASPRINTF(&error_msg, "write(): %s",
                                strerror(errno));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        close(new_vdisk_fd);
                        finished = TRUE;
                        break;
                    } else if (bytes_written != write_length) {
                        errorDialog(main_cdk_screen,
                                "The number of bytes written is different",
                                "than the requested write length!");
                        close(new_vdisk_fd);
                        finished = TRUE;
                        break;
                    }
                }
                /* This controls how often the progress bar is updated; it can
                 * adversely affect performance of the write operation if its
                 * updated too frequently */
                if ((position % (VDISK_WRITE_SIZE * 1000)) == 0) {
                    /* Since our maximum size was checked above against an int
                     * type, we'll assume we're safe if we made it this far */
                    setCDKHistogram(vdisk_progress, vPERCENT, CENTER,
                            g_color_dialog_text[g_curr_theme], 0, new_vdisk_mib,
                            (position / MEBIBYTE_SIZE), ' ' | A_REVERSE, TRUE);
                    drawCDKHistogram(vdisk_progress, TRUE);
                }
            }
            if (finished)
                break;

            /* We've completed writing the new file; update the progress bar */
            setCDKHistogram(vdisk_progress, vPERCENT, CENTER,
                    g_color_dialog_text[g_curr_theme],
                    0, new_vdisk_mib, new_vdisk_mib,
                    ' ' | A_REVERSE, TRUE);
            drawCDKHistogram(vdisk_progress, TRUE);

            // TODO: Do we actually need to flush to disk before returning?
            if (fsync(new_vdisk_fd) == -1) {
                SAFE_ASPRINTF(&error_msg, "fsync(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                close(new_vdisk_fd);
                break;
            }
            if (close(new_vdisk_fd) == -1) {
                SAFE_ASPRINTF(&error_msg, "close(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
            destroyCDKHistogram(vdisk_progress);
            vdisk_progress = NULL;
        }
        break;
    }

    /* Done */
    FREE_NULL(histogram_title);
    for (i = 0; i < ADD_VDISK_INFO_LINES; i++)
        FREE_NULL(vdisk_dialog_msg[i]);
    if (vdisk_progress != NULL)
        destroyCDKHistogram(vdisk_progress);
    if (vdisk_screen != NULL) {
        destroyCDKScreenObjects(vdisk_screen);
        destroyCDKScreen(vdisk_screen);
    }
    delwin(vdisk_window);
    return;
}


/**
 * @brief Run the "Delete Virtual Disk File" dialog.
 */
void delVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    CDKFSELECT *file_select = 0;
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0}, mount_cmd[MAX_SHELL_CMD_LEN] = {0};
    char *error_msg = NULL, *selected_file = NULL, *confirm_msg = NULL,
            *fselect_title = NULL;
    boolean mounted = FALSE, question = FALSE, confirm = FALSE;
    int exit_stat = 0, ret_val = 0;

    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    if (!mounted) {
        question = questionDialog(main_cdk_screen,
                NOT_MOUNTED_1, NOT_MOUNTED_2);
        if (question) {
            /* Run mount */
            snprintf(mount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                    MOUNT_BIN, fs_path);
            ret_val = system(mount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MOUNT_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                return;
            }
        } else {
            return;
        }
    }

    /* Create the file selector widget */
    SAFE_ASPRINTF(&fselect_title,
            "<C></%d/B>Choose a virtual disk file to delete:\n",
            g_color_dialog_title[g_curr_theme]);
    file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER, 20, 40,
            fselect_title, "VDisk File: ", g_color_dialog_input[g_curr_theme],
            '_' | g_color_dialog_input[g_curr_theme],
            A_REVERSE, "</N>", "</B>", "</N>", "</N>", TRUE, FALSE);
    if (!file_select) {
        errorDialog(main_cdk_screen, FSELECT_ERR_MSG, NULL);
        return;
    }
    setCDKFselectBoxAttribute(file_select, g_color_dialog_box[g_curr_theme]);
    setCDKFselectBackgroundAttrib(file_select,
            g_color_dialog_text[g_curr_theme]);
    setCDKFselectDirectory(file_select, fs_path);

    /* Activate the widget and let the user choose a file */
    selected_file = activateCDKFselect(file_select, 0);
    if (file_select->exitType == vNORMAL) {
        /* Get confirmation before deleting the virtual disk file */
        SAFE_ASPRINTF(&confirm_msg, "'%s'?", selected_file);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to delete virtual disk file",
                confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Delete (unlink) the virtual disk file */
            if (unlink(selected_file) == -1) {
                SAFE_ASPRINTF(&error_msg, "unlink(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
    }

    /* Done */
    destroyCDKFselect(file_select);
    /* Using the file selector widget changes the CWD -- fix it */
    if ((chdir(getenv("HOME"))) == -1) {
        SAFE_ASPRINTF(&error_msg, "chdir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    }
    FREE_NULL(fselect_title);
    return;
}


/**
 * @brief Run the "Virtual Disk File List" dialog.
 */
void vdiskFileListDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *vdisk_files = 0;
    char *swindow_info[MAX_VDLIST_INFO_LINES] = {NULL};
    char *error_msg = NULL, *pretty_size = NULL;
    int i = 0, line_pos = 0;
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0},
            vd_list_title[VDLIST_INFO_COLS] = {0},
            file_path[MAX_SYSFS_PATH_SIZE] = {0};
    boolean mounted = FALSE;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    struct stat file_stat = {0};

    /* Have the user select a file system */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    /* Open the directory */
    if ((dir_stream = opendir(fs_path)) == NULL) {
        SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
        return;
    }

    /* Setup scrolling window widget */
    snprintf(vd_list_title, VDLIST_INFO_COLS,
            "<C></%d/B>Virtual Disk File List (%.25s)\n",
            g_color_dialog_title[g_curr_theme], fs_path);
    vdisk_files = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (VDLIST_INFO_ROWS + 2), (VDLIST_INFO_COLS + 2),
            vd_list_title, MAX_VDLIST_INFO_LINES, TRUE, FALSE);
    if (!vdisk_files) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(vdisk_files,
            g_color_dialog_text[g_curr_theme]);
    setCDKSwindowBoxAttribute(vdisk_files, g_color_dialog_box[g_curr_theme]);

    /* Loop over each entry in the directory */
    if (line_pos < MAX_VDLIST_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], "<C></B>%-20.20s %15.15s",
                "File Name", "File Size");
        line_pos++;
    }
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* We only want the files */
        if (dir_entry->d_type != DT_DIR) {
            /* Add the contents to the scrolling window widget */
            if (line_pos < MAX_VDLIST_INFO_LINES) {
                snprintf(file_path, MAX_SYSFS_PATH_SIZE, "%s/%s",
                        fs_path, dir_entry->d_name);
                if (stat(file_path, &file_stat) == -1) {
                    SAFE_ASPRINTF(&error_msg, "stat(): %s", strerror(errno));
                    SAFE_ASPRINTF(&swindow_info[line_pos],
                            "<C>%-20.20s %15.15s",
                            dir_entry->d_name, error_msg);
                    FREE_NULL(error_msg);
                } else {
                    pretty_size = prettyFormatBytes(file_stat.st_size);
                    SAFE_ASPRINTF(&swindow_info[line_pos],
                            "<C>%-20.20s %15.15s",
                            dir_entry->d_name, pretty_size);
                    FREE_NULL(pretty_size);
                }
                line_pos++;
            }
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_VDLIST_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], " ");
        line_pos++;
    }
    if (line_pos < MAX_VDLIST_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;
    }

    /* Set the scrolling window content */
    setCDKSwindowContents(vdisk_files, swindow_info, line_pos);

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(vdisk_files, 'g');
    activateCDKSwindow(vdisk_files, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(vdisk_files);

    /* Done */
    for (i = 0; i < MAX_VDLIST_INFO_LINES; i++)
        FREE_NULL(swindow_info[i]);
    return;
}
