/**
 * @file menu_softraid.c
 * @brief Contains the menu actions for the 'Software RAID' menu.
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
    informDialog(main_cdk_screen, "TODO: This still needs to be implemented.",
            NULL);
}


/**
 * @brief Run the "Remove Array" dialog.
 */
void remArrayDialog(CDKSCREEN *main_cdk_screen) {
    informDialog(main_cdk_screen, "TODO: This still needs to be implemented.",
            NULL);
}


/**
 * @brief Run the "Set Device Faulty" dialog.
 */
void faultDevDialog(CDKSCREEN *main_cdk_screen) {
    informDialog(main_cdk_screen, "TODO: This still needs to be implemented.",
            NULL);
}


/**
 * @brief Run the "Add Device" dialog.
 */
void addDevDialog(CDKSCREEN *main_cdk_screen) {
    informDialog(main_cdk_screen, "TODO: This still needs to be implemented.",
            NULL);
}


/**
 * @brief Run the "Remove Device" dialog.
 */
void remDevDialog(CDKSCREEN *main_cdk_screen) {
    informDialog(main_cdk_screen, "TODO: This still needs to be implemented.",
            NULL);
}
