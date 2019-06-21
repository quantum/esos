/**
 * @file info_labels.c
 * @brief Functions for the TUI main screen information labels.
 * @author Copyright (c) 2019 Quantum Corporation
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cdk.h>
#include <sys/param.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/**
 * @brief This function is responsible for moving, resizing, and updating the
 * message lines in the main screen information labels. It will read the screen
 * size and message data for each label then act based on this data. It should
 * be called when the terminal is re-sized and frequently to update the display.
 */
boolean updateInfoLabels(CDKSCREEN *cdk_screen,
        CDKLABEL **tgt_info, CDKLABEL **sess_info,
        char *tgt_info_msg[],  char *sess_info_msg[],
        int *last_scr_y, int *last_scr_x,
        int *last_tgt_rows, int *last_sess_rows) {
    int window_x = 0, window_y = 0, usable_height = 0, half_height = 0,
            tgt_want_rows = 0, sess_want_rows = 0, tgt_lbl_rows = 0,
            sess_lbl_rows = 0, tgt_lbl_height = 0, sess_lbl_height = 0,
            tgt_y_start = 0, sess_y_start = 0,
            smallest_val = 0, largest_val = 0;
    boolean success = TRUE;

    /* Fill the label messages and get sizes */
    tgt_want_rows = readTargetData(tgt_info_msg);
    sess_want_rows = readSessionData(sess_info_msg);

    /* Figure out how much real estate we have */
    getmaxyx(cdk_screen->window, window_y, window_x);
    usable_height = window_y - 2;
    /* Its okay if its odd, integer division will truncate (1 spare row) */
    half_height = usable_height / 2;

    if (tgt_want_rows == sess_want_rows) {
        /* If they are both equal, they get equal amounts of the screen */
        tgt_lbl_height = half_height;
        sess_lbl_height = half_height;
        /* We like the sessions label, so if its odd, add the row here */
        if (usable_height % 2)
            sess_lbl_height++;
        tgt_lbl_rows = tgt_lbl_height - 2;
        sess_lbl_rows = sess_lbl_height - 2;

    } else {
        smallest_val = MIN(tgt_want_rows, sess_want_rows);
        largest_val = MAX(tgt_want_rows, sess_want_rows);
        if (smallest_val > half_height) {
            /* If the smallest one is greater than half, go 50/50 */
            tgt_lbl_height = half_height;
            sess_lbl_height = half_height;
            /* We like the sessions label, so if its odd, add the row here */
            if (usable_height % 2)
                sess_lbl_height++;
            tgt_lbl_rows = tgt_lbl_height - 2;
            sess_lbl_rows = sess_lbl_height - 2;

        } else {
            /* Find out what variable is the bigger one */
            if (tgt_want_rows == largest_val) {
                sess_lbl_height = smallest_val + 2;
                tgt_lbl_height = usable_height - sess_lbl_height;
            } else {
                tgt_lbl_height = smallest_val + 2;
                sess_lbl_height = usable_height - tgt_lbl_height;
            }
            tgt_lbl_rows = tgt_lbl_height - 2;
            sess_lbl_rows = sess_lbl_height - 2;
        }
    }

    /* Left-side y-values for the label widgets */
    tgt_y_start = 3;
    sess_y_start = tgt_y_start + tgt_lbl_height;

    /* If the screen size has changed, or either label row count has changed
     * then we need to delete the labels and start fresh */
    if ((window_y != *last_scr_y) || (window_x != *last_scr_x) ||
            (tgt_lbl_rows != *last_tgt_rows) ||
            (sess_lbl_rows != *last_sess_rows)) {
        if (*tgt_info != NULL) {
            destroyCDKLabel(*tgt_info);
            *tgt_info = NULL;
        }
        if (*sess_info != NULL) {
            destroyCDKLabel(*sess_info);
            *sess_info = NULL;
        }
        /* Set these here for next time around */
        *last_scr_y = window_y;
        *last_scr_x = window_x;
        *last_tgt_rows = tgt_lbl_rows;
        *last_sess_rows = sess_lbl_rows;
    }

    /* Create the information/status labels (if they don't exist) */
    while (1) {
        if (*tgt_info == NULL) {
            *tgt_info = newCDKLabel(cdk_screen, 1, tgt_y_start,
                    tgt_info_msg, tgt_lbl_rows, TRUE, FALSE);
            if (!*tgt_info) {
                errorDialog(cdk_screen, LABEL_ERR_MSG, NULL);
                success = FALSE;
                break;
            }
            setCDKLabelBoxAttribute(*tgt_info, g_color_main_box[g_curr_theme]);
            setCDKLabelBackgroundAttrib(*tgt_info,
                    g_color_main_text[g_curr_theme]);
        }
        if (*sess_info == NULL) {
            *sess_info = newCDKLabel(cdk_screen, 1, sess_y_start,
                    sess_info_msg, sess_lbl_rows, TRUE, FALSE);
            if (!*sess_info) {
                errorDialog(cdk_screen, LABEL_ERR_MSG, NULL);
                success = FALSE;
                break;
            }
            setCDKLabelBoxAttribute(*sess_info, g_color_main_box[g_curr_theme]);
            setCDKLabelBackgroundAttrib(*sess_info,
                    g_color_main_text[g_curr_theme]);
        }
        break;
    }

    /* Refresh information label messages */
    if (success) {
        setCDKLabelMessage(*tgt_info, tgt_info_msg, tgt_lbl_rows);
        setCDKLabelMessage(*sess_info, sess_info_msg, sess_lbl_rows);
    }

    /* Done */
    return success;
}


/**
 * @brief This function will fill an array of char pointers for the "targets"
 * information label (main screen). The return value is the number of rows
 * that should be displayed in the label. If an error occurs, we simply
 * print the error message in the label row data and return.
 */
int readTargetData(char *label_msg[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int row_cnt = 0, i = 0, j = 0, driver_cnt = 0, temp_int = 0,
            fc_adp_cnt = 0, ib_adp_cnt = 0, port_name_size = 0, tgt_cnt = 0;
    char line_buffer[TARGETS_LABEL_COLS],
            dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_val[MAX_SYSFS_ATTR_SIZE] = {0};
    char tgt_drivers[MAX_SCST_DRIVERS][MISC_STRING_LEN] = {{0}, {0}},
            driver_name[MAX_SCST_SESSNS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            tgt_name[MAX_SCST_SESSNS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            tgt_state[MAX_SCST_SESSNS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            fc_port_name[MAX_FC_ADAPTERS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            fc_speed[MAX_FC_ADAPTERS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            ib_node_guid[MAX_IB_ADAPTERS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            ib_port_speed[MAX_IB_ADAPTERS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}},
            tgt_speed_str[MAX_SCST_SESSNS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}};
    char *temp_pstr = NULL;

    /* Clear the label message */
    for (i = 0; i < MAX_INFO_LABEL_ROWS; i++)
        FREE_NULL(label_msg[i]);

    /* Set the initial label messages; the number of characters
     * controls the label width (using white space as padding for width) */
    SAFE_ASPRINTF(&label_msg[0],
            "</%d/B/U>Target<!%d><!B><!U>                           "
            "</%d/B/U>Driver<!%d><!B><!U>     "
            "</%d/B/U>State<!%d><!B><!U>      "
            "</%d/B/U>Link Speed<!%d><!B><!U>          ",
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme]);

    /* We start our row 1 down (skip title) */
    row_cnt = 1;

    /* Print a nice message if SCST isn't loaded and return */
    if (!isSCSTLoaded()) {
        snprintf(line_buffer, TARGETS_LABEL_COLS, NO_SCST_MSG);
        SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        row_cnt++;
        return row_cnt;
    }

    /* Fill the array with current SCST target drivers */
    if (!listSCSTTgtDrivers(tgt_drivers, &driver_cnt)) {
        snprintf(line_buffer, TARGETS_LABEL_COLS, TGT_DRIVERS_ERR);
        SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        row_cnt++;
        return row_cnt;
    }

    for (i = 0; i < driver_cnt; i++) {
        /* Open the directory for targets */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s", SYSFS_SCST_TGT, tgt_drivers[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            if (row_cnt < MAX_INFO_LABEL_ROWS) {
                snprintf(line_buffer, SESSIONS_LABEL_COLS, "opendir(): %s",
                        strerror(errno));
                SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
                row_cnt++;
            }
            return row_cnt;
        }
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            /* The target names are directories */
            if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
                /* Store the driver name for this target */
                snprintf(driver_name[j], MAX_SYSFS_ATTR_SIZE, tgt_drivers[i]);
                /* Store the target name */
                snprintf(tgt_name[j], MAX_SYSFS_ATTR_SIZE, dir_entry->d_name);
                /* Get the target enabled/disabled attribute */
                snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/enabled", SYSFS_SCST_TGT,
                        tgt_drivers[i], dir_entry->d_name);
                readAttribute(attr_path, attr_val);
                temp_int = atoi(attr_val);
                if (temp_int == 1)
                    snprintf(tgt_state[j], MAX_SYSFS_ATTR_SIZE, "Enabled");
                else
                    snprintf(tgt_state[j], MAX_SYSFS_ATTR_SIZE, "Disabled");
                j++;
            }
        }
        /* Close the target directory stream */
        closedir(dir_stream);
    }

    /* Fibre Channel / FCoE adapter information */
    if ((dir_stream = opendir(SYSFS_FC_HOST)) == NULL) {
        if (row_cnt < MAX_INFO_LABEL_ROWS) {
            snprintf(line_buffer, TARGETS_LABEL_COLS, "opendir(): %s",
                    strerror(errno));
            SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        }
        return row_cnt;
    }
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The hostX directory names are links */
        if (dir_entry->d_type == DT_LNK) {
            /* Get the port name */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/port_name",
                    SYSFS_FC_HOST, dir_entry->d_name);
            readAttribute(attr_path, attr_val);
            /* Make the port name match a SCST target name */
            temp_pstr = strchr(attr_val, 'x');
            if (temp_pstr != NULL) {
                /* In sysfs, if the WWPN begins with a '0' then in the
                 * port_name sysfs attribute, the leading '0' character is
                 * truncated, fix this by changing the 'x' to a '0' */
                if (strlen(temp_pstr) != 17)
                    *temp_pstr = '0';
                else
                    temp_pstr++;
                while ((temp_int = strlen(temp_pstr)) != 0) {
                    if (!((temp_int - 2) < 0)) {
                        if (port_name_size >= MAX_SYSFS_ATTR_SIZE) {
                            break;
                        } else {
                            strncat(fc_port_name[fc_adp_cnt], temp_pstr, 2);
                            port_name_size = port_name_size + 2;
                            if ((temp_int - 2) != 0) {
                                strncat(fc_port_name[fc_adp_cnt], ":", 1);
                                port_name_size++;
                            }
                            temp_pstr = temp_pstr + 2;
                        }
                    } else {
                        break;
                    }
                }
            }
            /* Get speed string */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/speed",
                    SYSFS_FC_HOST, dir_entry->d_name);
            readAttribute(attr_path, attr_val);
            snprintf(fc_speed[fc_adp_cnt], MAX_SYSFS_ATTR_SIZE, "%s", attr_val);
        }
        fc_adp_cnt++;
    }
    closedir(dir_stream);

    /* InfiniBand HCA information */
    if ((dir_stream = opendir(SYSFS_INFINIBAND)) == NULL) {
        if (row_cnt < MAX_INFO_LABEL_ROWS) {
            snprintf(line_buffer, TARGETS_LABEL_COLS, "opendir(): %s",
                    strerror(errno));
            SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        }
        return row_cnt;
    }
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The IB directory names are links */
        if (dir_entry->d_type == DT_LNK) {
            /* Get the HCA node GUID (this matches what SCST has) */
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/node_guid",
                    SYSFS_INFINIBAND, dir_entry->d_name);
            readAttribute(attr_path, attr_val);
            snprintf(ib_node_guid[ib_adp_cnt], MAX_SYSFS_ATTR_SIZE, attr_val);

            /* Get the port rate/speed */
            // TODO: It may be incorrect to assume there is only 1 port!
            snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/%s/ports/1/rate",
                    SYSFS_INFINIBAND, dir_entry->d_name);
            readAttribute(attr_path, attr_val);
            snprintf(ib_port_speed[ib_adp_cnt], MAX_SYSFS_ATTR_SIZE, attr_val);
        }
        ib_adp_cnt++;
    }
    closedir(dir_stream);

    /* Fill the label lines */
    tgt_cnt = j;
    for (i = 0; i < tgt_cnt; i++) {
        for (j = 0; j < fc_adp_cnt; j++) {
            if (strcmp(tgt_name[i], fc_port_name[j]) == 0) {
                /* Add speed/rate if its a matching FC adapter */
                snprintf(tgt_speed_str[i], MAX_SYSFS_ATTR_SIZE,
                        "%s", fc_speed[j]);
                break;
            }
        }
        for (j = 0; j < ib_adp_cnt; j++) {
            if (strcmp(tgt_name[i], ib_node_guid[j]) == 0) {
                /* Add speed/rate if its a matching IB adapter */
                snprintf(tgt_speed_str[i], MAX_SYSFS_ATTR_SIZE,
                        "%s", ib_port_speed[j]);
                break;
            }
        }
        if (tgt_speed_str[i][0] == '\0')
            snprintf(tgt_speed_str[i], MAX_SYSFS_ATTR_SIZE, "N/A");
        /* Put it all together */
        if (row_cnt < MAX_INFO_LABEL_ROWS) {
            snprintf(line_buffer, TARGETS_LABEL_COLS,
                    "%-32.32s %-10.10s %-10.10s %-20.20s",
                    prettyShrinkStr(32, tgt_name[i]), driver_name[i],
                    tgt_state[i], tgt_speed_str[i]);
            SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
            row_cnt++;
        }
    }

    /* Done */
    if (row_cnt == 1) {
        /* Add a blank line if there are no rows of data */
        SAFE_ASPRINTF(&label_msg[row_cnt], " ");
        row_cnt++;
    }
    return row_cnt;
}


/**
 * @brief This function will fill an array of char pointers for the "sessions"
 * information label (main screen). The return value is the number of rows
 * that should be displayed in the label. If an error occurs, we simply
 * print the error message in the label row data and return.
 */
int readSessionData(char *label_msg[]) {
    DIR *tgt_dir_stream = NULL, *sess_dir_stream = NULL;
    struct dirent *tgt_dir_entry = NULL, *sess_dir_entry = NULL;
    int i = 0, j = 0, row_cnt = 0, driver_cnt = 0, num_sessions = 0,
            max_index = 0, tmp_act_cmds = 0, tmp_lun_cnt = 0;
    int active_cmds[MAX_SCST_SESSNS] = {0}, lun_count[MAX_SCST_SESSNS] = {0};
    unsigned long long read_io_kb[MAX_SCST_SESSNS] = {0},
            write_io_kb[MAX_SCST_SESSNS] = {0};
    unsigned long long tmp_read_io = 0, tmp_write_io = 0;
    char line_buffer[SESSIONS_LABEL_COLS],
            tgt_dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            sess_dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            attr_path[MAX_SYSFS_PATH_SIZE] = {0},
            attr_val[MAX_SYSFS_ATTR_SIZE] = {0},
            tmp_init_name[MAX_SYSFS_ATTR_SIZE] = {0};
    char tgt_drivers[MAX_SCST_DRIVERS][MISC_STRING_LEN] = {{0}, {0}},
            init_names[MAX_SCST_SESSNS][MAX_SYSFS_ATTR_SIZE] = {{0}, {0}};

    /* Clear the label message */
    for (i = 0; i < MAX_INFO_LABEL_ROWS; i++)
        FREE_NULL(label_msg[i]);

    /* Set the initial label messages; the number of characters
     * controls the label width (using white space as padding for width) */
    SAFE_ASPRINTF(&label_msg[0],
            "</%d/B/U>Session<!%d><!B><!U>                  "
            "  </%d/B/U>LUNs<!%d><!B><!U>"
            "  </%d/B/U>Cmds<!%d><!B><!U>"
            "       </%d/B/U>Read IO (KB)<!%d><!B><!U>"
            "      </%d/B/U>Write IO (KB)<!%d><!B><!U> ",
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme],
            g_color_info_header[g_curr_theme]);

    /* We start our row 1 down (skip title) */
    row_cnt = 1;

    /* Print a nice message if SCST isn't loaded and return */
    if (!isSCSTLoaded()) {
        snprintf(line_buffer, SESSIONS_LABEL_COLS, NO_SCST_MSG);
        SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        row_cnt++;
        return row_cnt;
    }

    /* Fill the array with current SCST target drivers */
    if (!listSCSTTgtDrivers(tgt_drivers, &driver_cnt)) {
        snprintf(line_buffer, SESSIONS_LABEL_COLS, TGT_DRIVERS_ERR);
        SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
        row_cnt++;
        return row_cnt;
    }

    for (i = 0; i < driver_cnt; i++) {
        /* Open the directory for targets */
        snprintf(tgt_dir_name, MAX_SYSFS_PATH_SIZE,
                "%s/targets/%s", SYSFS_SCST_TGT, tgt_drivers[i]);
        if ((tgt_dir_stream = opendir(tgt_dir_name)) == NULL) {
            if (row_cnt < MAX_INFO_LABEL_ROWS) {
                snprintf(line_buffer, SESSIONS_LABEL_COLS, "opendir(): %s",
                        strerror(errno));
                SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
                row_cnt++;
            }
            return row_cnt;
        }
        while ((tgt_dir_entry = readdir(tgt_dir_stream)) != NULL) {
            /* The target names are directories */
            if ((tgt_dir_entry->d_type == DT_DIR) &&
                    (strcmp(tgt_dir_entry->d_name, ".") != 0) &&
                    (strcmp(tgt_dir_entry->d_name, "..") != 0)) {
                /* Open the directory for sessions */
                snprintf(sess_dir_name, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/sessions",
                        SYSFS_SCST_TGT, tgt_drivers[i],
                        tgt_dir_entry->d_name);
                if ((sess_dir_stream = opendir(sess_dir_name)) == NULL) {
                    closedir(tgt_dir_stream);
                    if (row_cnt < MAX_INFO_LABEL_ROWS) {
                        snprintf(line_buffer, SESSIONS_LABEL_COLS,
                                "opendir(): %s", strerror(errno));
                        SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
                        row_cnt++;
                    }
                    return row_cnt;
                }
                while ((sess_dir_entry = readdir(sess_dir_stream)) != NULL) {
                    /* The session names are directories */
                    if ((sess_dir_entry->d_type == DT_DIR) &&
                            (strcmp(sess_dir_entry->d_name, ".") != 0) &&
                            (strcmp(sess_dir_entry->d_name, "..") != 0)) {
                        /* Get the initiator name attribute */
                        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                                "%s/targets/%s/%s/sessions/%s/initiator_name",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name,
                                sess_dir_entry->d_name);
                        readAttribute(attr_path, attr_val);
                        snprintf(init_names[j], MAX_SYSFS_ATTR_SIZE,
                                "%s", attr_val);
                        /* Get the number of LUNs for this session */
                        lun_count[j] = countSCSTSessLUNs(tgt_dir_entry->d_name,
                                tgt_drivers[i], sess_dir_entry->d_name);
                        /* Get the active commands attribute */
                        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                                "%s/targets/%s/%s/sessions/%s/active_commands",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name,
                                sess_dir_entry->d_name);
                        readAttribute(attr_path, attr_val);
                        active_cmds[j] = atoi(attr_val);
                        /* Get the read IO (in KB) attribute */
                        snprintf(attr_path, MAX_SYSFS_PATH_SIZE,
                                "%s/targets/%s/%s/sessions/%s/read_io_count_kb",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name,
                                sess_dir_entry->d_name);
                        readAttribute(attr_path, attr_val);
                        errno = 0;
                        read_io_kb[j] = strtoull(attr_val, NULL, 10);
                        if (errno != 0) {
                            closedir(sess_dir_stream);
                            closedir(tgt_dir_stream);
                            if (row_cnt < MAX_INFO_LABEL_ROWS) {
                                snprintf(line_buffer, SESSIONS_LABEL_COLS,
                                        "strtoull(): %s", strerror(errno));
                                SAFE_ASPRINTF(&label_msg[row_cnt], "%s",
                                        line_buffer);
                                row_cnt++;
                            }
                            return row_cnt;
                        }
                        /* Get the write IO (in KB) attribute */
                        snprintf(attr_path, MAX_SYSFS_PATH_SIZE, "%s/targets/"
                                "%s/%s/sessions/%s/write_io_count_kb",
                                SYSFS_SCST_TGT, tgt_drivers[i],
                                tgt_dir_entry->d_name,
                                sess_dir_entry->d_name);
                        readAttribute(attr_path, attr_val);
                        errno = 0;
                        write_io_kb[j] = strtoull(attr_val, NULL, 10);
                        if (errno != 0) {
                            closedir(sess_dir_stream);
                            closedir(tgt_dir_stream);
                            if (row_cnt < MAX_INFO_LABEL_ROWS) {
                                snprintf(line_buffer, SESSIONS_LABEL_COLS,
                                        "strtoull(): %s", strerror(errno));
                                SAFE_ASPRINTF(&label_msg[row_cnt], "%s",
                                        line_buffer);
                                row_cnt++;
                            }
                            return row_cnt;
                        }
                        j++;
                    }
                }
                /* Close the session directory stream */
                closedir(sess_dir_stream);
            }
        }
        /* Close the target directory stream */
        closedir(tgt_dir_stream);
    }

    /* Sort the data so the busiest session (most read IO in KB) is higher */
    num_sessions = j;
    for (i = 0; i < num_sessions; i++) {
        max_index = i;
        for (j = i; j < num_sessions; j++) {
            if (read_io_kb[max_index] < read_io_kb[j])
                max_index = j;
        }
        /* Set temporary variable */
        strncpy(tmp_init_name, init_names[i], MAX_SYSFS_ATTR_SIZE);
        tmp_lun_cnt = lun_count[i];
        tmp_act_cmds = active_cmds[i];
        tmp_read_io = read_io_kb[i];
        tmp_write_io = write_io_kb[i];
        /* Set current position variable with new maximum */
        strncpy(init_names[i], init_names[max_index], MAX_SYSFS_ATTR_SIZE);
        lun_count[i] = lun_count[max_index];
        active_cmds[i] = active_cmds[max_index];
        read_io_kb[i] = read_io_kb[max_index];
        write_io_kb[i] = write_io_kb[max_index];
        /* Move variable to old location */
        strncpy(init_names[max_index], tmp_init_name, MAX_SYSFS_ATTR_SIZE);
        lun_count[max_index] = tmp_lun_cnt;
        active_cmds[max_index] = tmp_act_cmds;
        read_io_kb[max_index] = tmp_read_io;
        write_io_kb[max_index] = tmp_write_io;
    }

    /* Finally, fill the label array with our sorted data */
    for (i = 0; i < num_sessions; i++) {
        if (row_cnt < MAX_INFO_LABEL_ROWS) {
            snprintf(line_buffer, SESSIONS_LABEL_COLS,
                    "%-25.25s %5d %5d %18llu %18llu",
                    prettyShrinkStr(25, init_names[i]), lun_count[i],
                    active_cmds[i], read_io_kb[i], write_io_kb[i]);
            SAFE_ASPRINTF(&label_msg[row_cnt], "%s", line_buffer);
            row_cnt++;
        }
    }

    /* Done */
    if (row_cnt == 1) {
        /* Add a blank line if there are no rows of data */
        SAFE_ASPRINTF(&label_msg[row_cnt], " ");
        row_cnt++;
    }
    return row_cnt;
}
