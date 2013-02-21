/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <sys/wait.h>
#include <syslog.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"

int main(int argc, char** argv) {
    CDKSCREEN *cdk_screen = 0;
    WINDOW *main_window = 0, *sub_window = 0;
    CDKMENU *menu = 0;
    CDKLABEL *adapters_label = 0, *targets_label = 0, *devices_label = 0;
    const char *menu_list[MAX_MENU_ITEMS][MAX_SUB_ITEMS] = {{NULL}, {NULL}};
    int submenu_size[CDK_MENU_MAX_SIZE] = {0}, menu_loc[CDK_MENU_MAX_SIZE] = {0};
    char *adapters_label_msg[ADAPTERS_LABEL_ROWS] = {NULL};
    char *targets_label_msg[TARGETS_LABEL_ROWS] = {NULL};
    char *devices_label_msg[DEVICES_LABEL_ROWS] = {NULL};
    char *error_msg = NULL;
    int selection = 0, key_pressed = 0, menu_choice = 0, submenu_choice = 0,
            screen_x = 0, screen_y = 0, orig_scr_x = 0, orig_scr_y = 0,
            child_status = 0, proc_status = 0;
    static char *adapters_label_title = "HBAs / HCAs",
            *devices_label_title = "SCST Devices",
            *targets_label_title = "Dynamic Targets";
    pid_t child_pid = 0;
    uid_t saved_uid = 0;

    /* Make sure the umask is something sane (per the man page, this call always succeeds) */
    umask(0022);

    /* Initialize screen and check size */
    start:
    main_window = initscr();
    curs_set(0);
    noecho();
    getmaxyx(main_window, screen_y, screen_x);
    if (screen_y < MIN_SCR_Y || screen_x < MIN_SCR_X) termSize(main_window);

    /* Setup CDK */
    sub_window = newwin(LINES-2, COLS-2, 1, 1);
    orig_scr_y = LINES;
    orig_scr_x = COLS;
    wbkgd(main_window, COLOR_MAIN_TEXT);
    wbkgd(sub_window, COLOR_MAIN_TEXT);
    cdk_screen = initCDKScreen(sub_window);
    initCDKColor();

    /* Create the menu lists */
    menu_list[SYSTEM_MENU][0]                           = "</29/B/U>S<!29><!U>ystem  <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_SYNC_CONF]            = "</B>Sync. Configuration <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_NETWORK]              = "</B>Network Settings    <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_RESTART_NET]          = "</B>Restart Networking  <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_MAIL]                 = "</B>Mail Setup          <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_TEST_EMAIL]           = "</B>Send Test Email     <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_ADD_USER]             = "</B>Add User            <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_DEL_USER]             = "</B>Delete User         <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_CHG_PASSWD]           = "</B>Change Password     <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_SCST_INFO]            = "</B>SCST Information    <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_CRM_STATUS]           = "</B>CRM Status          <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_DATE_TIME]            = "</B>Date & Time Settings<!B>";

    menu_list[BACK_STORAGE_MENU][0]                             = "</29/B/U>B<!29><!U>ack-End Storage  <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADP_PROP]         = "</B>Adapter Properties      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADP_INFO]         = "</B>Adapter Information     <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADD_VOL]          = "</B>Add Volume              <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DEL_VOL]          = "</B>Delete Volume           <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_VOL_PROP]         = "</B>Volume Properties       <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DRBD_STAT]        = "</B>DRBD Status             <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_SOFT_RAID_STAT]   = "</B>Software RAID Status    <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_LVM2_INFO]        = "</B>LVM2 LV Information     <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_CREATE_FS]        = "</B>Create File System      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_REMOVE_FS]        = "</B>Remove File System      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADD_VDISK_FILE]   = "</B>Add Virtual Disk File   <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DEL_VDISK_FILE]   = "</B>Delete Virtual Disk File<!B>";

    menu_list[HOSTS_MENU][0]                            = "</29/B/U>H<!29><!U>osts  <!B>";
    menu_list[HOSTS_MENU][HOSTS_ADD_GROUP]              = "</B>Add Group       <!B>";
    menu_list[HOSTS_MENU][HOSTS_REM_GROUP]              = "</B>Remove Group    <!B>";
    menu_list[HOSTS_MENU][HOSTS_ADD_INIT]               = "</B>Add Initiator   <!B>";
    menu_list[HOSTS_MENU][HOSTS_REM_INIT]               = "</B>Remove Initiator<!B>";

    menu_list[DEVICES_MENU][0]                          = "</29/B/U>D<!29><!U>evices  <!B>";
    menu_list[DEVICES_MENU][DEVICES_LUN_LAYOUT]         = "</B>LUN Layout        <!B>";
    menu_list[DEVICES_MENU][DEVICES_DEV_INFO]           = "</B>Device Information<!B>";
    menu_list[DEVICES_MENU][DEVICES_ADD_DEV]            = "</B>Add Device        <!B>";
    menu_list[DEVICES_MENU][DEVICES_DEL_DEV]            = "</B>Delete Device     <!B>";
    menu_list[DEVICES_MENU][DEVICES_MAP_TO]             = "</B>Map to Group      <!B>";
    menu_list[DEVICES_MENU][DEVICES_UNMAP_FROM]         = "</B>Unmap from Group  <!B>";

    menu_list[TARGETS_MENU][0]                          = "</29/B/U>T<!29><!U>argets  <!B>";
    menu_list[TARGETS_MENU][TARGETS_TGT_INFO]           = "</B>Target Information   <!B>";
    menu_list[TARGETS_MENU][TARGETS_ADD_ISCSI]          = "</B>Add iSCSI Target     <!B>";
    menu_list[TARGETS_MENU][TARGETS_REM_ISCSI]          = "</B>Remove iSCSI Target  <!B>";
    menu_list[TARGETS_MENU][TARGETS_LIP]                = "</B>Issue LIP            <!B>";
    menu_list[TARGETS_MENU][TARGETS_TOGGLE]             = "</B>Enable/Disable Target<!B>";

    menu_list[INTERFACE_MENU][0]                        = "</29/B/U>I<!29><!U>nterface<!B>";
    menu_list[INTERFACE_MENU][INTERFACE_QUIT]           = "</B>Quit          <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_SHELL]          = "</B>Exit to Shell <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_HELP]           = "</B>Help          <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_SUPPORT_PKG]    = "</B>Support Bundle<!B>";
    menu_list[INTERFACE_MENU][INTERFACE_ABOUT]          = "</B>About         <!B>";

    /* Set menu sizes and locations */
    submenu_size[SYSTEM_MENU]       = 12;
    menu_loc[SYSTEM_MENU]           = LEFT;
    submenu_size[BACK_STORAGE_MENU]  = 13;
    menu_loc[BACK_STORAGE_MENU]      = LEFT;
    submenu_size[HOSTS_MENU]        = 5;
    menu_loc[HOSTS_MENU]            = LEFT;
    submenu_size[DEVICES_MENU]      = 7;
    menu_loc[DEVICES_MENU]          = LEFT;
    submenu_size[TARGETS_MENU]      = 6;
    menu_loc[TARGETS_MENU]          = LEFT;
    submenu_size[INTERFACE_MENU]    = 6;
    menu_loc[INTERFACE_MENU]        = RIGHT;

    /* Create the menu */
    menu = newCDKMenu(cdk_screen, menu_list, 6, submenu_size, menu_loc,
            TOP, A_NORMAL, COLOR_MENU_TEXT);
    if (menu != NULL) {
        setCDKMenuBackgroundColor(menu, "</5>");
    }

    /* Set the initial label messages; the number of characters
     * controls the label width (using white space as padding for width) */
    asprintf(&adapters_label_msg[0], "</21/B/U>%s<!21><!B><!U>%*s",
            adapters_label_title,
            (int) (ADAPTERS_LABEL_COLS-strlen(adapters_label_title)), "");
    asprintf(&devices_label_msg[0], "</21/B/U>%s<!21><!B><!U>%*s",
            devices_label_title,
            (int) (DEVICES_LABEL_COLS-strlen(devices_label_title)), "");
    asprintf(&targets_label_msg[0], "</21/B/U>%s<!21><!B><!U>%*s",
            targets_label_title,
            (int) (TARGETS_LABEL_COLS-strlen(targets_label_title)), "");
    
    /* Create the information/status labels */
    adapters_label = newCDKLabel(cdk_screen,
            1, 2,
            adapters_label_msg, ADAPTERS_LABEL_ROWS, TRUE, FALSE);
    if (adapters_label != NULL) {
        setCDKLabelBoxAttribute(adapters_label, COLOR_MAIN_BOX);
        setCDKLabelBackgroundAttrib(adapters_label, COLOR_MAIN_TEXT);
    }
    devices_label = newCDKLabel(cdk_screen,
            1, 4+ADAPTERS_LABEL_ROWS,
            devices_label_msg, DEVICES_LABEL_ROWS, TRUE, FALSE);
    if (devices_label != NULL) {
        setCDKLabelBoxAttribute(devices_label, COLOR_MAIN_BOX);
        setCDKLabelBackgroundAttrib(devices_label, COLOR_MAIN_TEXT);
    }
    targets_label = newCDKLabel(cdk_screen,
            3+DEVICES_LABEL_COLS, 4+ADAPTERS_LABEL_ROWS,
            targets_label_msg, TARGETS_LABEL_ROWS, TRUE, FALSE);
    if (targets_label != NULL) {
        setCDKLabelBoxAttribute(targets_label, COLOR_MAIN_BOX);
        setCDKLabelBackgroundAttrib(targets_label, COLOR_MAIN_TEXT);
    }

    /* We need root privileges; for the short term I don't see any other way
     around this; long term we can hopefully do something else */
    saved_uid = getuid();
    if (setresuid(0, -1, saved_uid) == -1) {
        asprintf(&error_msg, "setresuid(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg, "Your capabilities may be reduced...");
        freeChar(error_msg);
    }

    /* Draw the CDK screen */
    statusBar(main_window);
    refreshCDKScreen(cdk_screen);

    /* Loop refreshing the labels and waiting for input */
    halfdelay(REFRESH_DELAY);
    for (;;) {
        /* Update the adapters label message */
        readAdapterData(adapters_label_msg);
        setCDKLabelMessage(adapters_label, adapters_label_msg, ADAPTERS_LABEL_ROWS);

        /* Update the devices label message */
        readDeviceData(devices_label_msg);
        setCDKLabelMessage(devices_label, devices_label_msg, DEVICES_LABEL_ROWS);

        /* Update the disks label message */
        readTargetData(targets_label_msg);
        setCDKLabelMessage(targets_label, targets_label_msg, TARGETS_LABEL_ROWS);

        /* Get user input */
        wrefresh(sub_window);
        keypad(sub_window, TRUE);
        key_pressed = wgetch(sub_window);

        /* Check and see what we got */
        if (key_pressed == 115 || key_pressed == 83) {
            /* Start with the System menu */
            cbreak();
            setCDKMenu(menu, SYSTEM_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == 98 || key_pressed == 66) {
            /* Start with the Back-End Storage menu */
            cbreak();
            setCDKMenu(menu, BACK_STORAGE_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == 104 || key_pressed == 72) {
            /* Start with the Hosts menu */
            cbreak();
            setCDKMenu(menu, HOSTS_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == 100 || key_pressed == 68) {
            /* Start with the Devices menu */
            cbreak();
            setCDKMenu(menu, DEVICES_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == 116 || key_pressed == 84) {
            /* Start with the Targets menu */
            cbreak();
            setCDKMenu(menu, TARGETS_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == 105 || key_pressed == 73) {
            /* Start with the Interface menu */
            cbreak();
            setCDKMenu(menu, INTERFACE_MENU, 0, A_NORMAL, COLOR_MENU_TEXT);
            selection = activateCDKMenu(menu, 0);

        } else if (key_pressed == KEY_RESIZE) {
            /* Screen re-size */
            screenResize(cdk_screen, main_window, orig_scr_x, orig_scr_y);

        } else if (key_pressed == ERR) {
            /* Looks like the user didn't press anything (half-delay mode) */

        } else {
            beep();
        }

        if (menu->exitType == vNORMAL) {
            /* Get the selected menu choice */
            menu_choice = selection / 100;
            submenu_choice = selection % 100;

            if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_SHELL - 1) {
                /* Set the UID to what was saved at the start */
                if (setresuid(saved_uid, 0, -1) == -1) {
                    asprintf(&error_msg, "setresuid(): %s", strerror(errno));
                    errorDialog(cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                }
                /* Fork and execute a shell */
                if ((child_pid = fork()) < 0) {
                    /* Could not fork */
                    asprintf(&error_msg, "fork(): %s", strerror(errno));
                    errorDialog(cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                } else if (child_pid == 0) {
                    /* Child; fix up the terminal and execute the shell */
                    endwin();
                    curs_set(1);
                    echo();
                    system(CLEAR_BIN);
                    /* Execute the shell; if we fail, print something useful to syslog */
                    if ((execl(SHELL_BIN, SHELL_BIN, "--rcfile", GLOBAL_BASHRC, "-i", (char *) NULL)) == -1) {
                        openlog(LOG_PREFIX, LOG_OPTIONS, LOG_FACILITY);
                        syslog(LOG_ERR, "Calling execl() failed: %s", strerror(errno));
                        closelog();
                    }
                    exit(2);
                } else {
                    /* Parent; wait for the child to finish */
                    while ((proc_status = wait(&child_status)) != child_pid) {
                        if (proc_status < 0 && errno == ECHILD)
                            break;
                        errno = 0;
                    }
                    /* Yes, using 'goto' again... ending everything and
                     * starting fresh seems to work best when switching
                     * between the shell and UI */
                    destroyCDKScreenObjects(cdk_screen);
                    destroyCDKScreen(cdk_screen);
                    endCDK();
                    freeChar(adapters_label_msg[0]);
                    freeChar(devices_label_msg[0]);
                    freeChar(targets_label_msg[0]);
                    goto start;
                }

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_QUIT - 1) {
                /* Synchronize the configuration */
                syncConfig(cdk_screen);
                /* All done -- clean up */
                destroyCDKScreenObjects(cdk_screen);
                destroyCDKScreen(cdk_screen);
                endCDK();
                delwin(sub_window);
                delwin(main_window);
                system(CLEAR_BIN);
                exit(EXIT_SUCCESS);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_HELP - 1) {
                /* Help dialog */
                helpDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_SUPPORT_PKG - 1) {
                /* Support Bundle dialog */
                supportArchDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_ABOUT - 1) {
                /* About dialog */
                aboutDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_SYNC_CONF - 1) {
                /* Sync. Configuration dialog */
                syncConfig(cdk_screen);
                
            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_NETWORK - 1) {
                /* Networking dialog */
                networkDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_RESTART_NET - 1) {
                /* Restart Networking dialog */
                restartNetDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_MAIL - 1) {
                /* Mail Setup dialog */
                mailDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_TEST_EMAIL - 1) {
                /* Test Email dialog */
                testEmailDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_ADD_USER - 1) {
                /* Add User dialog */
                addUserDialog(cdk_screen);
                
            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_DEL_USER - 1) {
                /* Delete User dialog */
                delUserDialog(cdk_screen);
                
            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_CHG_PASSWD - 1) {
                /* Change Password dialog */
                chgPasswdDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_SCST_INFO - 1) {
                /* SCST Information dialog */
                scstInfoDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_CRM_STATUS - 1) {
                /* CRM Status dialog */
                crmStatusDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_DATE_TIME - 1) {
                /* Date & Time Settings dialog */
                dateTimeDialog(cdk_screen);

            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_ADP_PROP - 1) {
                /* Adapter Properties dialog */
                adpPropsDialog(cdk_screen);

            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_ADP_INFO - 1) {
                /* Adapter Information dialog */
                adpInfoDialog(cdk_screen);

            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_ADD_VOL - 1) {
                /* Add Volume dialog */
                addVolumeDialog(cdk_screen);

            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_DEL_VOL - 1) {
                /* Delete Volume dialog */
                delVolumeDialog(cdk_screen);

            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_VOL_PROP - 1) {
                /* Volume Properties dialog */
                volPropsDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_DRBD_STAT - 1) {
                /* DRBD Status dialog */
                drbdStatDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_SOFT_RAID_STAT - 1) {
                /* Software RAID Status dialog */
                softRAIDStatDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_LVM2_INFO - 1) {
                /* LVM2 LV Information dialog */
                lvm2InfoDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_CREATE_FS - 1) {
                /* Add Create File System dialog */
                createFSDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_REMOVE_FS - 1) {
                /* Remove File System dialog */
                removeFSDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_ADD_VDISK_FILE - 1) {
                /* Add Virtual Disk File dialog */
                addVDiskFileDialog(cdk_screen);
                
            } else if (menu_choice == BACK_STORAGE_MENU &&
                    submenu_choice == BACK_STORAGE_DEL_VDISK_FILE - 1) {
                /* Delete Virtual Disk File dialog */
                delVDiskFileDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_ADD_GROUP - 1) {
                /* Add Group dialog */
                addGroupDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_REM_GROUP - 1) {
                /* Remove Group dialog */
                remGroupDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_ADD_INIT - 1) {
                /* Add Initiator dialog */
                addInitDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_REM_INIT - 1) {
                /* Remove Initiator dialog */
                remInitDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_LUN_LAYOUT - 1) {
                /* LUN Layout dialog */
                lunLayoutDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_DEV_INFO - 1) {
                /* Device Information dialog */
                devInfoDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_ADD_DEV - 1) {
                /* Add Device dialog */
                addDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_DEL_DEV - 1) {
                /* Delete Device dialog */
                delDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_MAP_TO - 1) {
                /* Map to Group dialog */
                mapDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_UNMAP_FROM - 1) {
                /* Unmap from Group dialog */
                unmapDeviceDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_TGT_INFO - 1) {
                /* Target Information dialog */
                tgtInfoDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_ADD_ISCSI - 1) {
                /* Add iSCSI Target dialog */
                addiSCSITgtDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_REM_ISCSI - 1) {
                /* Remove iSCSI Target dialog */
                remiSCSITgtDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_LIP - 1) {
                /* Issue LIP dialog */
                issueLIPDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_TOGGLE - 1) {
                /* Enable/Disable Target dialog */
                enblDsblTgtDialog(cdk_screen);
            }

            /* At this point we've finished the dialog, so we make
             * the screen look nice again and reset the menu exit status */
            menu->exitType = vNEVER_ACTIVATED;
            wbkgd(cdk_screen->window, COLOR_MAIN_TEXT);
            wrefresh(cdk_screen->window);
            refreshCDKScreen(cdk_screen);
            curs_set(0);
        }

        /* Done with the menu, go back to halfdelay mode */
        halfdelay(REFRESH_DELAY);
    }
}


/*
 * Helper to fix initial terminal/screen size (before CDK initialization)
 */
void termSize(WINDOW *screen) {
    int input_char = 0, screen_x = 0, screen_y = 0;
    char *size_msgs[3] = {NULL};
    int size_msg_lens[3] = {0};

    /* This seems bad using goto, but I can't think of a better
     * solution at the moment */
    goto resize_start;
    halfdelay(REFRESH_DELAY);

    /* Wait for a resize event or escape */
    for (;;) {
        wrefresh(screen);
        input_char = wgetch(screen);

        if (input_char == KEY_RESIZE) {
            resize_start:
            clear();
            refresh();
            getmaxyx(screen, screen_y, screen_x);

            /* Check new screen size and return if we meet the minimums */
            if (screen_y >= MIN_SCR_Y && screen_x >= MIN_SCR_X) {
                return;
            } else {
                size_msgs[0] = "Your terminal is not the correct size!";
                size_msg_lens[0] = strlen(size_msgs[0]);
                asprintf(&size_msgs[1],
                        "Current screen size: %d rows X %d columns",
                        screen_y, screen_x);
                size_msg_lens[1] = strlen(size_msgs[1]);
                asprintf(&size_msgs[2],
                        "(Screen must be at least %d rows and %d columns.)",
                        MIN_SCR_Y, MIN_SCR_X);
                size_msg_lens[2] = strlen(size_msgs[2]);
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - size_msg_lens[0]) / 2), size_msgs[0]);
                mvaddstr((screen_y / 2), ((screen_x - size_msg_lens[1]) / 2),
                        size_msgs[1]);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - size_msg_lens[2]) / 2), size_msgs[2]);

                //freeChar(size_msgs[0]);
                //freeChar(size_msgs[1]);
                //freeChar(size_msgs[2]);
            }

        } else if (input_char == KEY_ESC) {
            printf("\n");
            endwin();
            exit(EXIT_SUCCESS);
        }
    }
    return;
}


/*
 * Handle terminal resize (SIGWINCH / KEY_RESIZE)
 */
void screenResize(CDKSCREEN *cdk, WINDOW *main_screen,
        int orig_term_x, int orig_term_y) {
    int input_char = 0, screen_y = 0, screen_x = 0;
    char *size_msgs[3] = {NULL};
    int size_msg_lens[3] = {0};

    size_msgs[0] = "Aw, snap! CDK doesn't currently support SIGWINCH.";
    size_msg_lens[0] = strlen(size_msgs[0]);
    size_msgs[1] = "You can not make the terminal smaller while using the interface.";
    size_msg_lens[1] = strlen(size_msgs[1]);
    size_msgs[2] = "Press any key to exit...";
    size_msg_lens[2] = strlen(size_msgs[2]);

    getmaxyx(main_screen, screen_y, screen_x);

    if (screen_y < orig_term_y || screen_x < orig_term_x) {
        /* Screen is smaller than it should be */
        destroyCDKScreenObjects(cdk);
        destroyCDKScreen(cdk);
        endCDK();
        halfdelay(REFRESH_DELAY);
        goto small_scr_start;

        /* Wait for keyboard input */
        for (;;) {
            wrefresh(main_screen);
            input_char = wgetch(main_screen);
            if (input_char == KEY_RESIZE) {
                /* Screen resize */
                small_scr_start:
                clear();
                refresh();
                getmaxyx(main_screen, screen_y, screen_x);
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - size_msg_lens[0]) / 2), size_msgs[0]);
                mvaddstr((screen_y / 2), ((screen_x - size_msg_lens[1]) / 2),
                        size_msgs[1]);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - size_msg_lens[2]) / 2), size_msgs[2]);

            } else if (input_char == ERR) {
                /* Timed out waiting for input -- loop */

            } else {
                /* User hit a key to exit */
                endwin();
                printf("\n");
                exit(EXIT_SUCCESS);
            }

        }

    } else {
        /* Screen grew in size so try to make it pretty */
        wclear(main_screen);
        wrefresh(main_screen);
        statusBar(main_screen);
        refreshCDKScreen(cdk);
        return;
    }
}


/*
 * Strip whitespace and trailing newline from a string.
 * Originally from Linux kernel lib/string.c (strim()).
 */
char *strStrip(char *string) {
    size_t size = 0;
    char *end = NULL;

    while (isspace(*string))
        ++string;

    size = strlen(string);
    if (!size)
        return string;

    end = string + size - 1;
    while (end >= string && isspace(*end))
        end--;
    *(end + 1) = '\0';

    return string;
}


/*
 * Make a nice pretty status bar; may be called multiple times in case
 * of a screen resize. Also draws the box around the window.
 */
void statusBar(WINDOW *window) {
    FILE *ver_file = NULL;
    char esos_ver[STAT_BAR_ESOS_VER_MAX] = {0}, esos_ver_str[STAT_BAR_ESOS_VER_MAX] = {0},
            username_str[STAT_BAR_UNAME_MAX] = {0};
    int esos_ver_size = 0, username_size = 0, bar_space = 0, junk = 0;
    uid_t ruid = 0, euid = 0, suid = 0;
    struct passwd *pwd = NULL;
    char *status_msg = NULL;
    chtype *status_bar = NULL;

    /* Open the ESOS release/version file and get version information */
    ver_file = fopen(ESOS_VER_FILE, "r");
    if (ver_file == NULL) {
        snprintf(esos_ver, STAT_BAR_ESOS_VER_MAX, "%s: %s", ESOS_VER_FILE,
                strerror(errno));
    } else {
        fgets(esos_ver, sizeof(esos_ver), ver_file);
        fclose(ver_file);
    }
    strncpy(esos_ver_str, strStrip(esos_ver), STAT_BAR_ESOS_VER_MAX);
    esos_ver_size = strlen(esos_ver_str);

    /* Get username */
    getresuid(&ruid, &euid, &suid);
    pwd = getpwuid(suid);
    strncpy(username_str, pwd->pw_name, STAT_BAR_UNAME_MAX);
    username_size = strlen(username_str);

    /* Figure out spacing for status bar */
    bar_space = (COLS - 2) - 2 - esos_ver_size - username_size;

    /* Box the window and make the status bar */
    boxWindow(window, COLOR_MAIN_BOX);
    asprintf(&status_msg, "</B> %s%*s%s <!B>", esos_ver_str, bar_space,
            "", username_str);
    status_bar = char2Chtype(status_msg, &junk, &junk);
    writeChtypeAttrib(window, 1, LINES-1, status_bar, COLOR_STATUS_BAR,
            HORIZONTAL, 0, COLS-2);
    freeChtype(status_bar);
    wrefresh(window);

    /* Done */
    freeChar(status_msg);
    return;
}


/*
 * Read a sysfs attribute value
 */
void readAttribute(char sysfs_attr[], char attr_value[]) {
     FILE *sysfs_file = NULL;
     char *remove_me = NULL;

     /* Open the file and retrieve the value */
     if ((sysfs_file = fopen(sysfs_attr, "r")) == NULL) {
         sprintf(attr_value, "fopen(): %s", strerror(errno));
         return;
     } else {
         fgets(attr_value, MAX_SYSFS_ATTR_SIZE, sysfs_file);
         fclose(sysfs_file);
     }

     /* Get rid of the newline character */
     remove_me = strrchr(attr_value, '\n');
     if (remove_me) {
         *remove_me = '\0';
     }

     /* Done */
     return;
}


/*
 * Write a sysfs attribute value; if an error is encountered, we return
 * errno, otherwise 0
 */
int writeAttribute(char sysfs_attr[], char attr_value[]) {
     FILE *sysfs_file = NULL;
     int ret_val = 0;

     /* Open the file and write the value */
     if ((sysfs_file = fopen(sysfs_attr, "w")) == NULL) {
         return errno;
     } else {
         fprintf(sysfs_file, "%s", attr_value);
         if ((ret_val = fclose(sysfs_file)) == 0) {
             return ret_val;
         } else {
             return errno;
         }
     }
}
