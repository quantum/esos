/**
 * @file menu_interface.c
 * @brief Contains the menu actions for the 'Interface' menu.
 * @author Copyright (c) 2012-2018 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <assert.h>
#include <iniparser.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief Run the "Color Theme" dialog.
 */
void themeDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCROLL *iface_theme_list = 0;
    int theme_choice = 0, i = 0;
    char *scroll_theme_list[MAX_TUI_THEMES] = {NULL};
    char *error_msg = NULL, *scroll_title = NULL;
    char theme_str[MISC_STRING_LEN] = {0};
    dictionary *ini_dict = NULL;
    FILE *ini_file = NULL;

    while (1) {
        /* Set the theme choices */
        SAFE_ASPRINTF(&scroll_theme_list[BLUE_TUI],
                "<C>Blue / Cyan / White Text");
        SAFE_ASPRINTF(&scroll_theme_list[BLACK_TUI],
                "<C>Black / Green / White Text");

        /* Get the interface theme choice from user */
        SAFE_ASPRINTF(&scroll_title, "<C></%d/B>Choose a Color Theme\n",
                g_color_dialog_title[g_curr_theme]);
        iface_theme_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE,
                15, 55, scroll_title, scroll_theme_list, 2, FALSE,
                g_color_dialog_select[g_curr_theme], TRUE, FALSE);
        if (!iface_theme_list) {
            errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(iface_theme_list,
                g_color_dialog_box[g_curr_theme]);
        setCDKScrollBackgroundAttrib(iface_theme_list,
                g_color_dialog_text[g_curr_theme]);
        setCDKScrollCurrentItem(iface_theme_list, g_curr_theme);
        theme_choice = activateCDKScroll(iface_theme_list, 0);

        /* Check exit from widget and write the value if normal */
        if ((iface_theme_list->exitType == vNORMAL) &&
                ((int) g_curr_theme != theme_choice)) {
            /* Load the ESOS configuration file (INI file) */
            ini_dict = iniparser_load(ESOS_CONF);
            if (ini_dict == NULL) {
                errorDialog(main_cdk_screen, ESOS_CONF_READ_ERR_1,
                        ESOS_CONF_READ_ERR_2);
                break;
            }
            /* Figure out which theme string to set */
            if (theme_choice == BLUE_TUI)
                snprintf(theme_str, MISC_STRING_LEN, "blue");
            else if (theme_choice == BLACK_TUI)
                snprintf(theme_str, MISC_STRING_LEN, "black");
            else
                snprintf(theme_str, MISC_STRING_LEN, "blue");
            /* Set the selected theme choice */
            if (iniparser_set(ini_dict, "tui:theme", theme_str) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            /* Save the file */
            if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                SAFE_ASPRINTF(&error_msg, ESOS_CONF_WRITE_ERR, strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
            iniparser_dump_ini(ini_dict, ini_file);
            fclose(ini_file);
            /* Changes won't take effect until next login */
            informDialog(main_cdk_screen, "The new theme selection will",
                    "take effect on the next login.");
        }
        break;
    }

    /* Done */
    destroyCDKScroll(iface_theme_list);
    refreshCDKScreen(main_cdk_screen);
    FREE_NULL(scroll_title);
    for (i = 0; i < MAX_TUI_THEMES; i++)
        FREE_NULL(scroll_theme_list[i]);
    if (ini_dict != NULL)
        iniparser_freedict(ini_dict);
    return;
}


/**
 * @brief Run the "Help" dialog.
 */
void helpDialog(CDKSCREEN *main_cdk_screen) {
    CDKDIALOG *help_dialog = 0;
    char *message[HELP_MSG_SIZE] = {NULL};
    int i = 0;

    /* Set the message */
    SAFE_ASPRINTF(&message[0], "<C></%d/B>ESOS TUI Help",
            g_color_dialog_title[g_curr_theme]);
    SAFE_ASPRINTF(&message[1], " ");
    SAFE_ASPRINTF(&message[2], "</B>Main Menu");
    SAFE_ASPRINTF(&message[3], "To activate the main menu, hit any of the "
            "menu hot keys. You can");
    SAFE_ASPRINTF(&message[4], "then use the arrow keys to navigate the menu, "
            "and use ENTER to make");
    SAFE_ASPRINTF(&message[5], "a selection. Use ESCAPE to exit the menu "
            "without making a selection.");
    SAFE_ASPRINTF(&message[6], " ");
    SAFE_ASPRINTF(&message[7], "</B>Navigating Dialogs");
    SAFE_ASPRINTF(&message[8], "On dialogs (screens) that contain more than "
            "one widget, you can use");
    SAFE_ASPRINTF(&message[9], "TAB and SHIFT+TAB to traverse through the "
            "widgets (field entry, radio");
    SAFE_ASPRINTF(&message[10], "lists, buttons, etc.) and use ENTER to "
            "execute button functions.");
    SAFE_ASPRINTF(&message[11], "On selection dialogs, single field entry "
            "widgets, etc. you can use");
    SAFE_ASPRINTF(&message[12], "ENTER or TAB to make a choice, or use ESCAPE "
            "to cancel.");
    SAFE_ASPRINTF(&message[13], " ");
    SAFE_ASPRINTF(&message[14], "</B>Scrolling Windows");
    SAFE_ASPRINTF(&message[15], "When a scrolling window widget is active, you "
            "can use the arrow keys");
    SAFE_ASPRINTF(&message[16], "to scroll through the text and use ENTER or "
            "ESCAPE to exit.");

    while (1) {
        /* Display the TUI help message dialog box */
        help_dialog = newCDKDialog(main_cdk_screen, CENTER, CENTER, message,
                HELP_MSG_SIZE, g_ok_msg, 1, g_color_dialog_select[g_curr_theme],
                TRUE, TRUE, FALSE);
        if (!help_dialog) {
            errorDialog(main_cdk_screen, DIALOG_ERR_MSG, NULL);
            break;
        }
        setCDKDialogBackgroundAttrib(help_dialog,
                g_color_dialog_text[g_curr_theme]);
        setCDKDialogBoxAttribute(help_dialog, g_color_dialog_box[g_curr_theme]);

        /* We don't care how the user exits the widget */
        activateCDKDialog(help_dialog, 0);
        destroyCDKDialog(help_dialog);
        break;
    }

    /* All done */
    for (i = 0; i < HELP_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Run the "Support Bundle" dialog.
 */
void supportArchDialog(CDKSCREEN *main_cdk_screen) {
    CDKDIALOG *bundle_dialog = 0;
    char pkg_file_path[MAX_SYSFS_ATTR_SIZE] = {0};
    char *cmd_str = NULL, *error_msg = NULL;
    char *message[SUPPORT_PKG_MSG_SIZE] = {NULL};
    int ret_val = 0, exit_stat = 0, i = 0;
    FILE *support_cmd = NULL;

    /* Archive the configuration files and logs */
    SAFE_ASPRINTF(&cmd_str, "%s 2>&1", SUPPORT_TOOL);
    support_cmd = popen(cmd_str, "r");
    fgets(pkg_file_path, sizeof (pkg_file_path), support_cmd);
    if ((exit_stat = pclose(support_cmd)) == -1) {
        ret_val = -1;
    } else {
        if (WIFEXITED(exit_stat))
            ret_val = WEXITSTATUS(exit_stat);
        else
            ret_val = -1;
    }
    FREE_NULL(cmd_str);
    if (ret_val != 0) {
        SAFE_ASPRINTF(&error_msg, CMD_FAILED_ERR, SUPPORT_TOOL, ret_val);
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
        return;
    }

    /* Set a message for the dialog */
    SAFE_ASPRINTF(&message[0], "<C></%d/B>ESOS Support Bundle",
            g_color_dialog_title[g_curr_theme]);
    SAFE_ASPRINTF(&message[1], " ");
    SAFE_ASPRINTF(&message[2], "<C>An archive containing configuration files "
            "and logs has been");
    SAFE_ASPRINTF(&message[3], "<C>created; its located here: </B>%s<!B>",
            pkg_file_path);
    SAFE_ASPRINTF(&message[4], "<C>You may now retrieve the file from this "
            "host via SFTP/SCP.");
    SAFE_ASPRINTF(&message[5], " ");
    SAFE_ASPRINTF(&message[6], " ");

    while (1) {
        /* Display the dialog box */
        bundle_dialog = newCDKDialog(main_cdk_screen, CENTER, CENTER, message,
                SUPPORT_PKG_MSG_SIZE, g_ok_msg, 1,
                g_color_dialog_select[g_curr_theme],
                TRUE, TRUE, FALSE);
        if (!bundle_dialog) {
            errorDialog(main_cdk_screen, DIALOG_ERR_MSG, NULL);
            break;
        }
        setCDKDialogBackgroundAttrib(bundle_dialog,
                g_color_dialog_text[g_curr_theme]);
        setCDKDialogBoxAttribute(bundle_dialog,
                g_color_dialog_box[g_curr_theme]);

        /* We don't care how the user exits the widget */
        activateCDKDialog(bundle_dialog, 0);
        destroyCDKDialog(bundle_dialog);
        break;
    }

    /* All done */
    for (i = 0; i < SUPPORT_PKG_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Run the "About" dialog.
 */
void aboutDialog(CDKSCREEN *main_cdk_screen) {
    CDKDIALOG *about_dialog = 0;
    char *message[ABOUT_MSG_SIZE] = {NULL};
    char esos_ver[MAX_SYSFS_ATTR_SIZE] = {0}, hostname[MISC_STRING_LEN] = {0};
    int i = 0;

    /* Set the message */
    snprintf(esos_ver, MAX_SYSFS_ATTR_SIZE,
            "ESOS - Enterprise Storage OS %s", ESOS_VERSION);
    if (gethostname(hostname, ((sizeof hostname) - 1)) == -1)
        snprintf(hostname, sizeof (hostname), "hostname");
    SAFE_ASPRINTF(&message[0], "<C></%d/B>About ESOS",
            g_color_dialog_title[g_curr_theme]);
    SAFE_ASPRINTF(&message[1], " ");
    SAFE_ASPRINTF(&message[2], "</B>This Host:<!B>\t%-.40s", hostname);
    SAFE_ASPRINTF(&message[3], "</B>Running:<!B>\t%-.40s", esos_ver);
    SAFE_ASPRINTF(&message[4], " ");
    SAFE_ASPRINTF(&message[5], "</B>Build Options:<!B>\t%-.40s", BUILD_OPTS);
    SAFE_ASPRINTF(&message[6], " ");
    SAFE_ASPRINTF(&message[7], "</B>ESOS Copyright (C) 2017 Marc A. Smith");
    SAFE_ASPRINTF(&message[8], "This program comes with ABSOLUTELY NO "
            "WARRANTY; for details view");
    SAFE_ASPRINTF(&message[9], "the ESOS license file. This is free software, "
            "and you are welcome to");
    SAFE_ASPRINTF(&message[10], "redistribute it under certain conditions; "
            "view the ESOS license file");
    SAFE_ASPRINTF(&message[11], "for details. ESOS license file: %s",
            ESOS_LICENSE);
    SAFE_ASPRINTF(&message[12], " ");
    SAFE_ASPRINTF(&message[13], " ");

    while (1) {
        /* Display the dialog box */
        about_dialog = newCDKDialog(main_cdk_screen, CENTER, CENTER, message,
                ABOUT_MSG_SIZE, g_ok_msg, 1,
                g_color_dialog_select[g_curr_theme], TRUE, TRUE, FALSE);
        if (!about_dialog) {
            errorDialog(main_cdk_screen, DIALOG_ERR_MSG, NULL);
            break;
        }
        setCDKDialogBackgroundAttrib(about_dialog,
                g_color_dialog_text[g_curr_theme]);
        setCDKDialogBoxAttribute(about_dialog,
                g_color_dialog_box[g_curr_theme]);

        /* We don't care how the user exits the widget */
        activateCDKDialog(about_dialog, 0);
        destroyCDKDialog(about_dialog);
        break;
    }

    /* All done */
    for (i = 0; i < ABOUT_MSG_SIZE; i++)
        FREE_NULL(message[i]);
    refreshCDKScreen(main_cdk_screen);
    return;
}
