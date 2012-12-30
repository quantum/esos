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
 * Run the Target Information dialog
 */
void tgtInfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *tgt_info = 0;
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    char *swindow_info[MAX_TGT_INFO_LINES] = {NULL};
    int i = 0, line_pos = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    
    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;
    
    /* Setup scrolling window widget */
    tgt_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            TGT_INFO_ROWS+2, TGT_INFO_COLS+2,
            "<C></31/B>SCST Target Information:\n",
            MAX_TGT_INFO_LINES, TRUE, FALSE);
    if (!tgt_info) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(tgt_info, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(tgt_info, COLOR_DIALOG_BOX);

    /* Add target information */
    asprintf(&swindow_info[0], "</B>Target Name:<!B>\t%s", scst_tgt);
    asprintf(&swindow_info[1], "</B>Target Driver:<!B>\t%s", tgt_driver);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/enabled",
            SYSFS_SCST_TGT, tgt_driver, scst_tgt);
    readAttribute(dir_name, tmp_buff);
    asprintf(&swindow_info[2], "</B>Enabled:<!B>\t%s", tmp_buff);
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/cpu_mask",
            SYSFS_SCST_TGT, tgt_driver, scst_tgt);
    readAttribute(dir_name, tmp_buff);
    asprintf(&swindow_info[3], "</B>CPU Mask:<!B>\t%s", tmp_buff);
    asprintf(&swindow_info[4], " ");
    asprintf(&swindow_info[5], "</B>Groups:<!B>");

    /* Add target group information */
    line_pos = 6;
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/ini_groups",
            SYSFS_SCST_TGT, tgt_driver, scst_tgt);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        asprintf(&swindow_info[line_pos], "opendir(): %s", strerror(errno));
    } else {
        /* Loop over each entry in the directory */
        i = 0;
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The group names are directories; skip '.' and '..' */
            if (dir_entry->d_type == DT_DIR) {
                if (i > 1) {
                    if (line_pos < MAX_TGT_INFO_LINES) {
                        asprintf(&swindow_info[line_pos], "\t%s", dir_entry->d_name);
                        line_pos++;
                    }
                }
                i++;
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_TGT_INFO_LINES) {
        asprintf(&swindow_info[line_pos], " ");
        line_pos++;
    }
    if (line_pos < MAX_TGT_INFO_LINES) {
        asprintf(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;
    }
    
    /* Set the scrolling window content */
    setCDKSwindowContents(tgt_info, swindow_info, line_pos);
    
    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(tgt_info, 'g');
    activateCDKSwindow(tgt_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(tgt_info);

    /* Done */
    for (i = 0; i < MAX_TGT_INFO_LINES; i++ ) {
        freeChar(swindow_info[i]);
    }
    return;
}


/*
 * Run the Add iSCSI Target dialog
 */
void addiSCSITgtDialog(CDKSCREEN *main_cdk_screen) {
    CDKENTRY *tgt_name_entry = 0;
    char temp_str[SCST_ISCSI_TGT_LEN] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            nice_date[MISC_STRING_LEN] = {0}, hostname[MISC_STRING_LEN] = {0},
            rand_str[MISC_STRING_LEN] = {0}, def_iqn[SCST_ISCSI_TGT_LEN] = {0};
    static char hex_str[] = "0123456789abcdef";
    char *entry_title = NULL, *target_name = NULL, *error_msg = NULL, *tmp_pstr = NULL;
    int i = 0, temp_int = 0;
    time_t now = 0;
    struct tm *tm_now = NULL;

    /* Generate a default IQN for the new target */
    now = time(NULL);
    tm_now = localtime(&now);
    strftime(nice_date, sizeof(nice_date), "%Y-%m", tm_now);
    if (gethostname(hostname, sizeof(hostname)) == -1)
        snprintf(hostname, sizeof(hostname), "hostname");
    tmp_pstr = strchr(hostname, '.');
    if (tmp_pstr)
        *tmp_pstr = '\0';
    srand(time(NULL));
    snprintf(rand_str, MISC_STRING_LEN, "%c%c%c%c%c",
            hex_str[rand()%16], hex_str[rand()%16],hex_str[rand()%16],
            hex_str[rand()%16], hex_str[rand()%16]);
    snprintf(def_iqn, SCST_ISCSI_TGT_LEN, "iqn.%s.esos.%s:%s",
            nice_date, hostname, rand_str);

    /* Get new target name (entry widget) */
    asprintf(&entry_title, "<C></31/B>Adding a new iSCSI target...\n");
    tgt_name_entry = newCDKEntry(main_cdk_screen, CENTER, CENTER,
            entry_title, "</B>New target name (no spaces): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            SCST_ISCSI_TGT_LEN, 0, SCST_ISCSI_TGT_LEN, TRUE, FALSE);
    if (!tgt_name_entry) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(tgt_name_entry, COLOR_DIALOG_BOX);
    setCDKEntryBackgroundAttrib(tgt_name_entry, COLOR_DIALOG_TEXT);
    setCDKEntryValue(tgt_name_entry, def_iqn);

    /* Draw the entry widget */
    curs_set(1);
    target_name = activateCDKEntry(tgt_name_entry, 0);

    /* Check exit from widget */
    if (tgt_name_entry->exitType == vNORMAL) {
        /* Check target name for bad characters */
        strncpy(temp_str, target_name, SCST_ISCSI_TGT_LEN);
        i = 0;
        while (temp_str[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_INIT_CHAR(temp_str[i])) {
                errorDialog(main_cdk_screen,
                        "The target name entry field contains invalid characters!",
                        VALID_INIT_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }
        /* User didn't provide a target name */
        if (i == 0) {
            errorDialog(main_cdk_screen, "The target name field cannot be empty!", NULL);
            goto cleanup;
        }

        /* Add the new iSCSI target */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/iscsi/mgmt",
                SYSFS_SCST_TGT);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "add_target %s", target_name);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't add SCST iSCSI target: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    cleanup:
    freeChar(entry_title);
    if (tgt_name_entry)
        destroyCDKEntry(tgt_name_entry);
    return;
}


/*
 * Run the Remove iSCSI Target dialog
 */
void remiSCSITgtDialog(CDKSCREEN *main_cdk_screen) {
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    boolean confirm = FALSE;
    int temp_int = 0;

    /* Have the user choose an SCST iSCSI target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, "iscsi");
    if (scst_tgt[0] == '\0')
        return;

    /* Get a final confirmation from user before we delete */
    asprintf(&confirm_msg, "target %s?", scst_tgt);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you want to delete SCST iSCSI", confirm_msg);
    freeChar(confirm_msg);
    if (confirm) {
        /* Delete the specified iSCSI target */
        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/iscsi/mgmt", SYSFS_SCST_TGT);
        snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "del_target %s", scst_tgt);
        if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
            asprintf(&error_msg, "Couldn't delete SCST iSCSI target: %s", strerror(temp_int));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    return;
}


/*
 * Run the Issue LIP dialog
 */
void issueLIPDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *lip_info = 0;
    char *error_msg = NULL;
    char *swindow_msg[MAX_LIP_INFO_LINES] = {NULL};
    char attr_path[MAX_SYSFS_PATH_SIZE] = {0};
    int i = 0, temp_int = 0;
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;

    /* Setup scrolling window widget */
    lip_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            LIP_INFO_ROWS+2, LIP_INFO_COLS+2,
            "<C></31/B>Issuing LIP on all Fibre Channel targets:\n",
            MAX_LIP_INFO_LINES, TRUE, FALSE);
    if (!lip_info) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(lip_info, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(lip_info, COLOR_DIALOG_BOX);

    /* Draw the widget */
    drawCDKSwindow(lip_info, TRUE);
    refreshCDKScreen(main_cdk_screen);

    /* Open the Fibre Channel sysfs directory */
    if ((dir_stream = opendir(SYSFS_FC_HOST)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    } else {
        /* Loop over each entry in the directory */
        i = 0;
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The FC host names are links */
            if (dir_entry->d_type == DT_LNK) {
                if (i < MAX_LIP_INFO_LINES) {
                    asprintf(&swindow_msg[i], "<C>LIP on FC host %s...", dir_entry->d_name);
                    addCDKSwindow(lip_info, swindow_msg[i], BOTTOM);
                    i++;
                }

                /* Write the sysfs attribute (issue_lip) */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/issue_lip",
                        SYSFS_FC_HOST, dir_entry->d_name);
                if ((temp_int = writeAttribute(attr_path, "1")) != 0) {
                    asprintf(&error_msg, "Couldn't issue LIP on FC host %s: %s",
                            dir_entry->d_name, strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);
    }
        
    /* Done */
    destroyCDKSwindow(lip_info);
    for (i = 0; i < MAX_LIP_INFO_LINES; i++) {
        freeChar(swindow_msg[i]);
    }
    return;
}


/*
 * Run the Enable/Disable Target dialog
 */
void enblDsblTgtDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCREEN *tgt_screen = 0;
    CDKRADIO *enbl_dsbl_radio = 0;
    CDKLABEL *tgt_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    WINDOW *tgt_window = 0;
    char scst_tgt[MAX_SYSFS_ATTR_SIZE] = {0},
            tgt_driver[MAX_SYSFS_ATTR_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_value[MAX_SYSFS_ATTR_SIZE] = {0},
            dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    static char *dsbl_enbl[] = {"Disabled (0)", "Enabled (1)"};
    char *error_msg = NULL, *confirm_msg = NULL;
    char *tgt_info_msg[TGT_ON_OFF_INFO_LINES] = {NULL};
    boolean confirm = FALSE;
    int tgt_window_lines = 0, tgt_window_cols = 0, window_y = 0, window_x = 0,
            temp_int = 0, i = 0, traverse_ret = 0, curr_state = 0, new_state = 0;
    
    /* Have the user choose a SCST target */
    getSCSTTgtChoice(main_cdk_screen, scst_tgt, tgt_driver);
    if (scst_tgt[0] == '\0' || tgt_driver[0] == '\0')
        return;

    /* Check the target's current status -- enabled or disabled */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/enabled",
            SYSFS_SCST_TGT, tgt_driver, scst_tgt);
    readAttribute(dir_name, attr_value);
    curr_state = atoi(attr_value);

    /* New CDK screen */
    tgt_window_lines = 14;
    tgt_window_cols = 50;
    window_y = ((LINES / 2) - (tgt_window_lines / 2));
    window_x = ((COLS / 2) - (tgt_window_cols / 2));
    tgt_window = newwin(tgt_window_lines, tgt_window_cols, window_y, window_x);
    if (tgt_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        return;
    }
    tgt_screen = initCDKScreen(tgt_window);
    if (tgt_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        return;
    }
    boxWindow(tgt_window, COLOR_DIALOG_BOX);
    wbkgd(tgt_window, COLOR_DIALOG_TEXT);
    wrefresh(tgt_window);

    /* Information label */
    asprintf(&tgt_info_msg[0], "</31/B>Enable/Disable Target...");
    asprintf(&tgt_info_msg[1], " ");
    asprintf(&tgt_info_msg[2], "Target: %s", scst_tgt);
    asprintf(&tgt_info_msg[3], "Driver: %s", tgt_driver);
    if (curr_state == 0)
        asprintf(&tgt_info_msg[4], "Current state: Disabled");
    else if (curr_state == 1)
        asprintf(&tgt_info_msg[4], "Current state: Enabled");
    tgt_info = newCDKLabel(tgt_screen, (window_x + 1), (window_y + 1),
            tgt_info_msg, TGT_ON_OFF_INFO_LINES, FALSE, FALSE);
    if (!tgt_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(tgt_info, COLOR_DIALOG_TEXT);

    /* Enable/disable widget (radio) */
    enbl_dsbl_radio = newCDKRadio(tgt_screen, (window_x + 1), (window_y + 7),
            NONE, 3, 10, "</B>Target", dsbl_enbl, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!enbl_dsbl_radio) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(enbl_dsbl_radio, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(enbl_dsbl_radio, curr_state);

    /* Buttons */
    ok_button = newCDKButton(tgt_screen, (window_x + 16), (window_y + 12),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(tgt_screen, (window_x + 26), (window_y + 12),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(tgt_screen);
    traverse_ret = traverseCDKScreen(tgt_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check if we are actually changing anything */
        new_state = getCDKRadioSelectedItem(enbl_dsbl_radio);
        if (new_state != curr_state) {
            if (new_state == 0) {
                /* Enabled -> Disabled (we need a warning) */
                asprintf(&confirm_msg, "%s (%s)?", scst_tgt, tgt_driver);
                confirm = confirmDialog(main_cdk_screen,
                        "Are you sure you want to disable SCST target",
                        confirm_msg);
                freeChar(confirm_msg);
                if (!confirm)
                    goto cleanup;
            }

            /* Make sure iSCSI targets are a possibility (driver level?) */
            if (((strcmp(tgt_driver, "iscsi")) == 0) && (new_state == 1)) {
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/iscsi/enabled",
                        SYSFS_SCST_TGT);
                snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "%d", new_state);
                if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                    asprintf(&error_msg, "Couldn't enable iSCSI target support: %s",
                            strerror(temp_int));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
            }

            /* Set the new state for the target */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/%s/%s/enabled",
                    SYSFS_SCST_TGT, tgt_driver, scst_tgt);
            snprintf(attr_value, MAX_SYSFS_ATTR_SIZE, "%d", new_state);
            if ((temp_int = writeAttribute(attr_path, attr_value)) != 0) {
                asprintf(&error_msg, "Couldn't set SCST target state: %s", strerror(temp_int));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
            }
        }
    }

    /* All done */
    cleanup:
    for (i = 0; i < TGT_ON_OFF_INFO_LINES; i++)
        freeChar(tgt_info_msg[i]);
    if (tgt_screen != NULL) {
        destroyCDKScreenObjects(tgt_screen);
        destroyCDKScreen(tgt_screen);
        delwin(tgt_window);
    }
    return;
}