/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <curses.h>

#include "menu_actions-hosts.h"
#include "menu_actions.h"
#include "main.h"

/*
 * Run the Add Group dialog
 */
void addGroupDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *grp_name_entry = 0;
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
        tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
        temp_str[SCST_GRP_NAME_LEN] = {0},
        attr_path[MAX_SYSFS_PATH_SIZE] = {0},
        attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *entry_title = NULL, *group_name = NULL, *error_msg = NULL;
    int i = 0, temp_int = 0;
    
    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get new group name (entry widget) */
    asprintf(&entry_title, "<C></31/B>Adding new group to target %s (%s)...\n",
            scst_tgt, tgt_driver);
    grp_name_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
            entry_title, "</B>New group name (no spaces): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            SCST_GRP_NAME_LEN, 0, SCST_GRP_NAME_LEN, TRUE, FALSE);
    if (!grp_name_entry) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(grp_name_entry, COLOR_DIALOG_BOX);
    setCDKEntryBackgroundAttrib(grp_name_entry, COLOR_DIALOG_TEXT);

    /* Draw the entry widget */
    curs_set(1);
    group_name = activateCDKEntry(grp_name_entry, 0);

    /* Check exit from widget */
    if (grp_name_entry->exitType == vNORMAL) {
        /* Check group name for bad characters */
        strncpy(temp_str, group_name, SCST_GRP_NAME_LEN);
        i = 0;
        while (temp_str[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (isspace(temp_str[i]) || !isalnum(temp_str[i])) {
                errorDialog(main_cdk_screen,
                        "Group name field must only contain alphanumeric characters!", NULL);
                goto cleanup;
            }
            i++;
        }
        /* User didn't provide a group name */
        if (i == 0) {
            errorDialog(main_cdk_screen, "The group name field cannot be empty!", NULL);
            goto cleanup;
        }

        /* Add the new group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "create %s", group_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't add SCST group: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    cleanup:
    freeChar(entry_title);
    if (grp_name_entry)
        destroyCDKEntry(grp_name_entry);
    return;
}


/*
 * Run the Remove Group dialog
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
    asprintf(&confirm_msg, "Are you sure you want to delete SCST target group %s?",
            group_name);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    freeChar(confirm_msg);
    if (confirm) {
        /* Delete the specified SCST group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/mgmt", SYSFS_SCST_TGT, tgt_driver, scst_tgt);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", group_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't delete SCST group: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    return;
}


/*
 * Run the Add Initiator dialog
 */
void addInitDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *init_entry = 0;
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
        tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
        temp_str[SCST_INITIATOR_LEN] = {0},
        attr_path[MAX_SYSFS_PATH_SIZE] = {0},
        attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
        group_name[MAX_SYSFS_ATTR_SIZE] = {0};
    char *entry_title = NULL, *initiator = NULL, *error_msg = NULL;
    int i = 0, temp_int = 0;

    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Get group choice from user (based on previously selected target) */
    getSCSTGroupChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name);
    if (group_name[0] == '\0')
        return;

    /* Get initiator (entry widget) */
    asprintf(&entry_title, "<C></31/B>Adding initiator to group %s...\n",
            group_name);
    init_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
            entry_title, "</B>Initiator (no spaces): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            30, 0, SCST_INITIATOR_LEN, TRUE, FALSE);
    if (!init_entry) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(init_entry, COLOR_DIALOG_BOX);
    setCDKEntryBackgroundAttrib(init_entry, COLOR_DIALOG_TEXT);

    /* Draw the entry widget */
    curs_set(1);
    initiator = activateCDKEntry(init_entry, 0);

    /* Check exit from widget */
    if (init_entry->exitType == vNORMAL) {
        /* Check group name for bad characters */
        strncpy(temp_str, initiator, SCST_INITIATOR_LEN);
        i = 0;
        while (temp_str[i] != '\0') {
            /* If the user didn't input an acceptable string, then cancel out */
            if (isspace(temp_str[i])) {
                errorDialog(main_cdk_screen,
                        "The initiator field cannot contain any spaces!", NULL);
                goto cleanup;
            }
            i++;
        }
        /* User didn't provide a initiator */
        if (i == 0) {
            errorDialog(main_cdk_screen, "The initiator field cannot be empty!", NULL);
            goto cleanup;
        }

        /* Add the initiator to the target group */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/initiators/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add %s", initiator);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't add initiator to SCST group: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    cleanup:
    freeChar(entry_title);
    if (init_entry)
        destroyCDKEntry(init_entry);
    return;
}


/*
 * Run the Remove Initiator dialog
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
    getSCSTInitChoice(main_cdk_screen, scst_tgt, tgt_driver, group_name, init_name);
    if (init_name[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    asprintf(&confirm_msg, "%s from group %s?", init_name, group_name);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you want to remove initiator", confirm_msg);
    freeChar(confirm_msg);
    if (confirm) {
        /* Remove the initiator */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups/%s/initiators/mgmt",
                SYSFS_SCST_TGT, tgt_driver, scst_tgt, group_name);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del %s", init_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't remove initiator: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    return;
}