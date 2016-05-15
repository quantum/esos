/**
 * @file menu_lvm.c
 * @brief Contains the menu actions for the 'LVM' menu.
 * @author Copyright (c) 2012-2016 Marc A. Smith
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


/*
 * Run the LVM2 LV Information dialog
 */
void lvm2InfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *lvm2_info = 0;
    char *swindow_info[MAX_LVM2_INFO_LINES] = {NULL};
    char *error_msg = NULL, *lvdisplay_cmd = NULL;
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
            lvm2_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                    LVM2_INFO_ROWS+2, LVM2_INFO_COLS+2,
                    "<C></31/B>LVM2 Logical Volume Information\n",
                    MAX_LVM2_INFO_LINES, TRUE, FALSE);
            if (!lvm2_info) {
                errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(lvm2_info, COLOR_DIALOG_TEXT);
            setCDKSwindowBoxAttribute(lvm2_info, COLOR_DIALOG_BOX);

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
    for (i = 0; i < MAX_LVM2_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    return;
}
