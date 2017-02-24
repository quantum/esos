/**
 * @file menu_softraid.c
 * @brief Contains the menu actions for the 'Software RAID' menu.
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
 * @brief Present the user with a list of usable block devices detected on
 * the system, and let them select any number of devices. The list of selected
 * block devices are passed by reference as a single list of strings and this
 * value is set after a selection is made. We return the number of block
 * devices selected on success, and -1 if an error occurred. This function will
 * handle displaying any errors that are encountered.
 */
int getBlockDevSelection(CDKSCREEN *cdk_screen,
        char blk_dev_list[MAX_BLOCK_DEVS][MISC_STRING_LEN]) {
    CDKSELECTION *blk_dev_select = 0;
    char *blk_dev_select_title = NULL;
    char blk_dev_name[MAX_BLOCK_DEVS][MISC_STRING_LEN] = {{0}, {0}},
            blk_dev_info[MAX_BLOCK_DEVS][MISC_STRING_LEN] = {{0}, {0}},
            blk_dev_size[MAX_BLOCK_DEVS][MISC_STRING_LEN] = {{0}, {0}};
    char *selection_list[MAX_HWRAID_PDRVS] = {NULL};
    int dev_cnt, i = 0, chosen_dev_cnt = 0;
    boolean user_quit = FALSE;

    /* Get the usable block devices on this system */
    if ((dev_cnt = getUsableBlockDevs(cdk_screen, blk_dev_name,
            blk_dev_info, blk_dev_size)) == -1)
        return -1;

    /* Make sure we actually have something to present */
    if (dev_cnt == 0) {
        errorDialog(cdk_screen, "No block devices found!", NULL);
        return -1;
    }

    /* Fill the list (pretty) for our CDK label with block devices */
    for (i = 0; i < dev_cnt; i++) {
        if (i < MAX_BLOCK_DEVS) {
            SAFE_ASPRINTF(&selection_list[i],
                    "<C>%-8.8s %-26.26s Size: %-14.14s",
                    blk_dev_name[i], blk_dev_info[i], blk_dev_size[i]);
        }
    }

    while (1) {
        /* Selection widget for block devices */
        SAFE_ASPRINTF(&blk_dev_select_title, "<C></%d/B>Select Block Devices\n",
                g_color_dialog_title[g_curr_theme]);
        blk_dev_select = newCDKSelection(cdk_screen, CENTER, CENTER, NONE,
                14, 64, blk_dev_select_title, selection_list, dev_cnt,
                g_choice_char, 2, g_color_dialog_select[g_curr_theme],
                TRUE, FALSE);
        if (!blk_dev_select) {
            errorDialog(cdk_screen, SELECTION_ERR_MSG, NULL);
            break;
        }
        setCDKSelectionBoxAttribute(blk_dev_select,
                g_color_dialog_box[g_curr_theme]);
        setCDKSelectionBackgroundAttrib(blk_dev_select,
                g_color_dialog_text[g_curr_theme]);

        /* Activate the widget */
        activateCDKSelection(blk_dev_select, 0);

        /* User hit escape, so we get out of this */
        if (blk_dev_select->exitType == vESCAPE_HIT) {
            user_quit = TRUE;
            destroyCDKSelection(blk_dev_select);
            refreshCDKScreen(cdk_screen);
            break;

        /* User hit return/tab so we can continue on
         * and get what was selected */
        } else if (blk_dev_select->exitType == vNORMAL) {
            chosen_dev_cnt = 0;
            for (i = 0; i < dev_cnt; i++) {
                if (blk_dev_select->selections[i] == 1) {
                    snprintf(blk_dev_list[chosen_dev_cnt], MISC_STRING_LEN,
                            "%s", blk_dev_name[i]);
                    chosen_dev_cnt++;
                }
            }
            destroyCDKSelection(blk_dev_select);
            refreshCDKScreen(cdk_screen);
        }

        /* Check and make sure some devices were actually selected */
        if (chosen_dev_cnt == 0) {
            errorDialog(cdk_screen, "No block devices selected!", NULL);
            break;
        }
        break;
    }

    /* Done */
    FREE_NULL(blk_dev_select_title);
    for (i = 0; i < dev_cnt; i++)
        FREE_NULL(selection_list[i]);
    if (!user_quit && chosen_dev_cnt > 0)
        return chosen_dev_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of MD software RAID arrays that are
 * running, and let them choose one. The MD array level, device count, metadata
 * version, and device path are passed by reference and these values are set
 * after a selection is made. We return the number of MD arrays found on
 * success, and -1 if an error occurred. This function will handle displaying
 * any errors that are encountered.
 */
int getMDArrayChoice(CDKSCREEN *cdk_screen, char level[], char dev_cnt[],
        char metadata[], char dev_path[]) {
    CDKSCROLL *array_scroll = 0;
    char *md_levels[MAX_MD_ARRAYS] = {NULL},
            *md_dev_cnts[MAX_MD_ARRAYS] = {NULL},
            *md_metadatas[MAX_MD_ARRAYS] = {NULL},
            *md_dev_paths[MAX_MD_ARRAYS] = {NULL},
            *scroll_list[MAX_MD_ARRAYS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int array_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --detail --scan --export 2>&1",
                MDADM_BIN);
        shell_cmd = popen(command_str, "r");
        if (!shell_cmd) {
            SAFE_ASPRINTF(&error_msg, "popen(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        } else {
            /* Loop over the command output and parse the lines */
            while (fgets(output_line, sizeof (output_line),
                    shell_cmd) != NULL) {
                if (array_cnt < MAX_MD_ARRAYS) {
                    if (strstr(output_line, "MD_LEVEL=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_levels[array_cnt], "%s",
                                strStrip(strtok_result));
                    } else if (strstr(output_line, "MD_DEVICES=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_dev_cnts[array_cnt], "%s",
                                strStrip(strtok_result));
                    } else if (strstr(output_line, "MD_METADATA=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_metadatas[array_cnt], "%s",
                                strStrip(strtok_result));
                    } else if (strstr(output_line, "MD_DEVNAME=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_dev_paths[array_cnt], "/dev/md/%s",
                                strStrip(strtok_result));
                        /* Last line to find for an array, so add it here */
                        SAFE_ASPRINTF(&scroll_list[array_cnt],
                                "<C>%-16.16s Level: %-10.10s Devices: %-4.4s "
                                "Metadata: %-6.6s", md_dev_paths[array_cnt],
                                md_levels[array_cnt], md_dev_cnts[array_cnt],
                                md_metadatas[array_cnt]);
                        array_cnt++;
                    }
                }
            }
            status = pclose(shell_cmd);
            if (status == -1) {
                SAFE_ASPRINTF(&error_msg, "pclose(): %s", strerror(errno));
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            } else {
                if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN,
                            WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (array_cnt == 0) {
            errorDialog(cdk_screen, "No running MD arrays were detected.",
                    NULL);
            break;
        }

        /* Get MD array choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a MD RAID Array\n",
                g_color_dialog_title[g_curr_theme]);
        array_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                scroll_title, scroll_list, array_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!array_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(array_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(array_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(array_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (array_scroll->exitType == vNORMAL) {
            snprintf(level, MISC_STRING_LEN, "%s",
                    md_levels[user_choice]);
            snprintf(dev_cnt, MISC_STRING_LEN, "%s",
                    md_dev_cnts[user_choice]);
            snprintf(metadata, MISC_STRING_LEN, "%s",
                    md_metadatas[user_choice]);
            snprintf(dev_path, MISC_STRING_LEN, "%s",
                    md_dev_paths[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(array_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < array_cnt; i++) {
        FREE_NULL(md_levels[i]);
        FREE_NULL(md_dev_cnts[i]);
        FREE_NULL(md_metadatas[i]);
        FREE_NULL(md_dev_paths[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && array_cnt > 0)
        return array_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of MD array member devices for the given
 * MD array device path, and let them choose one. The MD member device itself
 * (block device) and role number/ID are passed by reference and these values
 * are set after a selection is made. We return the number of member devices
 * found on success, and -1 if an error occurred. This function will handle
 * displaying any errors that are encountered.
 */
int getMDMemberDevChoice(CDKSCREEN *cdk_screen, char dev_path[],
        char member_dev[], char member_role[]) {
    CDKSCROLL *member_dev_scroll = 0;
    char *md_member_devs[MAX_MD_MEMBERS] = {NULL},
            *md_member_roles[MAX_MD_MEMBERS] = {NULL},
            *scroll_list[MAX_MD_MEMBERS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int dev_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --detail %s --export 2>&1", MDADM_BIN, dev_path);
        shell_cmd = popen(command_str, "r");
        if (!shell_cmd) {
            SAFE_ASPRINTF(&error_msg, "popen(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        } else {
            /* Loop over the command output and parse the lines */
            while (fgets(output_line, sizeof (output_line),
                    shell_cmd) != NULL) {
                if (dev_cnt < MAX_MD_MEMBERS) {
                    if (strstr(output_line, "_ROLE=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_member_roles[dev_cnt], "%s",
                                strStrip(strtok_result));
                    } else if (strstr(output_line, "_DEV=") != NULL) {
                        strtok_result = strtok(output_line, "=");
                        strtok_result = strtok(NULL, "=");
                        SAFE_ASPRINTF(&md_member_devs[dev_cnt], "%s",
                                strStrip(strtok_result));
                        /* Last line to find for a device, so add it here */
                        SAFE_ASPRINTF(&scroll_list[dev_cnt],
                                "<C>Block Device: %-16.16s Role: %-4.4s",
                                md_member_devs[dev_cnt],
                                md_member_roles[dev_cnt]);
                        dev_cnt++;
                    }
                }
            }
            status = pclose(shell_cmd);
            if (status == -1) {
                SAFE_ASPRINTF(&error_msg, "pclose(): %s", strerror(errno));
                errorDialog(cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            } else {
                if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN,
                            WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (dev_cnt == 0) {
            errorDialog(cdk_screen,
                    "No member devices were found for the MD array.", NULL);
            break;
        }

        /* Get array member device choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a Member Device (%s)\n",
                g_color_dialog_title[g_curr_theme], dev_path);
        member_dev_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE,
                15, 70, scroll_title, scroll_list, dev_cnt, FALSE,
                g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!member_dev_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(member_dev_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(member_dev_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(member_dev_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (member_dev_scroll->exitType == vNORMAL) {
            snprintf(member_dev, MISC_STRING_LEN, "%s",
                    md_member_devs[user_choice]);
            snprintf(member_role, MISC_STRING_LEN, "%s",
                    md_member_roles[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(member_dev_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < dev_cnt; i++) {
        FREE_NULL(md_member_devs[i]);
        FREE_NULL(md_member_roles[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && dev_cnt > 0)
        return dev_cnt;
    else
        return -1;
}


/**
 * @brief Run the "Software RAID Status" dialog.
 */
void softRAIDStatDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *mdstat_info = 0;
    char *swindow_info[MAX_MDSTAT_INFO_LINES] = {NULL};
    char *error_msg = NULL, *swindow_title = NULL;
    int i = 0, line_pos = 0;
    char line[MDSTAT_INFO_COLS] = {0};
    FILE *mdstat_file = NULL;

    /* Open the file */
    if ((mdstat_file = fopen(PROC_MDSTAT, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg, "fopen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    } else {
        /* Setup scrolling window widget */
        SAFE_ASPRINTF(&swindow_title,
                "<C></%d/B>Linux Software RAID (md) Status\n",
                g_color_dialog_title[g_curr_theme]);
        mdstat_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                (MDSTAT_INFO_ROWS + 2), (MDSTAT_INFO_COLS + 2),
                swindow_title, MAX_MDSTAT_INFO_LINES, TRUE, FALSE);
        if (!mdstat_info) {
            errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
            return;
        }
        setCDKSwindowBackgroundAttrib(mdstat_info,
                g_color_dialog_text[g_curr_theme]);
        setCDKSwindowBoxAttribute(mdstat_info,
                g_color_dialog_box[g_curr_theme]);

        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), mdstat_file) != NULL) {
            if (line_pos < MAX_MDSTAT_INFO_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }
        fclose(mdstat_file);

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_MDSTAT_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_MDSTAT_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Set the scrolling window content */
        setCDKSwindowContents(mdstat_info, swindow_info, line_pos);

        /* The 'g' makes the swindow widget scroll to the top, then activate */
        injectCDKSwindow(mdstat_info, 'g');
        activateCDKSwindow(mdstat_info, 0);

        /* We fell through -- the user exited the widget, but
         * we don't care how */
        destroyCDKSwindow(mdstat_info);
    }

    /* Done */
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_MDSTAT_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}


/**
 * @brief Run the "Add Array" dialog.
 */
void addArrayDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *new_array_window = 0;
    CDKSCREEN *new_array_screen = 0;
    CDKLABEL *new_array_label = 0, *add_array_msg = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKRADIO *chunk_size = 0, *raid_lvl = 0;
    CDKENTRY *array_name = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int chosen_dev_cnt = 0, i = 0, ret_val = 0, exit_stat = 0,
            traverse_ret = 0, window_y = 0, window_x = 0,
            new_array_window_lines = 0, new_array_window_cols = 0,
            dev_info_size = 0, dev_info_line_size = 0;
    char *error_msg = NULL, *temp_pstr = NULL;
    char *new_array_msg[NEW_ARRAY_INFO_LINES] = {NULL};
    char blk_dev_list[MAX_MD_MEMBERS][MISC_STRING_LEN] = {0, 0},
            dev_info_line_buffer[MAX_DEV_INFO_LINE_BUFF] = {0},
            array_name_str[MISC_STRING_LEN] = {0},
            raid_lvl_str[MISC_STRING_LEN] = {0},
            chunk_size_str[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    boolean finished = FALSE;

    /* The user first needs to select the block devices to use */
    if ((chosen_dev_cnt = getBlockDevSelection(main_cdk_screen,
            blk_dev_list)) == -1) {
        return;
    }

    while (1) {
        /* Setup a new CDK screen for required input (to create new array) */
        new_array_window_lines = 18;
        new_array_window_cols = 60;
        window_y = ((LINES / 2) - (new_array_window_lines / 2));
        window_x = ((COLS / 2) - (new_array_window_cols / 2));
        new_array_window = newwin(new_array_window_lines, new_array_window_cols,
                window_y, window_x);
        if (new_array_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        new_array_screen = initCDKScreen(new_array_window);
        if (new_array_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(new_array_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(new_array_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(new_array_window);

        /* Put selected block_device information into a string */
        for (i = 0; i < chosen_dev_cnt; i++) {
            if (i == (chosen_dev_cnt - 1))
                SAFE_ASPRINTF(&temp_pstr, "/dev/%s", blk_dev_list[i]);
            else
                SAFE_ASPRINTF(&temp_pstr, "/dev/%s ", blk_dev_list[i]);
            /* We add one extra for the null byte */
            dev_info_size = strlen(temp_pstr) + 1;
            dev_info_line_size = dev_info_line_size + dev_info_size;
            if (dev_info_line_size >= MAX_DEV_INFO_LINE_BUFF) {
                errorDialog(main_cdk_screen, "The maximum device information "
                        "line buffer size has been reached!", NULL);
                FREE_NULL(temp_pstr);
                finished = TRUE;
                break;
            } else {
                strcat(dev_info_line_buffer, temp_pstr);
                FREE_NULL(temp_pstr);
            }
        }
        if (finished)
            break;

        /* Make a new label for the add-MD-array screen */
        SAFE_ASPRINTF(&new_array_msg[0], "</%d/B>Creating new MD RAID array...",
                g_color_dialog_title[g_curr_theme]);
        SAFE_ASPRINTF(&new_array_msg[1], " ");
        SAFE_ASPRINTF(&new_array_msg[2], "Selected Block Devices - %.32s",
                prettyShrinkStr(32, dev_info_line_buffer));
        new_array_label = newCDKLabel(new_array_screen, (window_x + 1),
                (window_y + 1), new_array_msg, NEW_ARRAY_INFO_LINES,
                FALSE, FALSE);
        if (!new_array_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(new_array_label,
                g_color_dialog_text[g_curr_theme]);

        /* Array name */
        array_name = newCDKEntry(new_array_screen, (window_x + 1),
                (window_y + 5), NULL, "</B>Array Name: ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vLMIXED, 20,
                0, MAX_ARRAY_NAME_LEN, FALSE, FALSE);
        if (!array_name) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(array_name, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(array_name,
                    g_color_dialog_text[g_curr_theme]);

        /* RAID level radio list */
        raid_lvl = newCDKRadio(new_array_screen, (window_x + 1), (window_y + 7),
                NONE, 7, 10, "</B>RAID Level", g_md_level_opts, 6,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!raid_lvl) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(raid_lvl,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(raid_lvl, 0);

        /* Chunk size radio list */
        chunk_size = newCDKRadio(new_array_screen, (window_x + 16),
                (window_y + 7), NONE, 7, 11, "</B>Chunk Size", g_md_chunk_opts,
                6, '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!chunk_size) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(chunk_size,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(chunk_size, 3);

        /* Buttons */
        ok_button = newCDKButton(new_array_screen, (window_x + 20),
                (window_y + 16), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(new_array_screen, (window_x + 30),
                (window_y + 16), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(new_array_screen);
        traverse_ret = traverseCDKScreen(new_array_screen);
        break;
    }

    /* We need these below */
    snprintf(array_name_str, MISC_STRING_LEN, "%s",
            getCDKEntryValue(array_name));
    snprintf(raid_lvl_str, MISC_STRING_LEN, "%s",
            g_md_level_opts[getCDKRadioSelectedItem(raid_lvl)]);
    snprintf(chunk_size_str, MISC_STRING_LEN, "%s",
            g_md_chunk_opts[getCDKRadioSelectedItem(chunk_size)]);

    /* Cleanup */
    for (i = 0; i < NEW_ARRAY_INFO_LINES; i++)
        FREE_NULL(new_array_msg[i]);
    if (new_array_screen != NULL) {
        destroyCDKScreenObjects(new_array_screen);
        destroyCDKScreen(new_array_screen);
    }
    delwin(new_array_window);
    refreshCDKScreen(main_cdk_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Make sure the entry field value is valid */
        if (!checkInputStr(main_cdk_screen, NAME_CHARS, array_name_str))
            return;

        /* Display a label message while adding the array */
        add_array_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
                g_add_array_label_msg, g_add_array_label_msg_size(),
                TRUE, FALSE);
        if (!add_array_msg) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            return;
        }
        setCDKLabelBackgroundAttrib(add_array_msg,
                g_color_dialog_text[g_curr_theme]);
        setCDKLabelBoxAttribute(add_array_msg,
                g_color_dialog_box[g_curr_theme]);
        refreshCDKScreen(main_cdk_screen);

        /* Add the new MD array */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --create --run "
                "/dev/md/%s --name=%s --level=%s --raid-devices=%d "
                "--chunk=%s %s > /dev/null 2>&1", MDADM_BIN, array_name_str,
                array_name_str, raid_lvl_str, chosen_dev_cnt, chunk_size_str,
                dev_info_line_buffer);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    destroyCDKLabel(add_array_msg);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Run the "Remove Array" dialog.
 */
void remArrayDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char md_level[MISC_STRING_LEN] = {0},
            md_dev_cnt[MISC_STRING_LEN] = {0},
            md_metadata[MISC_STRING_LEN] = {0},
            md_dev_path[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int array_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Let the user pick a MD array */
    if ((array_cnt = getMDArrayChoice(main_cdk_screen, md_level, md_dev_cnt,
            md_metadata, md_dev_path)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before we stop the array */
        SAFE_ASPRINTF(&confirm_msg, "array '%s' (Level: %s)?",
                md_dev_path, md_level);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to remove (stop) MD", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Stop the existing MD array */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s --stop %s > /dev/null 2>&1", MDADM_BIN, md_dev_path);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Set Device Faulty" dialog.
 */
void faultDevDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char md_level[MISC_STRING_LEN] = {0},
            md_dev_cnt[MISC_STRING_LEN] = {0},
            md_metadata[MISC_STRING_LEN] = {0},
            md_dev_path[MISC_STRING_LEN] = {0},
            md_member_dev[MISC_STRING_LEN] = {0},
            md_member_role[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int array_cnt = 0, member_dev_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Have the user pick a MD array */
    if ((array_cnt = getMDArrayChoice(main_cdk_screen, md_level, md_dev_cnt,
            md_metadata, md_dev_path)) == -1) {
        return;
    }

    /* Now they can pick a member device */
    if ((member_dev_cnt = getMDMemberDevChoice(main_cdk_screen, md_dev_path,
            md_member_dev, md_member_role)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before failing the device */
        SAFE_ASPRINTF(&confirm_msg, "'%s' (Role: %s) as faulty?",
                md_member_dev, md_member_role);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to mark device", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Mark the array member device as faulty */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s --fail %s %s > /dev/null 2>&1",
                    MDADM_BIN, md_dev_path, md_member_dev);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Device" dialog.
 */
void addDevDialog(CDKSCREEN *main_cdk_screen) {
    char *block_dev = NULL, *error_msg = NULL;
    char md_level[MISC_STRING_LEN] = {0},
            md_dev_cnt[MISC_STRING_LEN] = {0},
            md_metadata[MISC_STRING_LEN] = {0},
            md_dev_path[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int array_cnt = 0, ret_val = 0, exit_stat = 0;

    /* Have the user pick a MD array */
    if ((array_cnt = getMDArrayChoice(main_cdk_screen, md_level, md_dev_cnt,
            md_metadata, md_dev_path)) == -1) {
        return;
    }

    /* Now they can pick a new block device to add */
    if ((block_dev = getBlockDevChoice(main_cdk_screen)) == NULL) {
        return;
    }

    while (1) {
        /* Add the block device to the existing array */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --add %s %s > /dev/null 2>&1",
                MDADM_BIN, md_dev_path, block_dev);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
        break;
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Remove Device" dialog.
 */
void remDevDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char md_level[MISC_STRING_LEN] = {0},
            md_dev_cnt[MISC_STRING_LEN] = {0},
            md_metadata[MISC_STRING_LEN] = {0},
            md_dev_path[MISC_STRING_LEN] = {0},
            md_member_dev[MISC_STRING_LEN] = {0},
            md_member_role[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int array_cnt = 0, member_dev_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Have the user pick a MD array */
    if ((array_cnt = getMDArrayChoice(main_cdk_screen, md_level, md_dev_cnt,
            md_metadata, md_dev_path)) == -1) {
        return;
    }

    /* Now they can pick a member device */
    if ((member_dev_cnt = getMDMemberDevChoice(main_cdk_screen, md_dev_path,
            md_member_dev, md_member_role)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before removing the device */
        SAFE_ASPRINTF(&confirm_msg, "'%s' (Role: %s) from the array?",
                md_member_dev, md_member_role);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to remove device", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Remove the device from the MD array */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s --remove %s %s > /dev/null 2>&1",
                    MDADM_BIN, md_dev_path, md_member_dev);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, MDADM_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }
        break;
    }

    /* Done */
    return;
}
