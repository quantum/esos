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
#include <fcntl.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"

int main(int argc, char** argv) {
    CDKSCREEN *cdk_screen = 0;
    WINDOW *main_window = 0, *sub_window = 0;
    CDKMENU *menu = 0;
    CDKLABEL *targets_label = 0, *sessions_label = 0;
    const char *menu_list[MAX_MENU_ITEMS][MAX_SUB_ITEMS] = {{NULL}, {NULL}};
    char *tgt_label_msg[MAX_INFO_LABEL_ROWS] = {NULL},
            *sess_label_msg[MAX_INFO_LABEL_ROWS] = {NULL};
    char *error_msg = NULL;
    int selection = 0, key_pressed = 0, menu_choice = 0, submenu_choice = 0,
            screen_x = 0, screen_y = 0, latest_scr_y = 0, latest_scr_x = 0,
            i = 0, child_status = 0, proc_status = 0, tty_fd = 0,
            labels_last_scr_y = 0, labels_last_scr_x = 0,
            last_tgt_lbl_rows = 0, last_sess_lbl_rows = 0;
    int submenu_size[CDK_MENU_MAX_SIZE] = {0},
            menu_loc[CDK_MENU_MAX_SIZE] = {0};
    pid_t child_pid = 0;
    uid_t saved_uid = 0;

    /* Make sure the umask is something sane (per the man
     * page, this call always succeeds) */
    umask(0022);

    /* Initialize screen and check size */
    start:
    main_window = initscr();
    curs_set(0);
    noecho();
    getmaxyx(main_window, screen_y, screen_x);
    if (screen_y < MIN_SCR_Y || screen_x < MIN_SCR_X) termSize(main_window);

    /* Setup CDK */
    sub_window = newwin((LINES - 2), (COLS - 2), 1, 1);
    latest_scr_y = LINES;
    latest_scr_x = COLS;
    wbkgd(main_window, COLOR_MAIN_TEXT);
    wbkgd(sub_window, COLOR_MAIN_TEXT);
    cdk_screen = initCDKScreen(sub_window);
    initCDKColor();

    /* Create the menu lists */
    menu_list[SYSTEM_MENU][0] = "</29/B/U>S<!29><!U>ystem  <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_SYNC_CONF] = \
            "</B>Sync. Configuration <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_NETWORK] = \
            "</B>Network Settings    <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_RESTART_NET] = \
            "</B>Restart Networking  <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_MAIL] = \
            "</B>Mail Setup          <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_TEST_EMAIL] = \
            "</B>Send Test Email     <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_ADD_USER] = \
            "</B>Add User            <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_DEL_USER] = \
            "</B>Delete User         <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_CHG_PASSWD] = \
            "</B>Change Password     <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_SCST_INFO] = \
            "</B>SCST Information    <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_CRM_STATUS] = \
            "</B>CRM Status          <!B>";
    menu_list[SYSTEM_MENU][SYSTEM_DATE_TIME] = \
            "</B>Date & Time Settings<!B>";

    menu_list[BACK_STORAGE_MENU][0] = "</29/B/U>B<!29><!U>ack-End Storage  <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADP_PROP] = \
            "</B>Adapter Properties      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADP_INFO] = \
            "</B>Adapter Information     <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADD_VOL] = \
            "</B>Add Volume              <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DEL_VOL] = \
            "</B>Delete Volume           <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_VOL_PROP] = \
            "</B>Volume Properties       <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DRBD_STAT] = \
            "</B>DRBD Status             <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_SOFT_RAID_STAT] = \
            "</B>Software RAID Status    <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_LVM2_INFO] = \
            "</B>LVM2 LV Information     <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_CREATE_FS] = \
            "</B>Create File System      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_REMOVE_FS] = \
            "</B>Remove File System      <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_ADD_VDISK_FILE] = \
            "</B>Add Virtual Disk File   <!B>";
    menu_list[BACK_STORAGE_MENU][BACK_STORAGE_DEL_VDISK_FILE] = \
            "</B>Delete Virtual Disk File<!B>";

    menu_list[HOSTS_MENU][0] = "</29/B/U>H<!29><!U>osts  <!B>";
    menu_list[HOSTS_MENU][HOSTS_ADD_GROUP] = \
            "</B>Add Group       <!B>";
    menu_list[HOSTS_MENU][HOSTS_REM_GROUP] = \
            "</B>Remove Group    <!B>";
    menu_list[HOSTS_MENU][HOSTS_ADD_INIT] = \
            "</B>Add Initiator   <!B>";
    menu_list[HOSTS_MENU][HOSTS_REM_INIT] = \
            "</B>Remove Initiator<!B>";

    menu_list[DEVICES_MENU][0] = "</29/B/U>D<!29><!U>evices  <!B>";
    menu_list[DEVICES_MENU][DEVICES_LUN_LAYOUT] = \
            "</B>LUN Layout        <!B>";
    menu_list[DEVICES_MENU][DEVICES_DEV_INFO] = \
            "</B>Device Information<!B>";
    menu_list[DEVICES_MENU][DEVICES_ADD_DEV] = \
            "</B>Add Device        <!B>";
    menu_list[DEVICES_MENU][DEVICES_DEL_DEV] = \
            "</B>Delete Device     <!B>";
    menu_list[DEVICES_MENU][DEVICES_MAP_TO] = \
            "</B>Map to Group      <!B>";
    menu_list[DEVICES_MENU][DEVICES_UNMAP_FROM] = \
            "</B>Unmap from Group  <!B>";

    menu_list[TARGETS_MENU][0] = "</29/B/U>T<!29><!U>argets  <!B>";
    menu_list[TARGETS_MENU][TARGETS_TGT_INFO] = \
            "</B>Target Information   <!B>";
    menu_list[TARGETS_MENU][TARGETS_ADD_ISCSI] = \
            "</B>Add iSCSI Target     <!B>";
    menu_list[TARGETS_MENU][TARGETS_REM_ISCSI] = \
            "</B>Remove iSCSI Target  <!B>";
    menu_list[TARGETS_MENU][TARGETS_LIP] = \
            "</B>Issue LIP            <!B>";
    menu_list[TARGETS_MENU][TARGETS_TOGGLE] = \
            "</B>Enable/Disable Target<!B>";

    menu_list[INTERFACE_MENU][0] = "</29/B/U>I<!29><!U>nterface<!B>";
    menu_list[INTERFACE_MENU][INTERFACE_QUIT] = \
            "</B>Quit          <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_SHELL] = \
            "</B>Exit to Shell <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_HELP] = \
            "</B>Help          <!B>";
    menu_list[INTERFACE_MENU][INTERFACE_SUPPORT_PKG] = \
            "</B>Support Bundle<!B>";
    menu_list[INTERFACE_MENU][INTERFACE_ABOUT] = \
            "</B>About         <!B>";

    /* Set menu sizes and locations */
    submenu_size[SYSTEM_MENU]       = 12;
    menu_loc[SYSTEM_MENU]           = LEFT;
    submenu_size[BACK_STORAGE_MENU] = 13;
    menu_loc[BACK_STORAGE_MENU]     = LEFT;
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
    if (!menu) {
        errorDialog(cdk_screen, MENU_ERR_MSG, NULL);
        goto quit;
    }
    setCDKMenuBackgroundColor(menu, "</5>");

    /* We need root privileges; for the short term I don't see any other way
     * around this; long term we can hopefully do something else */
    saved_uid = getuid();
    if (setresuid(0, -1, saved_uid) == -1) {
        asprintf(&error_msg, "setresuid(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg,
                "Your capabilities may be reduced...");
        FREE_NULL(error_msg);
    }

    /* Draw the CDK screen */
    statusBar(main_window);
    refreshCDKScreen(cdk_screen);

    /* Check and see if SCST is loaded */
    if (! isSCSTLoaded()) {
        errorDialog(cdk_screen, "It appears SCST is not loaded; a number",
                "of the TUI functions will not work.");
    }

    /* Loop, refreshing the labels and waiting for input */
    halfdelay(REFRESH_DELAY);
    for (;;) {
        /* Update the information labels */
        if (!updateInfoLabels(cdk_screen, &targets_label, &sessions_label,
                tgt_label_msg, sess_label_msg,
                &labels_last_scr_y, &labels_last_scr_x,
                &last_tgt_lbl_rows, &last_sess_lbl_rows))
            goto quit;

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
            screenResize(cdk_screen, main_window, sub_window,
                    &latest_scr_y, &latest_scr_x);
            continue;

        } else if (key_pressed == ERR) {
            /* Looks like the user didn't press anything (half-delay mode) */
            continue;

        } else {
            beep();
            continue;
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
                    FREE_NULL(error_msg);
                }
                /* Fork and execute a shell */
                if ((child_pid = fork()) < 0) {
                    /* Could not fork */
                    asprintf(&error_msg, "fork(): %s", strerror(errno));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                } else if (child_pid == 0) {
                    /* Child; fix up the terminal and execute the shell */
                    endwin();
                    curs_set(1);
                    echo();
                    system(CLEAR_BIN);
                    /* Execute the shell; if we fail, print something 
                     * useful to syslog */
                    if ((execl(SHELL_BIN, SHELL_BIN, "--rcfile", GLOBAL_BASHRC,
                            "-i", (char *) NULL)) == -1) {
                        openlog(LOG_PREFIX, LOG_OPTIONS, LOG_FACILITY);
                        syslog(LOG_ERR, "Calling execl() failed: %s",
                                strerror(errno));
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
                    /* Ending everything and starting fresh seems to work
                     * best when switching between the shell and UI */
                    destroyCDKLabel(targets_label);
                    targets_label = NULL;
                    destroyCDKLabel(sessions_label);
                    sessions_label = NULL;
                    destroyCDKScreenObjects(cdk_screen);
                    destroyCDKScreen(cdk_screen);
                    endCDK();
                    cdk_screen = NULL;

                    /* Check and see if we're still attached to our terminal */
                    if ((tty_fd = open("/dev/tty", O_RDONLY)) == -1) {
                        /* Guess not, so we're done */
                        goto quit;
                    } else {
                        close(tty_fd);
                        goto start;
                    }
                }

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_QUIT - 1) {
                /* Synchronize the configuration and quit */
                syncConfig(cdk_screen);
                goto quit;

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

        /* Done with the menu, go back to half-delay mode */
        halfdelay(REFRESH_DELAY);
    }

    /* All done -- clean up */
quit:
    if (cdk_screen != NULL) {
        destroyCDKScreenObjects(cdk_screen);
        destroyCDKScreen(cdk_screen);
        endCDK();
    }
    delwin(sub_window);
    delwin(main_window);
    for (i = 0; i < MAX_INFO_LABEL_ROWS; i++) {
        FREE_NULL(tgt_label_msg[i]);
        FREE_NULL(sess_label_msg[i]);
    }
    system(CLEAR_BIN);
    exit(EXIT_SUCCESS);
}


/*
 * Helper to fix initial terminal/screen size (before CDK initialization). If
 * the screen size is less than the minimum, keep displaying the message until
 * the user resizes to the minimum values.
 */
void termSize(WINDOW *screen) {
    int input_char = 0, screen_x = 0, screen_y = 0;
    char curr_term_size[MISC_STRING_LEN] = {0},
            reqd_screen_size[MISC_STRING_LEN] = {0};
    boolean first_run = true;

    /* Start half-delay mode */
    halfdelay(REFRESH_DELAY);

    for (;;) {
        /* We only want to wait for input if this is not the first loop */
        if (!first_run) {
            wrefresh(screen);
            input_char = wgetch(screen);
        } else {
            input_char = KEY_RESIZE;
        }

        /* Wait for a resize event or escape */
        if (input_char == KEY_RESIZE) {
            first_run = false;
            clear();
            refresh();
            getmaxyx(screen, screen_y, screen_x);

            /* Check new screen size and return if we meet the minimums */
            if (screen_y >= MIN_SCR_Y && screen_x >= MIN_SCR_X) {
                return;
            } else {
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - strlen(TOO_SMALL_TERM)) / 2),
                        TOO_SMALL_TERM);
                snprintf(curr_term_size, MISC_STRING_LEN,
                        CURR_TERM_SIZE, screen_y, screen_x);
                mvaddstr((screen_y / 2),
                        ((screen_x - strlen(curr_term_size)) / 2),
                        curr_term_size);
                snprintf(reqd_screen_size, MISC_STRING_LEN,
                        REQD_SCREEN_SIZE, MIN_SCR_Y, MIN_SCR_X);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - strlen(reqd_screen_size)) / 2),
                        reqd_screen_size);
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
 * Handle terminal resize (SIGWINCH / KEY_RESIZE). If the screen is made
 * smaller than the last size, we display a message to the user until
 * they exit. If its larger, we try to handle that gracefully.
 */
void screenResize(CDKSCREEN *cdk_screen, WINDOW *main_window,
        WINDOW *sub_window, int *latest_term_y, int *latest_term_x) {
    int input_char = 0, screen_y = 0, screen_x = 0;
    boolean first_run = true;

    getmaxyx(main_window, screen_y, screen_x);
    if ((screen_y < *latest_term_y) || (screen_x < *latest_term_x)) {
        /* Screen is smaller than it should be */
        destroyCDKScreenObjects(cdk_screen);
        destroyCDKScreen(cdk_screen);
        endCDK();
        halfdelay(REFRESH_DELAY);

        for (;;) {
            /* We only want to wait for input if this is not the first loop */
            if (!first_run) {
                wrefresh(main_window);
                input_char = wgetch(main_window);
            } else {
                input_char = KEY_RESIZE;
            }

            /* A SIGWINCH generates a KEY_RESIZE character with our ncurses */
            if (input_char == KEY_RESIZE) {
                first_run = false;
                clear();
                refresh();
                getmaxyx(main_window, screen_y, screen_x);
                /* The screen size has changed so print a fancy message */
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - strlen(NO_RESIZE)) / 2),
                        NO_RESIZE);
                mvaddstr((screen_y / 2),
                        ((screen_x - strlen(NO_SMALL_TERM)) / 2),
                        NO_SMALL_TERM);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - strlen(ANY_KEY_EXIT)) / 2),
                        ANY_KEY_EXIT);

            } else if (input_char == ERR) {
                /* Timed out waiting for input -- loop */
                continue;

            } else {
                /* User hit a key to exit */
                endwin();
                printf("\n");
                exit(EXIT_SUCCESS);
            }
        }

    } else {
        /* Screen grew in size so make it pretty */
        wresize(sub_window, (screen_y - 2), (screen_x - 2));
        wclear(main_window);
        wrefresh(main_window);
        statusBar(main_window);
        refreshCDKScreen(cdk_screen);
        /* Set these for the next trip */
        *latest_term_y = screen_y;
        *latest_term_x = screen_x;
        return;
    }
}


/*
 * Make a nice pretty status bar; may be called multiple times in case
 * of a screen resize. Also draws the box around the window.
 */
void statusBar(WINDOW *window) {
    FILE *ver_file = NULL;
    char esos_ver[STAT_BAR_ESOS_VER_MAX] = {0},
            esos_ver_str[STAT_BAR_ESOS_VER_MAX] = {0},
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
    FREE_NULL(status_msg);
    return;
}
