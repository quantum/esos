/**
 * @file menu_hwraid.c
 * @brief Contains the menu actions for the 'Hardware RAID' menu.
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
 * @brief Present the user with a list of hardware RAID controllers and let
 * them choose one. The controller type, ID number, model, and serial number
 * are passed by reference and these values are set after a selection is made.
 * We return the number of controllers found on success, and -1 if an error
 * occurred. This function will handle displaying any errors that are
 * encountered.
 */
int getCtrlrChoice(CDKSCREEN *cdk_screen, char type[], char id_num[],
        char model[], char serial[]) {
    CDKSCROLL *ctrlr_scroll = 0;
    char *ctrlr_types[MAX_HWRAID_CTRLRS] = {NULL},
            *ctrlr_ids[MAX_HWRAID_CTRLRS] = {NULL},
            *ctrlr_models[MAX_HWRAID_CTRLRS] = {NULL},
            *ctrlr_serials[MAX_HWRAID_CTRLRS] = {NULL},
            *scroll_list[MAX_HWRAID_CTRLRS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int ctrlr_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --list-controllers 2>&1",
                HWRAID_CLI_TOOL);
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
                if (ctrlr_cnt < MAX_HWRAID_CTRLRS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    SAFE_ASPRINTF(&ctrlr_types[ctrlr_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ctrlr_ids[ctrlr_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ctrlr_models[ctrlr_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ctrlr_serials[ctrlr_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&scroll_list[ctrlr_cnt],
                            "<C>%-10.10s ID # %-4.4s %-32.32s %-16.16s",
                            ctrlr_types[ctrlr_cnt], ctrlr_ids[ctrlr_cnt],
                            ctrlr_models[ctrlr_cnt], ctrlr_serials[ctrlr_cnt]);
                    ctrlr_cnt++;
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
                            HWRAID_CLI_TOOL, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (ctrlr_cnt == 0) {
            errorDialog(cdk_screen,
                    "No hardware RAID controllers were detected.",
                    "Are the correct HW RAID CLI tools installed?");
            break;
        }

        /* Get file system choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a RAID Controller\n",
                g_color_dialog_title[g_curr_theme]);
        ctrlr_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 12, 72,
                scroll_title, scroll_list, ctrlr_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!ctrlr_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(ctrlr_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(ctrlr_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(ctrlr_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (ctrlr_scroll->exitType == vNORMAL) {
            snprintf(type, MISC_STRING_LEN, "%s", ctrlr_types[user_choice]);
            snprintf(id_num, MISC_STRING_LEN, "%s", ctrlr_ids[user_choice]);
            snprintf(model, MISC_STRING_LEN, "%s", ctrlr_models[user_choice]);
            snprintf(serial, MISC_STRING_LEN, "%s", ctrlr_serials[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(ctrlr_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < ctrlr_cnt; i++) {
        FREE_NULL(ctrlr_types[i]);
        FREE_NULL(ctrlr_ids[i]);
        FREE_NULL(ctrlr_models[i]);
        FREE_NULL(ctrlr_serials[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && ctrlr_cnt > 0)
        return ctrlr_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of hardware RAID physical drives for the
 * given controller type/ID, and let them select any number of disks. The
 * physical drive enclosure ID and slot number are passed by reference as a
 * single list of strings and this value is set after a selection is made. We
 * return the number of physical drives selected on success, and -1 if an error
 * occurred. This function will handle displaying any errors that are
 * encountered.
 */
int getPDSelection(CDKSCREEN *cdk_screen, boolean avail_only, char type[],
        char id_num[], char encl_slot_list[MAX_HWRAID_PDRVS][MISC_STRING_LEN]) {
    CDKSELECTION *pd_select = 0;
    char *strtok_result = NULL, *error_msg = NULL, *pd_select_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    char *pd_encl_ids[MAX_HWRAID_PDRVS] = {NULL},
            *pd_slot_nums[MAX_HWRAID_PDRVS] = {NULL},
            *pd_states[MAX_HWRAID_PDRVS] = {NULL},
            *pd_sizes[MAX_HWRAID_PDRVS] = {NULL},
            *pd_models[MAX_HWRAID_PDRVS] = {NULL},
            *selection_list[MAX_HWRAID_PDRVS] = {NULL};
    int pd_cnt = 0, i = 0, status = 0, chosen_pd_cnt = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --list-physical-drives --type=%s --ctrlr-id=%s %s 2>&1",
                HWRAID_CLI_TOOL, type, id_num,
                (avail_only ? "--avail-only" : ""));
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
                if (pd_cnt < MAX_HWRAID_PDRVS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    strtok_result = strtok(NULL, ",");
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_encl_ids[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_slot_nums[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_states[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_sizes[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_models[pd_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&selection_list[pd_cnt],
                            "<C>Encl: %-5.5s Slot: %-4.4s "
                            "State: %-8.8s %-12.12s %-15.15s",
                            pd_encl_ids[pd_cnt], pd_slot_nums[pd_cnt],
                            pd_states[pd_cnt], pd_sizes[pd_cnt],
                            pd_models[pd_cnt]);
                    pd_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            HWRAID_CLI_TOOL, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (pd_cnt == 0) {
            errorDialog(cdk_screen,
                    "No physical drives were detected for",
                    "the selected hardware RAID controller.");
            break;
        }

        /* Selection widget for disks */
        SAFE_ASPRINTF(&pd_select_title,
                "<C></%d/B>Select Physical Drives (%s # %s)\n",
                g_color_dialog_title[g_curr_theme], type, id_num);
        pd_select = newCDKSelection(cdk_screen, CENTER, CENTER, NONE,
                18, 74, pd_select_title, selection_list, pd_cnt,
                g_choice_char, 2, g_color_dialog_select[g_curr_theme],
                TRUE, FALSE);
        if (!pd_select) {
            errorDialog(cdk_screen, SELECTION_ERR_MSG, NULL);
            break;
        }
        setCDKSelectionBoxAttribute(pd_select,
                g_color_dialog_box[g_curr_theme]);
        setCDKSelectionBackgroundAttrib(pd_select,
                g_color_dialog_text[g_curr_theme]);

        /* Activate the widget */
        activateCDKSelection(pd_select, 0);

        /* User hit escape, so we get out of this */
        if (pd_select->exitType == vESCAPE_HIT) {
            user_quit = TRUE;
            destroyCDKSelection(pd_select);
            refreshCDKScreen(cdk_screen);
            break;

        /* User hit return/tab so we can continue on and get what was selected */
        } else if (pd_select->exitType == vNORMAL) {
            chosen_pd_cnt = 0;
            for (i = 0; i < pd_cnt; i++) {
                if (pd_select->selections[i] == 1) {
                    snprintf(encl_slot_list[chosen_pd_cnt], MISC_STRING_LEN,
                            "%s:%s", pd_encl_ids[i], pd_slot_nums[i]);
                    chosen_pd_cnt++;
                }
            }
            destroyCDKSelection(pd_select);
            refreshCDKScreen(cdk_screen);
        }

        /* Check and make sure some drives were actually selected */
        if (chosen_pd_cnt == 0) {
            errorDialog(cdk_screen, "No physical drives selected!", NULL);
            break;
        }
        break;
    }

    /* Done */
    FREE_NULL(pd_select_title);
    for (i = 0; i < pd_cnt; i++) {
        FREE_NULL(pd_encl_ids[i]);
        FREE_NULL(pd_slot_nums[i]);
        FREE_NULL(pd_states[i]);
        FREE_NULL(pd_sizes[i]);
        FREE_NULL(pd_models[i]);
        FREE_NULL(selection_list[i]);
    }
    if (!user_quit && chosen_pd_cnt > 0)
        return chosen_pd_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of hardware RAID physical drives for the
 * given controller type/ID, and let them choose one. The physical drive
 * enclosure ID, slot number, state, size, and model are passed by reference
 * and these values are set after a selection is made. We return the number of
 * physical drives found on success, and -1 if an error occurred. This function
 * will handle displaying any errors that are encountered.
 */
int getPDChoice(CDKSCREEN *cdk_screen, boolean avail_only, char type[],
        char id_num[], char encl_id[], char slot_num[], char state[],
        char size[], char model[]) {
    CDKSCROLL *pd_scroll = 0;
    char *pd_encl_ids[MAX_HWRAID_PDRVS] = {NULL},
            *pd_slot_nums[MAX_HWRAID_PDRVS] = {NULL},
            *pd_states[MAX_HWRAID_PDRVS] = {NULL},
            *pd_sizes[MAX_HWRAID_PDRVS] = {NULL},
            *pd_models[MAX_HWRAID_PDRVS] = {NULL},
            *scroll_list[MAX_HWRAID_PDRVS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int pd_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --list-physical-drives --type=%s --ctrlr-id=%s %s 2>&1",
                HWRAID_CLI_TOOL, type, id_num,
                (avail_only ? "--avail-only" : ""));
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
                if (pd_cnt < MAX_HWRAID_PDRVS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    strtok_result = strtok(NULL, ",");
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_encl_ids[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_slot_nums[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_states[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_sizes[pd_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&pd_models[pd_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&scroll_list[pd_cnt],
                            "<C>%-4.4s %-4.4s %-12.12s %-15.15s %-15.15s",
                            pd_encl_ids[pd_cnt], pd_slot_nums[pd_cnt],
                            pd_states[pd_cnt], pd_sizes[pd_cnt],
                            pd_models[pd_cnt]);
                    pd_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            HWRAID_CLI_TOOL, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (pd_cnt == 0) {
            errorDialog(cdk_screen,
                    "No physical drives were detected for",
                    "the selected hardware RAID controller.");
            break;
        }

        /* Get file system choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a Physical Drive\n",
                g_color_dialog_title[g_curr_theme]);
        pd_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                scroll_title, scroll_list, pd_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!pd_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(pd_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(pd_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(pd_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (pd_scroll->exitType == vNORMAL) {
            snprintf(encl_id, MISC_STRING_LEN, "%s",
                    pd_encl_ids[user_choice]);
            snprintf(slot_num, MISC_STRING_LEN, "%s",
                    pd_slot_nums[user_choice]);
            snprintf(state, MISC_STRING_LEN, "%s",
                    pd_states[user_choice]);
            snprintf(size, MISC_STRING_LEN, "%s",
                    pd_sizes[user_choice]);
            snprintf(model, MISC_STRING_LEN, "%s",
                    pd_models[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(pd_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < pd_cnt; i++) {
        FREE_NULL(pd_encl_ids[i]);
        FREE_NULL(pd_slot_nums[i]);
        FREE_NULL(pd_states[i]);
        FREE_NULL(pd_sizes[i]);
        FREE_NULL(pd_models[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && pd_cnt > 0)
        return pd_cnt;
    else
        return -1;
}


/**
 * @brief Present the user with a list of hardware RAID logical drives for the
 * given controller type/ID, and let them choose one. The logical drive ID
 * number, RAID level, state, size, and name are passed by reference and these
 * values are set after a selection is made. We return the number of logical
 * drives found on success, and -1 if an error occurred. This function will
 * handle displaying any errors that are encountered.
 */
int getLDChoice(CDKSCREEN *cdk_screen, char type[], char id_num[],
        char ld_id[], char raid_lvl[], char state[], char size[],
        char name[]) {
    CDKSCROLL *ld_scroll = 0;
    char *ld_ids[MAX_HWRAID_LDRVS] = {NULL},
            *ld_raid_lvls[MAX_HWRAID_LDRVS] = {NULL},
            *ld_states[MAX_HWRAID_LDRVS] = {NULL},
            *ld_sizes[MAX_HWRAID_LDRVS] = {NULL},
            *ld_names[MAX_HWRAID_LDRVS] = {NULL},
            *scroll_list[MAX_HWRAID_LDRVS] = {NULL};
    char *strtok_result = NULL, *error_msg = NULL, *scroll_title = NULL;
    char command_str[MAX_SHELL_CMD_LEN] = {0},
            output_line[MAX_CMD_LINE_LEN] = {0};
    int ld_cnt = 0, i = 0, status = 0, user_choice = 0;
    FILE *shell_cmd = NULL;
    boolean user_quit = FALSE;

    while (1) {
        /* Put the command string together and execute it */
        snprintf(command_str, MAX_SHELL_CMD_LEN,
                "%s --list-logical-drives --type=%s --ctrlr-id=%s 2>&1",
                HWRAID_CLI_TOOL, type, id_num);
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
                if (ld_cnt < MAX_HWRAID_LDRVS) {
                    strtok_result = strtok(output_line, ",");
                    if (strtok_result == NULL)
                        continue;
                    strtok_result = strtok(NULL, ",");
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ld_ids[ld_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ld_raid_lvls[ld_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ld_states[ld_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ld_sizes[ld_cnt], "%s",
                            strStrip(strtok_result));
                    strtok_result = strtok(NULL, ",");
                    SAFE_ASPRINTF(&ld_names[ld_cnt], "%s",
                            strStrip(strtok_result));
                    SAFE_ASPRINTF(&scroll_list[ld_cnt],
                            "<C>%-4.4s %-10.10s %-12.12s %-15.15s %-12.12s",
                            ld_ids[ld_cnt], ld_raid_lvls[ld_cnt],
                            ld_states[ld_cnt], ld_sizes[ld_cnt],
                            ld_names[ld_cnt]);
                    ld_cnt++;
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
                    SAFE_ASPRINTF(&error_msg, "The %s command exited with %d.",
                            HWRAID_CLI_TOOL, WEXITSTATUS(status));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }

        /* Make sure we actually have something to present */
        if (ld_cnt == 0) {
            errorDialog(cdk_screen,
                    "No logical drives were detected for",
                    "the selected hardware RAID controller.");
            break;
        }

        /* Get logical drive choice */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a Logical Drive\n",
                g_color_dialog_title[g_curr_theme]);
        ld_scroll = newCDKScroll(cdk_screen, CENTER, CENTER, NONE, 15, 70,
                scroll_title, scroll_list, ld_cnt,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!ld_scroll) {
            errorDialog(cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(ld_scroll,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(ld_scroll,
                g_color_dialog_text[g_curr_theme]);
        user_choice = activateCDKScroll(ld_scroll, 0);

        /* Check exit from widget and write the data if normal */
        if (ld_scroll->exitType == vNORMAL) {
            snprintf(ld_id, MISC_STRING_LEN, "%s",
                    ld_ids[user_choice]);
            snprintf(raid_lvl, MISC_STRING_LEN, "%s",
                    ld_raid_lvls[user_choice]);
            snprintf(state, MISC_STRING_LEN, "%s",
                    ld_states[user_choice]);
            snprintf(size, MISC_STRING_LEN, "%s",
                    ld_sizes[user_choice]);
            snprintf(name, MISC_STRING_LEN, "%s",
                    ld_names[user_choice]);
        } else {
            user_quit = TRUE;
        }
        break;
    }

    /* Done */
    destroyCDKScroll(ld_scroll);
    refreshCDKScreen(cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < ld_cnt; i++) {
        FREE_NULL(ld_ids[i]);
        FREE_NULL(ld_raid_lvls[i]);
        FREE_NULL(ld_states[i]);
        FREE_NULL(ld_sizes[i]);
        FREE_NULL(ld_names[i]);
        FREE_NULL(scroll_list[i]);
    }
    if (!user_quit && ld_cnt > 0)
        return ld_cnt;
    else
        return -1;
}


/**
 * @brief Run the "Add Volume" dialog.
 */
void addVolDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *new_ld_window = 0;
    CDKSCREEN *new_ld_screen = 0;
    CDKLABEL *new_ld_label = 0, *add_ld_msg = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKRADIO *write_cache = 0, *read_cache = 0, *raid_lvl = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int ctrlr_cnt = 0, chosen_pd_cnt = 0, i = 0, ret_val = 0, exit_stat = 0,
            traverse_ret = 0, window_y = 0, window_x = 0,
            new_ld_window_lines = 0, new_ld_window_cols = 0, pd_info_size = 0,
            pd_info_line_size = 0, use_read_cache = 0, use_write_cache = 0;
    char *error_msg = NULL, *temp_pstr = NULL;
    char *new_ld_msg[NEW_LD_INFO_LINES] = {NULL};
    char ctrlr_type[MISC_STRING_LEN] = {0},
            ctrlr_id_num[MISC_STRING_LEN] = {0},
            ctrlr_model[MISC_STRING_LEN] = {0},
            ctrlr_serial[MISC_STRING_LEN] = {0},
            pd_encl_slot_list[MAX_HWRAID_PDRVS][MISC_STRING_LEN] = {{0}, {0}},
            pd_info_line_buffer[MAX_PD_INFO_LINE_BUFF] = {0},
            raid_lvl_str[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    boolean finished = FALSE;

    /* Have the user pick a RAID controller */
    if ((ctrlr_cnt = getCtrlrChoice(main_cdk_screen, ctrlr_type, ctrlr_id_num,
            ctrlr_model, ctrlr_serial)) == -1) {
        return;
    }

    /* Now they can select physical drives */
    if ((chosen_pd_cnt = getPDSelection(main_cdk_screen, TRUE, ctrlr_type,
            ctrlr_id_num, pd_encl_slot_list)) == -1) {
        return;
    }

    while (1) {
        /* Setup a new CDK screen for required input (to create new LD) */
        new_ld_window_lines = 14;
        new_ld_window_cols = 66;
        window_y = ((LINES / 2) - (new_ld_window_lines / 2));
        window_x = ((COLS / 2) - (new_ld_window_cols / 2));
        new_ld_window = newwin(new_ld_window_lines, new_ld_window_cols,
                window_y, window_x);
        if (new_ld_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        new_ld_screen = initCDKScreen(new_ld_window);
        if (new_ld_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(new_ld_window, g_color_dialog_box[g_curr_theme]);
        wbkgd(new_ld_window, g_color_dialog_text[g_curr_theme]);
        wrefresh(new_ld_window);

        /* Put selected physical drive information into a string */
        for (i = 0; i < chosen_pd_cnt; i++) {
            if (i == (chosen_pd_cnt - 1))
                SAFE_ASPRINTF(&temp_pstr, "%s", pd_encl_slot_list[i]);
            else
                SAFE_ASPRINTF(&temp_pstr, "%s,", pd_encl_slot_list[i]);
            /* We add one extra for the null byte */
            pd_info_size = strlen(temp_pstr) + 1;
            pd_info_line_size = pd_info_line_size + pd_info_size;
            if (pd_info_line_size >= MAX_PD_INFO_LINE_BUFF) {
                errorDialog(main_cdk_screen, "The maximum PD information "
                        "line buffer size has been reached!", NULL);
                FREE_NULL(temp_pstr);
                finished = TRUE;
                break;
            } else {
                strcat(pd_info_line_buffer, temp_pstr);
                FREE_NULL(temp_pstr);
            }
        }
        if (finished)
            break;

        /* Make a new label for the add-logical-drive screen */
        SAFE_ASPRINTF(&new_ld_msg[0],
                "</%d/B>Creating new %s LD (on controller # %s)...",
                g_color_dialog_title[g_curr_theme], ctrlr_type, ctrlr_id_num);
        SAFE_ASPRINTF(&new_ld_msg[1], " ");
        SAFE_ASPRINTF(&new_ld_msg[2], "Selected Drives (ENCL:SLOT) - %.35s",
                pd_info_line_buffer);
        new_ld_label = newCDKLabel(new_ld_screen, (window_x + 1),
                (window_y + 1), new_ld_msg, NEW_LD_INFO_LINES, FALSE, FALSE);
        if (!new_ld_label) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(new_ld_label,
                g_color_dialog_text[g_curr_theme]);

        /* RAID level radio list */
        raid_lvl = newCDKRadio(new_ld_screen, (window_x + 1), (window_y + 5),
                NONE, 5, 10, "</B>RAID Level", g_hw_raid_opts, 4,
                '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!raid_lvl) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(raid_lvl,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(raid_lvl, 0);

        /* Write cache radio list */
        write_cache = newCDKRadio(new_ld_screen, (window_x + 16),
                (window_y + 5), NONE, 3, 11, "</B>Write Cache", g_hw_write_opts,
                2, '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!write_cache) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(write_cache,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(write_cache, 1);

        /* Read cache radio list */
        read_cache = newCDKRadio(new_ld_screen, (window_x + 36),
                (window_y + 5), NONE, 3, 12, "</B>Read Cache", g_hw_read_opts,
                2, '#' | g_color_dialog_select[g_curr_theme], 1,
                g_color_dialog_select[g_curr_theme], FALSE, FALSE);
        if (!read_cache) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(read_cache,
                g_color_dialog_text[g_curr_theme]);
        setCDKRadioCurrentItem(read_cache, 1);

        /* Buttons */
        ok_button = newCDKButton(new_ld_screen, (window_x + 24),
                (window_y + 12), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button,
                g_color_dialog_input[g_curr_theme]);
        cancel_button = newCDKButton(new_ld_screen, (window_x + 34),
                (window_y + 12), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(cancel_button,
                g_color_dialog_input[g_curr_theme]);

        /* Allow user to traverse the screen */
        refreshCDKScreen(new_ld_screen);
        traverse_ret = traverseCDKScreen(new_ld_screen);
        break;
    }

    /* We need these below */
    snprintf(raid_lvl_str, MISC_STRING_LEN, "%s",
            g_hw_raid_opts[getCDKRadioSelectedItem(raid_lvl)]);
    use_read_cache = getCDKRadioSelectedItem(read_cache);
    use_write_cache = getCDKRadioSelectedItem(write_cache);

    /* Cleanup */
    for (i = 0; i < NEW_LD_INFO_LINES; i++)
        FREE_NULL(new_ld_msg[i]);
    if (new_ld_screen != NULL) {
        destroyCDKScreenObjects(new_ld_screen);
        destroyCDKScreen(new_ld_screen);
    }
    delwin(new_ld_window);
    refreshCDKScreen(main_cdk_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Display a label message while adding the logical drive */
        add_ld_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
                g_add_ld_label_msg, g_add_ld_label_msg_size(), TRUE, FALSE);
        if (!add_ld_msg) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            return;
        }
        setCDKLabelBackgroundAttrib(add_ld_msg,
                g_color_dialog_text[g_curr_theme]);
        setCDKLabelBoxAttribute(add_ld_msg,
                g_color_dialog_box[g_curr_theme]);
        refreshCDKScreen(main_cdk_screen);

        /* Add the new logical drive */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --add-logical-drive "
                "--type=%s --ctrlr-id=%s --raid-level=%s --phys-drives=%s "
                "%s %s > /dev/null 2>&1", HWRAID_CLI_TOOL, ctrlr_type,
                ctrlr_id_num, raid_lvl_str, pd_info_line_buffer,
                (use_read_cache ? "" : "--no-read-cache"),
                (use_write_cache ? "" : "--no-write-cache"));
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Error creating new logical drive; "
                    "hw_raid_cli.py exited with %d.", exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    destroyCDKLabel(add_ld_msg);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Run the "Remove Volume" dialog.
 */
void remVolDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL, *confirm_msg = NULL;
    char ctrlr_type[MISC_STRING_LEN] = {0},
            ctrlr_id_num[MISC_STRING_LEN] = {0},
            ctrlr_model[MISC_STRING_LEN] = {0},
            ctrlr_serial[MISC_STRING_LEN] = {0},
            ld_id[MISC_STRING_LEN] = {0},
            ld_raid_lvl[MISC_STRING_LEN] = {0},
            ld_state[MISC_STRING_LEN] = {0},
            ld_size[MISC_STRING_LEN] = {0},
            ld_name[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int ctrlr_cnt = 0, ld_cnt = 0, ret_val = 0, exit_stat = 0;
    boolean confirm = FALSE;

    /* Have the user pick a RAID controller */
    if ((ctrlr_cnt = getCtrlrChoice(main_cdk_screen, ctrlr_type, ctrlr_id_num,
            ctrlr_model, ctrlr_serial)) == -1) {
        return;
    }

    /* Now they can pick a logical drive */
    if ((ld_cnt = getLDChoice(main_cdk_screen, ctrlr_type, ctrlr_id_num,
            ld_id, ld_raid_lvl, ld_state, ld_size, ld_name)) == -1) {
        return;
    }

    while (1) {
        /* Get a final confirmation from user before we delete */
        SAFE_ASPRINTF(&confirm_msg, "drive # %s on %s controller %s?",
                ld_id, ctrlr_type, ctrlr_id_num);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to delete logical", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            /* Remove the existing logical drive */
            snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --rem-logical-drive "
                    "--type=%s --ctrlr-id=%s --rem-ld-id=%s > /dev/null 2>&1",
                    HWRAID_CLI_TOOL, ctrlr_type, ctrlr_id_num, ld_id);
            ret_val = system(command_str);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                SAFE_ASPRINTF(&error_msg, "Error removing the logical drive; "
                        "hw_raid_cli.py exited with %d.", exit_stat);
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
 * @brief Run the "Add Hot Spare" dialog.
 */
void addHSPDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL;
    char ctrlr_type[MISC_STRING_LEN] = {0},
            ctrlr_id_num[MISC_STRING_LEN] = {0},
            ctrlr_model[MISC_STRING_LEN] = {0},
            ctrlr_serial[MISC_STRING_LEN] = {0},
            pd_encl_id[MISC_STRING_LEN] = {0},
            pd_slot_num[MISC_STRING_LEN] = {0},
            pd_state[MISC_STRING_LEN] = {0},
            pd_size[MISC_STRING_LEN] = {0},
            pd_model[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int ctrlr_cnt = 0, pd_cnt = 0, ret_val = 0, exit_stat = 0;

    /* Have the user pick a RAID controller */
    if ((ctrlr_cnt = getCtrlrChoice(main_cdk_screen, ctrlr_type, ctrlr_id_num,
            ctrlr_model, ctrlr_serial)) == -1) {
        return;
    }

    /* Now they can pick a physical drive */
    if ((pd_cnt = getPDChoice(main_cdk_screen, TRUE, ctrlr_type, ctrlr_id_num,
            pd_encl_id, pd_slot_num, pd_state, pd_size, pd_model)) == -1) {
        return;
    }

    while (1) {
        /* Add the new hot spare */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --add-hot-spare "
                "--type=%s --ctrlr-id=%s --add-encl-id=%s --add-slot-num=%s "
                "> /dev/null 2>&1", HWRAID_CLI_TOOL, ctrlr_type, ctrlr_id_num,
                pd_encl_id, pd_slot_num);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Error adding the hot spare; "
                    "hw_raid_cli.py exited with %d.", exit_stat);
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
 * @brief Run the "Remove Hot Spare" dialog.
 */
void remHSPDialog(CDKSCREEN *main_cdk_screen) {
    char *error_msg = NULL;
    char ctrlr_type[MISC_STRING_LEN] = {0},
            ctrlr_id_num[MISC_STRING_LEN] = {0},
            ctrlr_model[MISC_STRING_LEN] = {0},
            ctrlr_serial[MISC_STRING_LEN] = {0},
            pd_encl_id[MISC_STRING_LEN] = {0},
            pd_slot_num[MISC_STRING_LEN] = {0},
            pd_state[MISC_STRING_LEN] = {0},
            pd_size[MISC_STRING_LEN] = {0},
            pd_model[MISC_STRING_LEN] = {0},
            command_str[MAX_SHELL_CMD_LEN] = {0};
    int ctrlr_cnt = 0, pd_cnt = 0, ret_val = 0, exit_stat = 0;

    /* Have the user pick a RAID controller */
    if ((ctrlr_cnt = getCtrlrChoice(main_cdk_screen, ctrlr_type, ctrlr_id_num,
            ctrlr_model, ctrlr_serial)) == -1) {
        return;
    }

    /* Now they can pick a physical drive */
    if ((pd_cnt = getPDChoice(main_cdk_screen, FALSE, ctrlr_type, ctrlr_id_num,
            pd_encl_id, pd_slot_num, pd_state, pd_size, pd_model)) == -1) {
        return;
    }

    while (1) {
        /* Remove the existing hot spare */
        snprintf(command_str, MAX_SHELL_CMD_LEN, "%s --rem-hot-spare "
                "--type=%s --ctrlr-id=%s --rem-encl-id=%s --rem-slot-num=%s "
                "> /dev/null 2>&1", HWRAID_CLI_TOOL, ctrlr_type, ctrlr_id_num,
                pd_encl_id, pd_slot_num);
        ret_val = system(command_str);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Error removing the hot spare; "
                    "hw_raid_cli.py exited with %d.", exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
        break;
    }

    /* Done */
    return;
}
