/**
 * @file menu_hosts.c
 * @brief Contains the menu actions for the 'Hosts' menu.
 * @author Copyright (c) 2012-2016 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <curses.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Add Group" dialog.
 */
void addGroupDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *grp_name_entry = 0;
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
        tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
        attr_path[MAX_SYSFS_PATH_SIZE] = {0},
        attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *entry_title = NULL, *group_name = NULL, *error_msg = NULL;
    int temp_int = 0;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    while (1) {
        /* Get new group name (entry widget) */
        SAFE_ASPRINTF(&entry_title,
                "<C></%d/B>Add New Group to Target %s (%s)\n",
                g_color_dialog_title[g_curr_theme], scst_tgt, tgt_driver);
        grp_name_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
                entry_title, "</B>New Group Name (no spaces): ",
                g_color_dialog_select[g_curr_theme], '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                SCST_SEC_GRP_NAME_LEN, 0, SCST_SEC_GRP_NAME_LEN, TRUE, FALSE);
        if (!grp_name_entry) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(grp_name_entry, g_color_dialog_box[g_curr_theme]);
        setCDKEntryBackgroundAttrib(grp_name_entry, g_color_dialog_text[g_curr_theme]);

        /* Draw the entry widget */
        curs_set(1);
        group_name = activateCDKEntry(grp_name_entry, 0);

        /* Check exit from widget */
        if (grp_name_entry->exitType == vNORMAL) {
            /* Check group name for bad characters */
            if (!checkInputStr(main_cdk_screen, NAME_CHARS, group_name))
                break;

            /* Add the new group */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                    "%s/targets/%s/%s/ini_groups/mgmt",
                    SYSFS_SCST_TGT, tgt_driver, scst_tgt);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "create %s", group_name);
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                SAFE_ASPRINTF(&error_msg, "Couldn't add SCST security group: %s",
                        strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* Done */
    FREE_NULL(entry_title);
    if (grp_name_entry)
        destroyCDKEntry(grp_name_entry);
    return;
}


/**
 * @brief Run the "Remove Group" dialog.
 */
void remGroupDialog(CDKSCREEN *main_cdk_screen) {
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            group_name[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg,
            "Are you sure you want to delete SCST security group '%s'?",
            group_name);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", group_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Couldn't delete SCST security group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}


/**
 * @brief Run the "Add Initiator" dialog.
 */
void addInitDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *init_entry = 0;
    CDKSCROLL *scst_init_list = 0;
    int init_choice = 0, i = 0, temp_int = 0, init_use_cnt = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char *scst_sess_inits[MAX_SCST_SESS_INITS] = {NULL},
            *scroll_init_list[MAX_SCST_SESS_INITS] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL, *entry_title = NULL,
            *entry_init_name = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            mgmt_attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            mgmt_attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            group_name[MAX_SYSFS_ATTR_SIZE] = {0},
            init_attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            init_attr_val[MAX_SYSFS_ATTR_SIZE] = {0};

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    while (1) {
        /* Open the directory for sessions */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/sessions",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Give the user an option to enter their own initiator name */
        SAFE_ASPRINTF(&scroll_init_list[i], "<C>Enter Initiator Name");
        i++;

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The session names are directories; skip '.' and '..' */
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                /* Read the initiator name attribute */
                snprintf(init_attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/sessions/%s/initiator_name",
                        SYSFS_SCST_TGT, tgt_driver, scst_tgt,
                        dir_entry->d_name);
                readAttribute(init_attr_path, init_attr_val);
                /* If the initiator is already in this group, don't show it */
                if (!isSCSTInitInGroup(scst_tgt, tgt_driver, group_name,
                        dir_entry->d_name)) {
                    if (i < MAX_SCST_SESS_INITS) {
                        SAFE_ASPRINTF(&scst_sess_inits[i], "%s", init_attr_val);
                        init_use_cnt = countSCSTInitUses(scst_tgt, tgt_driver,
                                init_attr_val);
                        SAFE_ASPRINTF(&scroll_init_list[i],
                                "<C>%.30s - Used By %d Groups",
                                prettyShrinkStr(30, scst_sess_inits[i]),
                                init_use_cnt);
                        i++;
                    }
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);

        /* Get SCST initiator choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Select an Initiator (%s)\n",
                g_color_dialog_title[g_curr_theme], scst_tgt);
        scst_init_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE,
                15, 55, scroll_title, scroll_init_list, i,
                FALSE, g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!scst_init_list) {
            errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(scst_init_list, g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(scst_init_list, g_color_dialog_text[g_curr_theme]);
        init_choice = activateCDKScroll(scst_init_list, 0);

        /* Check exit from widget */
        if (scst_init_list->exitType == vESCAPE_HIT) {
            destroyCDKScroll(scst_init_list);
            refreshCDKScreen(main_cdk_screen);
            break;

        } else if (scst_init_list->exitType == vNORMAL) {
            destroyCDKScroll(scst_init_list);
            refreshCDKScreen(main_cdk_screen);

            if (init_choice == 0) {
                /* Let the user enter their own initiator name (entry widget) */
                SAFE_ASPRINTF(&entry_title,
                        "<C></%d/B>Add Initiator to Group %s (%s)\n",
                        g_color_dialog_title[g_curr_theme], group_name,
                        scst_tgt);
                init_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
                        entry_title, "</B>Initiator Name (no spaces): ",
                        g_color_dialog_select[g_curr_theme], '_' | g_color_dialog_input[g_curr_theme], vMIXED,
                        30, 0, SCST_INITIATOR_LEN, TRUE, FALSE);
                if (!init_entry) {
                    errorDialog(main_cdk_screen, ENTRY_ERR_MSG,
                            NULL);
                    break;
                }
                setCDKEntryBoxAttribute(init_entry, g_color_dialog_box[g_curr_theme]);
                setCDKEntryBackgroundAttrib(init_entry, g_color_dialog_text[g_curr_theme]);

                /* Draw the entry widget */
                curs_set(1);
                entry_init_name = activateCDKEntry(init_entry, 0);

                /* Check exit from widget */
                if (init_entry->exitType == vNORMAL) {
                    /* Check initiator name for bad characters */
                    if (!checkInputStr(main_cdk_screen, INIT_CHARS,
                            entry_init_name))
                        break;
                }
            }
        }

        /* Add the initiator to the target group */
        snprintf(mgmt_attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/initiators/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(mgmt_attr_value, MAX_SYSFS_ATTR_SIZE, "add %s",
                ((entry_init_name == NULL) ?
                scst_sess_inits[init_choice] : entry_init_name));
        if ((temp_int = writeAttribute(mgmt_attr_path, mgmt_attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg,
                    "Couldn't add initiator to SCST group: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
        break;
    }

    /* Done */
    FREE_NULL(entry_title);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_SCST_SESS_INITS; i++) {
        FREE_NULL(scst_sess_inits[i]);
        FREE_NULL(scroll_init_list[i]);
    }
    if (init_entry)
        destroyCDKEntry(init_entry);
    return;
}


/**
 * @brief Run the "Remove Initiator" dialog.
 */
void remInitDialog(CDKSCREEN *main_cdk_screen) {
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
        tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
        init_name[MAX_SYSFS_ATTR_SIZE] = {0},
        attr_path[MAX_SYSFS_PATH_SIZE] = {0},
        attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
        group_name[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    /* Now the user selects an initiator */
    getSCSTInitChoice(main_cdk_screen, scst_tgt, tgt_driver,
            group_name, init_name);
    if (init_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    SAFE_ASPRINTF(&confirm_msg, "'%s' from group '%s'?", init_name, group_name);
    confirm = confirmDialog(main_cdk_screen,
            "Are you sure you want to remove initiator", confirm_msg);
    FREE_NULL(confirm_msg);
    if (confirm) {
        /* Remove the initiator */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s/%s/ini_groups/%s/initiators/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", init_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            SAFE_ASPRINTF(&error_msg, "Couldn't remove initiator: %s",
                    strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
        }
    }

    /* Done */
    return;
}
