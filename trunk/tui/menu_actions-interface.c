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
 * Run the Help dialog
 */
void helpDialog(CDKSCREEN *main_cdk_screen) {
    CDKDIALOG *help_dialog = 0;
    static char *buttons[] = {"</B>   OK   "};
    char *message[HELP_MSG_SIZE] = {NULL};
    int i = 0;

    /* Set the message */
    asprintf(&message[0], "<C></31/B>ESOS TUI Help");
    asprintf(&message[1], " ");
    asprintf(&message[2], "</B>Main Menu");
    asprintf(&message[3], "To activate the main menu, hit any of the menu hot keys. You can");
    asprintf(&message[4], "then use the arrow keys to navigate the menu, and use ENTER to make");
    asprintf(&message[5], "a selection. Use ESCAPE to exit the menu without making a selection.");
    asprintf(&message[6], " ");
    asprintf(&message[7], "</B>Navigating Dialogs");
    asprintf(&message[8], "On dialogs (screens) that contain more than one widget, you can use");
    asprintf(&message[9], "TAB and SHIFT+TAB to traverse through the widgets (field entry, radio");
    asprintf(&message[10], "lists, buttons, etc.) and use ENTER to execute button functions.");
    asprintf(&message[11], "On selection dialogs, single field entry widgets, etc. you can use");
    asprintf(&message[12], "ENTER or TAB to make a choice, or use ESCAPE to cancel.");
    asprintf(&message[13], " ");
    asprintf(&message[14], "</B>Scrolling Windows");
    asprintf(&message[15], "When a scrolling window widget is active, you can use the arrow keys");
    asprintf(&message[16], "to scroll through the text and use ENTER or ESCAPE to exit.");

    /* Display the TUI help message dialog box */
    help_dialog = newCDKDialog(main_cdk_screen, CENTER, CENTER, message, HELP_MSG_SIZE,
            buttons, 1, COLOR_DIALOG_SELECT, TRUE, TRUE, FALSE);
    if (!help_dialog) {
        errorDialog(main_cdk_screen, "Couldn't create dialog widget!", NULL);
        goto cleanup;
    }
    setCDKDialogBackgroundAttrib(help_dialog, COLOR_DIALOG_TEXT);
    setCDKDialogBoxAttribute(help_dialog, COLOR_DIALOG_BOX);
    /* We don't care how the user exits the widget */
    activateCDKDialog(help_dialog, 0);
    destroyCDKDialog(help_dialog);

    /* All done */
    cleanup:
    for (i = 0; i < HELP_MSG_SIZE; i++)
        freeChar(message[i]);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/*
 * Run the Support Bundle dialog
 */
void supportArchDialog(CDKSCREEN *main_cdk_screen) {
    return;
}


/*
 * Run the About dialog
 */
void aboutDialog(CDKSCREEN *main_cdk_screen) {
    return;
}

