/**
 * @file menu_lvm.c
 * @brief Contains the menu actions for the 'LVM' menu.
 * @author Copyright (c) 2019 Quantum Corporation
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
 * @brief Present the user with a list of LVM physical volumes (PV's) that are
 * present on the system, and let them select any number of PV's. The PV names
 * are passed by reference as a single list of strings and this value is set
 * after a selection is made. We return the number of LVM PV's selected on
 * success, and -1 if an error occurred. This function will handle displaying
 * any errors that are encountered.
 */
int getPVSelection(CDKSCREEN *cdk_screen, boolean avail_only,
        char pv_name_list[MAX_LVM_PVS][MISC_STRING_LEN]) {
    CDKSELECTION *pv_select = 0;
    char *strtok_result = NULL, *error_msg = NULL, *pv_select_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    char *pv_names[MAX_LVM_PVS] = {NULL},
            *pv_sizes[MAX_LVM_PVS] = {NULL},
            *vg_names[MAX_LVM_PVS] = {NULL},
            *selection_list[MAX_LVM_PVS] = {NULL};
    int pv_cnt = 0, i = 0, status = 0, chosen_pv_cnt = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --separator , "
                "--noheadings --options pv_name,pv_size,vg_name %s 2>&1",
                PVS_BIN, (avail_only ? "--select 'pv_pe_count=0'" : ""));
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
                if (pv_cnt < MAX_LVM_PVS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    SAFE_ASPRINTF(&pv_names[pv_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pv_sizes[pv_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&vg_names[pv_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&selection_list[pv_cnt],
                            "<C>%-12.12s Size: %-10.10s VG: %-12.12s",
                            pv_names[pv_cnt], pv_sizes[pv_cnt],
                            (vg_names[pv_cnt][0] == '\0' ?
                                "N/A" : vg_names[pv_cnt]));
                    pv_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                            PVS_BIN, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (pv_cnt == 0) {
            errorDialog(cdk_screen,
                    "No LVM physical volumes were detected on this system.",
                    NULL);
            break;
        }

        /* Selection widget for physical volumes */
        SAFE_ASPRINTF(&pv_select_title,
                "<C></%d/B>Select LVM Physical Volumes\n",
                g_color_dialog_title[g_curr_theme]);
        pv_select = newCDKSelection(cdk_screen, CENTER, CENTER, NONE,
                14, 58, pv_select_title, selection_list, pv_cnt,
                g_choice_char, 2, g_color_dialog_select[g_curr_theme],
                TRUE, FALSE);
        if (!pv_select) {
            errorDialog(cdk_screen, SELECTION_ERR_MSG, NULL);
            break;
        }
        setCDKSelectionBoxAttribute(pv_select,
                g_color_dialog_box[g_curr_theme]);
        setCDKSelectionBackgroundAttrib(pv_select,
                g_color_dialog_text[g_curr_theme]);

        /* Activate the widget */
        activateCDKSelection(pv_select, 0);

        /* User hit escape, so we get out of this */
        if (pv_select->exitType == vESCAPE_HIT) {
            user_quit = TRUE;
            destroyCDKSelection(pv_select);
            refreshCDKScreen(cdk_screen);
            break;

        /* User hit return/tab so we can continue on and get what was selected */
        } else if (pv_select->exitType == vNORMAL) {
            chosen_pv_cnt = 0;
            for (i = 0; i < pv_cnt; i++) {
                if (pv_select->selections[i] == 1) {
                    snprintf(pv_name_list[chosen_pv_cnt], MISC_STRING_LEN,
                            "%s", pv_names[i]);
                    chosen_pv_cnt++;
                }
            }
            destroyCDKSelection(pv_select);
            refreshCDKScreen(cdk_screen);
        }

        /* Check and make sure some PV's were actually selected */
        if (chosen_pv_cnt == 0) {
            errorDialog(cdk_screen, "No physical volumes selected!", NULL);
            break;
        }
        break;
    }

    /* Done */
    FREE_NULL(pv_select_title);
    for (i = 0; i < pv_cnt; i++) {
        FREE_NULL(pv_names[i]);
        FREE_NULL(pv_sizes[i]);
        FREE_NULL(vg_names[i]);
        FREE_NULL(selection_list[i]);
    }
    if (!user_quit && chosen_pv_cnt > 0)
        return chosen_pv_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of LVM volume groups (VG's) that are
 * present on the system and let them choose one. The volume group (VG) name,
 * size, free space, PV count, and LV count are passed by reference and these
 * values are set after a selection is made. We return the number of volume
 * groups found on success, and -1 if an error occurred. This function will
 * handle displaying any errors that are encountered.
 */
int getVGChoice(CDKSCREEN *cdk_screen, char name[], char size[],
        char free_space[], char pv_cnt[], char lv_cnt[]) {
    CDKSCROLL *vg_scroll = 0;
    char *vg_names[MAX_LVM_VGS] = {NULL},
            *vg_sizes[MAX_LVM_VGS] = {NULL},
            *vg_free_spaces[MAX_LVM_VGS] = {NULL},
            *vg_pv_cnts[MAX_LVM_VGS] = {NULL},
            *vg_lv_cnts[MAX_LVM_VGS] = {NULL},
            *scroll_list[MAX_LVM_VGS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int vg_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --separator , --noheadings --options "
                "vg_name,vg_size,vg_free,pv_count,lv_count 2>&1", VGS_BIN);
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
                if (vg_cnt < MAX_LVM_VGS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    SAFE_ASPRINTF(&vg_names[vg_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&vg_sizes[vg_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&vg_free_spaces[vg_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&vg_pv_cnts[vg_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&vg_lv_cnts[vg_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&scroll_list[vg_cnt],
                            "<C>%-12.12s Size: %-8.8s Free: %-8.8s "
                            "PV's: %-3.3s LV's: %-3.3s",
                            vg_names[vg_cnt], vg_sizes[vg_cnt],
                            vg_free_spaces[vg_cnt], vg_pv_cnts[vg_cnt],
                            vg_lv_cnts[vg_cnt]);
                    vg_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                            VGS_BIN, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (vg_cnt == 0) {
            errorDialog(cdk_screen,
                    "No LVM volume groups were detected on this system.",
                    NULL);
            break;
        }

        /* Get logical drive choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a LVM Volume Group\n",
                g_color_dialog_title[g_curr_theme]);
        vg_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                scroll_title, scroll_list, vg_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!vg_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(vg_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(vg_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(vg_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (vg_scroll->exitType == vNORMAL) {
            snprintf(name, MISC_STRING_LEN, "%s",
                    vg_names[user_choice]);
            snprintf(size, MISC_STRING_LEN, "%s",
                    vg_sizes[user_choice]);
            snprintf(free_space, MISC_STRING_LEN, "%s",
                    vg_free_spaces[user_choice]);
            snprintf(pv_cnt, MISC_STRING_LEN, "%s",
                    vg_pv_cnts[user_choice]);
            snprintf(lv_cnt, MISC_STRING_LEN, "%s",
                    vg_lv_cnts[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(vg_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < vg_cnt; i++) {
        FREE_NULL(vg_names[i]);
        FREE_NULL(vg_sizes[i]);
        FREE_NULL(vg_free_spaces[i]);
        FREE_NULL(vg_pv_cnts[i]);
        FREE_NULL(vg_lv_cnts[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && vg_cnt > 0)
        return vg_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of LVM logical volumes (LV's) that are
 * present on the system and let them choose one. The logical volume (LV) path,
 * size, free space, PV count, and LV count are passed by reference and these
 * values are set after a selection is made. We return the number of volume
 * groups found on success, and -1 if an error occurred. This function will
 * handle displaying any errors that are encountered.
 */
int getLVChoice(CDKSCREEN *cdk_screen, char path[], char size[], char attr[]) {
    CDKSCROLL *lv_scroll = 0;
    char *lv_paths[MAX_LVM_LVS] = {NULL},
            *lv_sizes[MAX_LVM_LVS] = {NULL},
            *lv_attrs[MAX_LVM_LVS] = {NULL},
            *scroll_list[MAX_LVM_LVS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int lv_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --separator , "
                "--noheadings --options lv_path,lv_size,lv_attr 2>&1", LVS_BIN);
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
                if (lv_cnt < MAX_LVM_LVS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    SAFE_ASPRINTF(&lv_paths[lv_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&lv_sizes[lv_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&lv_attrs[lv_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&scroll_list[lv_cnt],
                            "<C>%-26.26s Size: %-10.10s Attributes: %-12.12s",
                            lv_paths[lv_cnt], lv_sizes[lv_cnt],
                            lv_attrs[lv_cnt]);
                    lv_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                            LVS_BIN, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (lv_cnt == 0) {
            errorDialog(cdk_screen,
                    "No LVM logical volumes were detected on this system.",
                    NULL);
            break;
        }

        /* Get logical drive choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a LVM Logical Volume\n",
                g_color_dialog_title[g_curr_theme]);
        lv_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                scroll_title, scroll_list, lv_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!lv_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(lv_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(lv_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(lv_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (lv_scroll->exitType == vNORMAL) {
            snprintf(path, MISC_STRING_LEN, "%s",
                    lv_paths[user_choice]);
            snprintf(size, MISC_STRING_LEN, "%s",
                    lv_sizes[user_choice]);
            snprintf(attr, MISC_STRING_LEN, "%s",
                    lv_attrs[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(lv_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < lv_cnt; i++) {
        FREE_NULL(lv_paths[i]);
        FREE_NULL(lv_sizes[i]);
        FREE_NULL(lv_attrs[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && lv_cnt > 0)
        return lv_cnt;
    else
        return -1;
}


/**
 * @brief Run the "LVM2 LV Information" dialog.
 */
void lvm2InfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *lvm2_info = 0;
    char *swindow_info[MAX_LVM2_INFO_LINES] = {NULL};
    char *error_msg = NULL, *lvdisplay_cmd = NULL, *swindow_title = NULL;
    int i = 0, line_pos = 0, status = 0, ret_val = 0;
    char line[LVM2_INFO_COLS] = {0};
    FILE *lvdisplay_proc = NULL;

    /* Run the lvdisplay command */
    SAFE_ASPRINTF(&lvdisplay_cmd, "%s --all 2>&1", LVDISPLAY_BIN);
    if ((lvdisplay_proc = popen(lvdisplay_cmd, "r")) == NULL) {
        SAFE_ASPRINTF(&error_msg, "Couldn't open process for the %s command!",
                LVDISPLAY_BIN);
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
    } else {
        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), lvdisplay_proc) != NULL) {
            if (line_pos < MAX_LVM2_INFO_LINES) {
                SAFE_ASPRINTF(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_LVM2_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_LVM2_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Close the process stream and check exit status */
        if ((status = pclose(lvdisplay_proc)) == -1) {
            ret_val = -1;
        } else {
            if (WIFEXITED(status))
                ret_val = WEXITSTATUS(status);
            else
                ret_val = -1;
        }
        if (ret_val == 0) {
            /* Setup scrolling window widget */
            SAFE_ASPRINTF(&swindow_title,
                    "<C></%d/B>LVM2 Logical Volume Information\n",
                    g_color_dialog_title[g_curr_theme]);
            lvm2_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                    (LVM2_INFO_ROWS + 2), (LVM2_INFO_COLS + 2),
                    swindow_title, MAX_LVM2_INFO_LINES, TRUE, FALSE);
            if (!lvm2_info) {
                errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(lvm2_info,
                    g_color_dialog_text[g_curr_theme]);
            setCDKSwindowBoxAttribute(lvm2_info,
                    g_color_dialog_box[g_curr_theme]);

            /* Set the scrolling window content */
            setCDKSwindowContents(lvm2_info, swindow_info, line_pos);

            /* The 'g' makes the swindow widget scroll to the
             * top, then activate */
            injectCDKSwindow(lvm2_info, 'g');
            activateCDKSwindow(lvm2_info, 0);

            /* We fell through -- the user exited the widget, but
             * we don't care how */
            destroyCDKSwindow(lvm2_info);
        } else {
            SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                    LVDISPLAY_BIN, ret_val);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    FREE_NULL(lvdisplay_cmd);
    FREE_NULL(swindow_title);
    for (i = 0; i < MAX_LVM2_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}


/**
 * @brief Run the "Add PV" dialog.
 */
void addPVDialog(CDKSCREEN *main_cdk_screen) {
    char *temp_pstr = NULL, *error_msg = NULL;
    char blk_dev_list[MAX_LVM_PVS][MISC_STRING_LEN] = {{0}, {0}},
            dev_info_line_buffer[MAX_DEV_INFO_LINE_BUFF] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int chosen_dev_cnt = 0, i = 0, dev_info_size = 0, dev_info_line_size = 0,
            ret_val = 0, exit_stat = 0;
    boolean finished = FALSE;

    /* The user first needs to select the block devices to use */
    if ((chosen_dev_cnt = getBlockDevSelection(main_cdk_screen,
            blk_dev_list)) == -1) {
        return;
    }

    /* Put selected block device information into a string */
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
        return;

    while (1) {
        /* Create the LVM physical volumes */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                PVCREATE_BIN, dev_info_line_buffer);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, PVCREATE_BIN, exit_stat);
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
 * @brief Run the "Remove PV" dialog.
 */
void remPVDialog(CDKSCREEN *main_cdk_screen) {
    char *temp_pstr = NULL, *error_msg = NULL, *confirm_msg = NULL;
    char lvm_pv_list[MAX_LVM_PVS][MISC_STRING_LEN] = {{0}, {0}},
            dev_info_line_buffer[MAX_DEV_INFO_LINE_BUFF] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int chosen_pv_cnt = 0, i = 0, dev_info_size = 0, dev_info_line_size = 0,
            ret_val = 0, exit_stat = 0;
    boolean finished = FALSE, confirm = FALSE;

    /* The user first needs to select the block devices to use */
    if ((chosen_pv_cnt = getPVSelection(main_cdk_screen,
            TRUE, lvm_pv_list)) == -1) {
        return;
    }

    /* Put selected block device information into a string */
    for (i = 0; i < chosen_pv_cnt; i++) {
        if (i == (chosen_pv_cnt - 1))
            SAFE_ASPRINTF(&temp_pstr, "%s", lvm_pv_list[i]);
        else
            SAFE_ASPRINTF(&temp_pstr, "%s ", lvm_pv_list[i]);
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
        return;

    while (1) {
        /* Get a final confirmation from user before removing the PV's */
        SAFE_ASPRINTF(&confirm_msg, "%.74s", dev_info_line_buffer);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to remove these LVM PV's?", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Remove the LVM physical volumes */
            snprintf(command_str, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                    PVREMOVE_BIN, dev_info_line_buffer);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                        PVREMOVE_BIN, exit_stat);
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
 * @brief Run the "Add VG" dialog.
 */
void addVGDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *vg_name_entry = 0;
    char *temp_pstr = NULL, *error_msg = NULL, *entry_title = NULL,
            *vg_name_entry_val = NULL;
    char lvm_pv_list[MAX_LVM_PVS][MISC_STRING_LEN] = {{0}, {0}},
            dev_info_line_buffer[MAX_DEV_INFO_LINE_BUFF] = {0},
            vg_name_str[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int chosen_pv_cnt = 0, i = 0, dev_info_size = 0, dev_info_line_size = 0,
            ret_val = 0, exit_stat = 0;
    boolean finished = FALSE;

    /* The user first needs to select the block devices to use */
    if ((chosen_pv_cnt = getPVSelection(main_cdk_screen,
            TRUE, lvm_pv_list)) == -1) {
        return;
    }

    /* Put selected block device information into a string */
    for (i = 0; i < chosen_pv_cnt; i++) {
        if (i == (chosen_pv_cnt - 1))
            SAFE_ASPRINTF(&temp_pstr, "%s", lvm_pv_list[i]);
        else
            SAFE_ASPRINTF(&temp_pstr, "%s ", lvm_pv_list[i]);
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
        return;

    while (1) {
        /* Get new volume group name (entry widget) */
        SAFE_ASPRINTF(&entry_title, "<C></%d/B>New LVM Volume Group Name\n",
                g_color_dialog_title[g_curr_theme]);
        vg_name_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
                entry_title, "</B>New VG Name (no spaces): ",
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                LVM_VOL_GRP_NAME_LEN, 0, LVM_VOL_GRP_NAME_LEN, TRUE, FALSE);
        if (!vg_name_entry) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(vg_name_entry,
                g_color_dialog_box[g_curr_theme]);
        setCDKEntryBackgroundAttrib(vg_name_entry,
                g_color_dialog_text[g_curr_theme]);

        /* Draw the entry widget */
        curs_set(1);
        vg_name_entry_val = activateCDKEntry(vg_name_entry, 0);
        curs_set(0);

        /* Check exit from widget */
        if (vg_name_entry->exitType == vNORMAL) {
            /* Check group name for bad characters */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS, vg_name_entry_val))
                finished = TRUE;
            /* We need this below */
            snprintf(vg_name_str, MISC_STRING_LEN, "%s", vg_name_entry_val);

        } else {
            finished = TRUE;
        }

        FREE_NULL(entry_title);
        if (vg_name_entry)
            destroyCDKEntry(vg_name_entry);
        refreshCDKScreen(main_cdk_screen);

        if (!finished) {
            /* Add the new LVM volume group */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s %s %s > /dev/null 2>&1", VGCREATE_BIN,
                    vg_name_str, dev_info_line_buffer);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                        VGCREATE_BIN, exit_stat);
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
 * @brief Run the "Remove VG" dialog.
 */
void remVGDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char vg_name[MISC_STRING_LEN] = {0},
            vg_size[MISC_STRING_LEN] = {0},
            vg_free_space[MISC_STRING_LEN] = {0},
            vg_pv_cnt[MISC_STRING_LEN] = {0},
            vg_lv_cnt[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int vg_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Let the user pick a LVM volume group */
    if ((vg_cnt = getVGChoice(main_cdk_screen, vg_name, vg_size,
            vg_free_space, vg_pv_cnt, vg_lv_cnt)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before we remove the VG */
        SAFE_ASPRINTF(&confirm_msg, "LVM volume group '%s'?", vg_name);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to remove", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Remove the existing LVM VG */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s %s > /dev/null 2>&1", VGREMOVE_BIN, vg_name);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                        VGREMOVE_BIN, exit_stat);
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
 * @brief Run the "Add LV" dialog.
 */
void addLVDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *new_lv_window = 0;
    CDKSCREEN *new_lv_screen = 0;
    CDKLABEL *new_lv_label = 0, *add_lv_msg = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *lv_name = 0, *lv_size = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char *error_msg = NULL;
    char vg_name[MISC_STRING_LEN] = {0},
            vg_size[MISC_STRING_LEN] = {0},
            vg_free_space[MISC_STRING_LEN] = {0},
            vg_pv_cnt[MISC_STRING_LEN] = {0},
            vg_lv_cnt[MISC_STRING_LEN] = {0},
            lv_name_str[MISC_STRING_LEN] = {0},
            lv_size_str[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    char *new_lv_msg[NEW_LV_INFO_LINES] = {NULL};
    int vg_cnt = 0, window_y = 0, window_x = 0, traverse_ret = 0, i = 0,
            exit_stat = 0, ret_val = 0, new_lv_window_lines = 0,
            new_lv_window_cols = 0;

    /* Let the user pick a LVM volume group */
    if ((vg_cnt = getVGChoice(main_cdk_screen, vg_name, vg_size,
            vg_free_space, vg_pv_cnt, vg_lv_cnt)) == -1) {
        return;
    }

    while (1) {
        /* Setup a new small CDK screen for virtual disk information */
        new_lv_window_lines = 12;
        new_lv_window_cols = 70;
        window_y = ((LINES / 2) - (new_lv_window_lines / 2));
        window_x = ((COLS / 2) - (new_lv_window_cols / 2));
        new_lv_window = newwin(new_lv_window_lines, new_lv_window_cols,
                window_y, window_x);
        if (new_lv_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        new_lv_screen = initCDKScreen(new_lv_window);
        if (new_lv_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(new_lv_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(new_lv_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(new_lv_window);

        /* Fill the information label */
        SAFE_ASPRINTF(&new_lv_msg[0],
                "</%d/B>Creating new LVM logical volume...",
                g_color_dialog_title[g_curr_theme]);
        SAFE_ASPRINTF(&new_lv_msg[1], " ");
        SAFE_ASPRINTF(&new_lv_msg[2],
                "</B>Volume Group:<!B>\t%-20.20s </B>PV Count:<!B>\t\t%s",
                vg_name, vg_pv_cnt);
        SAFE_ASPRINTF(&new_lv_msg[3],
                "</B>Size:<!B>\t\t%-20.20s </B>Available Space:<!B>\t%s",
                vg_size, vg_free_space);
        new_lv_label = newCDKLabel(new_lv_screen, (window_x + 1),
                (window_y + 1), new_lv_msg, NEW_LV_INFO_LINES, FALSE, FALSE);
        if (!new_lv_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(new_lv_label,
                g_color_dialog_text[g_curr_theme]);

        /* LV name */
        lv_name = newCDKEntry(new_lv_screen, (window_x + 1), (window_y + 6),
                "</B>Logical Volume Name", NULL,
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vLMIXED,
                20, 0, MAX_LV_NAME_LEN, FALSE, FALSE);
        if (!lv_name) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(lv_name, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(lv_name, g_color_dialog_text[g_curr_theme]);

        /* LV size */
        lv_size = newCDKEntry(new_lv_screen, (window_x + 30), (window_y + 6),
                "</B>Logical Volume Size (GiB)", NULL,
                g_color_dialog_select[g_curr_theme],
                '_' | g_color_dialog_input[g_curr_theme], vINT,
                12, 0, 12, FALSE, FALSE);
        if (!lv_size) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(lv_size, g_color_dialog_input[g_curr_theme]);
        setCDKEntryBackgroundAttrib(lv_size,
                    g_color_dialog_text[g_curr_theme]);

        /* Buttons */
        ok_button = newCDKButton(new_lv_screen, (window_x + 26),
                (window_y + 10), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(new_lv_screen, (window_x + 36),
                (window_y + 10), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(new_lv_screen);
        traverse_ret = traverseCDKScreen(new_lv_screen);
        break;
    }

    /* We need these below */
    snprintf(lv_name_str, MISC_STRING_LEN, "%s",
            getCDKEntryValue(lv_name));
    snprintf(lv_size_str, MISC_STRING_LEN, "%s",
            getCDKEntryValue(lv_size));

    /* Cleanup */
    for (i = 0; i < NEW_LV_INFO_LINES; i++)
        FREE_NULL(new_lv_msg[i]);
    if (new_lv_screen != NULL) {
        destroyCDKScreenObjects(new_lv_screen);
        destroyCDKScreen(new_lv_screen);
    }
    delwin(new_lv_window);
    refreshCDKScreen(main_cdk_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Make sure the entry field value is valid */
        if (!checkInputStr(main_cdk_screen, NAME_CHARS, lv_name_str))
            return;
        if (!checkInputStr(main_cdk_screen, ASCII_CHARS, lv_size_str))
            return;

        /* Display a label message while adding the logical volume */
        add_lv_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
                g_add_lv_label_msg, g_add_lv_label_msg_size(),
                TRUE, FALSE);
        if (!add_lv_msg) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            return;
        }
        setCDKLabelBackgroundAttrib(add_lv_msg,
                g_color_dialog_text[g_curr_theme]);
        setCDKLabelBoxAttribute(add_lv_msg,
                g_color_dialog_box[g_curr_theme]);
        refreshCDKScreen(main_cdk_screen);

        /* Add the new LVM logical volume */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --name %s --size %sG "
                "--type linear %s > /dev/null 2>&1", LVCREATE_BIN, lv_name_str,
                lv_size_str, vg_name);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, LVCREATE_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    destroyCDKLabel(add_lv_msg);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Run the "Remove LV" dialog.
 */
void remLVDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char lv_path[MISC_STRING_LEN] = {0},
            lv_size[MISC_STRING_LEN] = {0},
            lv_attr[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int lv_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Let the user pick a LVM logical volume */
    if ((lv_cnt = getLVChoice(main_cdk_screen, lv_path,
            lv_size, lv_attr)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before we remove the LV */
        SAFE_ASPRINTF(&confirm_msg, "LVM logical volume '%s'?", lv_path);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to remove", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Remove the existing LVM LV */
            snprintf(command_str, MAX_SHELL_CMD_LEN,
                    "%s --force %s > /dev/null 2>&1", LVREMOVE_BIN, lv_path);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR,
                        LVREMOVE_BIN, exit_stat);
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
