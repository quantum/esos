/*
 * $Id$
 */

#ifndef STRINGS_H
#define	STRINGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* Screen sizing/resizing */
#define NO_RESIZE "Aw, snap! This TUI doesn't handle screen shrinking."
#define NO_SMALL_TERM "You can not make the terminal smaller while using the interface."
#define ANY_KEY_EXIT "Press any key to exit..."
#define TOO_SMALL_TERM "Your terminal is not the correct size!"
#define CURR_TERM_SIZE "Current screen size: %d rows X %d columns"
#define REQD_SCREEN_SIZE "(Screen must be at least %d rows and %d columns.)"

/* Widget error messages */
#define LABEL_ERR_MSG   "Couldn't create label widget!"

/* SCST utility function error messages */
#define TGT_DRIVERS_ERR "An error occurred while retrieving the list of drivers."

/* Canned dialog messages */
#define CONTINUE_MSG    "<C></B><Press ENTER to continue...>"
#define NO_SCST_MSG     "<C></B><SCST is not loaded!>"

#ifdef	__cplusplus
}
#endif

#endif	/* STRINGS_H */

