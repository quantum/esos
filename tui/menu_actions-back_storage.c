/*
 * $Id$
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

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "megaraid.h"

/*
 * Run the Adapter Properties dialog
 */
void adpPropsDialog(CDKSCREEN *main_cdk_screen) {
    MRADAPTER *mr_adapters[MAX_ADAPTERS] = {NULL};
    MRADPPROPS *mr_adp_props = NULL;
    WINDOW *adapter_window = 0;
    CDKSCREEN *adapter_screen = 0;
    CDKLABEL *adapter_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *cache_flush = 0, *rebuild_rate = 0;
    CDKRADIO *cluster_radio = 0, *ncq_radio = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int adp_count = 0, adp_choice = 0, i = 0, window_y = 0, window_x = 0,
            traverse_ret = 0, temp_int = 0, adp_window_lines = 0,
            adp_window_cols = 0;
    static char *dsbl_enbl[] = {"Disabled", "Enabled"};
    char temp_str[MAX_MR_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    char *adp_info_msg[10] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    // TODO: We assume MegaCLI works at this point if we got this far (probably bad)
    adp_count = getMRAdapterCount();

    /* Get the adapter properties */
    mr_adp_props = getMRAdapterProps(adp_choice);
    if (!mr_adp_props) {
        errorDialog(main_cdk_screen, "Error getting adapter properties.", NULL);
        return;
    }

    /* New CDK screen for selected adapter */
    adp_window_lines = 21;
    adp_window_cols = 60;
    window_y = ((LINES / 2) - (adp_window_lines / 2));
    window_x = ((COLS / 2) - (adp_window_cols / 2));
    adapter_window = newwin(adp_window_lines, adp_window_cols,
            window_y, window_x);
    if (adapter_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    adapter_screen = initCDKScreen(adapter_window);
    if (adapter_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(adapter_window, COLOR_DIALOG_BOX);
    wbkgd(adapter_window, COLOR_DIALOG_TEXT);
    wrefresh(adapter_window);

    /* Adapter info. label */
    asprintf(&adp_info_msg[0], "</31/B>Properties for MegaRAID adapter # %d...",
            adp_choice);
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&adp_info_msg[1], " ");
    asprintf(&adp_info_msg[2], "</B>Model:<!B>\t\t\t%-30s",
            mr_adapters[adp_choice]->prod_name);
    asprintf(&adp_info_msg[3], "</B>Serial Number:<!B>\t\t%-30s",
            mr_adapters[adp_choice]->serial);
    asprintf(&adp_info_msg[4], "</B>Firmware Version:<!B>\t%-30s",
            mr_adapters[adp_choice]->firmware);
    asprintf(&adp_info_msg[5], "</B>Memory:<!B>\t\t\t%-30s",
            mr_adapters[adp_choice]->memory);
    asprintf(&adp_info_msg[6], "</B>Battery:<!B>\t\t%-30s",
            mr_adapters[adp_choice]->bbu);
    asprintf(&adp_info_msg[7], "</B>Host Interface:<!B>\t\t%-30s",
            mr_adapters[adp_choice]->interface);
    asprintf(&adp_info_msg[8], "</B>Physical Disks:<!B>\t\t%-30d",
            mr_adapters[adp_choice]->disk_cnt);
    asprintf(&adp_info_msg[9], "</B>Logical Drives:<!B>\t\t%-30d",
            mr_adapters[adp_choice]->logical_drv_cnt);
    adapter_info = newCDKLabel(adapter_screen, (window_x + 1), (window_y + 1),
            adp_info_msg, 10, FALSE, FALSE);
    if (!adapter_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(adapter_info, COLOR_DIALOG_TEXT);

    /* Field entry widgets */
    cache_flush = newCDKEntry(adapter_screen, (window_x + 1), (window_y + 12),
            NULL, "</B>Cache Flush Interval (0 to 255): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
            FALSE, FALSE);
    if (!cache_flush) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(cache_flush, COLOR_DIALOG_INPUT);
    snprintf(temp_str, MAX_MR_ATTR_SIZE, "%d", mr_adp_props->cache_flush);
    setCDKEntryValue(cache_flush, temp_str);
    rebuild_rate = newCDKEntry(adapter_screen, (window_x + 1), (window_y + 13),
            NULL, "</B>Rebuild Rate (0 to 100): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
            FALSE, FALSE);
    if (!rebuild_rate) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(rebuild_rate, COLOR_DIALOG_INPUT);
    snprintf(temp_str, MAX_MR_ATTR_SIZE, "%d", mr_adp_props->rebuild_rate);
    setCDKEntryValue(rebuild_rate, temp_str);

    /* Radio lists */
    cluster_radio = newCDKRadio(adapter_screen, (window_x + 1), (window_y + 15),
            NONE, 3, 10, "</B>Cluster", dsbl_enbl, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!cluster_radio) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(cluster_radio, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(cluster_radio, (int) mr_adp_props->cluster);
    ncq_radio = newCDKRadio(adapter_screen, (window_x + 16), (window_y + 15),
            NONE, 3, 10, "</B>NCQ", dsbl_enbl, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!ncq_radio) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(ncq_radio, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(ncq_radio, (int) mr_adp_props->ncq);

    /* Buttons */
    ok_button = newCDKButton(adapter_screen, (window_x + 20), (window_y + 19),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(adapter_screen, (window_x + 30), (window_y + 19),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(adapter_screen);
    traverse_ret = traverseCDKScreen(adapter_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);
        
        /* Check entry inputs */
        temp_int = atoi(getCDKEntryValue(cache_flush));
        if (temp_int != mr_adp_props->cache_flush) {
            if (temp_int < 0 || temp_int > 255) {
                errorDialog(adapter_screen,
                    "Cache flush value must be 0 to 255.", NULL);
                goto cleanup;
            } else {
                mr_adp_props->cache_flush = temp_int;
            }
        }
        temp_int = atoi(getCDKEntryValue(rebuild_rate));
        if (temp_int != mr_adp_props->rebuild_rate) {
            if (temp_int < 0 || temp_int > 100) {
                errorDialog(adapter_screen,
                    "Rebuild rate value must be 0 to 100.", NULL);
                goto cleanup;
            } else {
                mr_adp_props->rebuild_rate = temp_int;
            }
        }

        /* Check radio inputs */
        temp_int = getCDKRadioSelectedItem(cluster_radio);
        if (temp_int != (int) mr_adp_props->cluster)
            mr_adp_props->cluster = temp_int;
        temp_int = getCDKRadioSelectedItem(ncq_radio);
        if (temp_int != (int) mr_adp_props->ncq)
            mr_adp_props->ncq = temp_int;

        /* Set adapter properties */
        temp_int = setMRAdapterProps(mr_adp_props);
        if (temp_int != 0) {
            asprintf(&error_msg, "Couldn't update adapter properties; MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* All done -- clean */
    cleanup:
    free(mr_adp_props);
    for (i = 0; i < 10; i++) {
        freeChar(adp_info_msg[i]);
    }
    for (i = 0; i < MAX_ADAPTERS; i++) {
        free(mr_adapters[i]);
    }
    if (adapter_screen != NULL) {
        destroyCDKScreenObjects(adapter_screen);
        destroyCDKScreen(adapter_screen);
    }
    delwin(adapter_window);
    return;
}


/*
 * Run the Adapter Information dialog
 */
void adpInfoDialog(CDKSCREEN *main_cdk_screen) {
    MRENCL *mr_enclosures[MAX_ENCLOSURES] = {NULL};
    MRADAPTER *mr_adapters[MAX_ADAPTERS] = {NULL};
    MRDISK *mr_disks[MAX_ENCLOSURES][MAX_DISKS] = {{NULL}, {NULL}};
    CDKSWINDOW *encl_swindow = 0;
    int adp_count = 0, adp_choice = 0, i = 0, encl_count = 0, j = 0, line_pos = 0;
    char *encl_title = NULL;
    char *error_msg = NULL;
    char *swindow_info[MAX_ADP_INFO_LINES] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    // TODO: We assume MegaCLI works at this point if we got this far (probably bad)
    adp_count = getMRAdapterCount();

    /* Get enclosures / disks (slots) */
    encl_count = getMREnclCount(adp_choice);
    if (encl_count == -1) {
        errorDialog(main_cdk_screen, "Error getting enclosure count.", NULL);
        return;
    } else if (encl_count == 0) {
        errorDialog(main_cdk_screen, "No enclosures found!", NULL);
        return;
    } else {
        for (i = 0; i < encl_count; i++) {
            mr_enclosures[i] = getMREnclosure(adp_choice, i);
            if (!mr_enclosures[i]) {
                asprintf(&error_msg,
                        "Couldn't get data from MegaRAID enclosure # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            for (j = 0; j < mr_enclosures[i]->slots; j++) {
                mr_disks[i][j] = getMRDisk(adp_choice, mr_enclosures[i]->device_id, j);
                if (!mr_disks[i][j]) {
                    asprintf(&error_msg, "Couldn't get disk information for slot %d, enclosure %d!", j, i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    goto cleanup;
                }
            }
        }
    }

    /* Setup scrolling window widget */
    asprintf(&encl_title, "<C></31/B>Enclosures/Slots on MegaRAID Adapter # %d\n",
            adp_choice);
    encl_swindow = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            ADP_INFO_ROWS+2, ADP_INFO_COLS+2, encl_title, MAX_ADP_INFO_LINES, TRUE, FALSE);
    if (!encl_swindow) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(encl_swindow, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(encl_swindow, COLOR_DIALOG_BOX);

    /* Add enclosure/disk information */
    line_pos = 0;
    for (i = 0; i < encl_count; i++) {
        if (line_pos < MAX_ADP_INFO_LINES) {
            asprintf(&swindow_info[line_pos],
                    "</B>Enclosure %d (%s %s)",
                    i, mr_enclosures[i]->vendor, mr_enclosures[i]->product);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
        if (line_pos < MAX_ADP_INFO_LINES) {
            asprintf(&swindow_info[line_pos],
                    "</B>Device ID: %d, Status: %s, Slots: %d",
                    mr_enclosures[i]->device_id, mr_enclosures[i]->status,
                    mr_enclosures[i]->slots);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
        for (j = 0; (j < mr_enclosures[i]->slots) && (line_pos < MAX_ADP_INFO_LINES); j++) {
            if (mr_disks[i][j]->present)
                asprintf(&swindow_info[line_pos], "\tSlot %3d: %s", j, mr_disks[i][j]->inquiry);
            else
                asprintf(&swindow_info[line_pos], "\tSlot %3d: Not Present", j);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_ADP_INFO_LINES) {
        asprintf(&swindow_info[line_pos], " ");
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        line_pos++;
    }
    if (line_pos < MAX_ADP_INFO_LINES) {
        asprintf(&swindow_info[line_pos], CONTINUE_MSG);
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        line_pos++;
    }

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(encl_swindow, 'g');
    activateCDKSwindow(encl_swindow, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(encl_swindow);

    /* Done */
    cleanup:
    freeChar(encl_title);
    for (i = 0; i < MAX_ADP_INFO_LINES; i++ ) {
        freeChar(swindow_info[i]);
    }
    for (i = 0; i < MAX_ADAPTERS; i++) {
        free(mr_adapters[i]);
    }
    for (i = 0; i < MAX_ENCLOSURES; i++) {
        free(mr_enclosures[i]);
        for (j = 0; j < MAX_DISKS; j++) {
            free(mr_disks[i][j]);
        }
    }
    return;
}


/*
 * Run the Add Volume dialog
 */
void addVolumeDialog(CDKSCREEN *main_cdk_screen) {
    MRADAPTER *mr_adapters[MAX_ADAPTERS] = {NULL};
    MRENCL *mr_enclosures[MAX_ENCLOSURES] = {NULL};
    MRDISK *mr_disks[MAX_ENCLOSURES][MAX_DISKS] = {{NULL}, {NULL}},
            *disk_selection[MAX_DISKS] = {NULL},
            *chosen_disks[MAX_DISKS] = {NULL};
    MRLDPROPS *new_ld_props = NULL;
    WINDOW *new_ld_window = 0;
    CDKSELECTION *disk_select = 0;
    CDKSCREEN *new_ld_screen = 0;
    CDKLABEL *new_ld_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKRADIO *cache_policy = 0, *write_cache = 0, *read_cache = 0,
            *bbu_cache = 0, *raid_lvl = 0, *strip_size = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int adp_choice = 0, adp_count = 0, i = 0, j = 0, encl_count = 0,
            selection_size = 0, chosen_disk_cnt = 0, traverse_ret = 0,
            temp_int = 0, window_y = 0, window_x = 0, new_ld_window_lines = 0,
            new_ld_window_cols = 0, pd_info_size = 0, pd_info_line_size = 0;
    static char *choice_char[] = {"[ ] ", "[*] "};
    static char *cache_opts[] = {"Cached", "Direct"};
    static char *write_opts[] = {"WT", "WB"};
    static char *read_opts[] = {"NORA", "RA", "ADRA"};
    static char *bbu_opts[] = {"CachedBadBBU", "NoCachedBadBBU"};
    static char *raid_opts[] = {"0", "1", "5", "6"};
    static char *strip_opts[] = {"8", "16", "32", "64", "128", "256", "512", "1024"};
    char new_ld_raid_lvl[MAX_MR_ATTR_SIZE] = {0},
            new_ld_strip_size[MAX_MR_ATTR_SIZE] = {0},
            pd_info_line_buffer[MAX_PD_INFO_LINE_BUFF] = {0};
    char *error_msg = NULL, *dsk_select_title = NULL, *temp_pstr = NULL;
    char *selection_list[MAX_DISKS] = {NULL}, *new_ld_msg[NEW_LD_INFO_LINES] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    // TODO: We assume MegaCLI works at this point if we got this far (probably bad)
    adp_count = getMRAdapterCount();

    /* Get enclosures / disks (slots) and fill selection list */
    encl_count = getMREnclCount(adp_choice);
    if (encl_count == -1) {
        errorDialog(main_cdk_screen, "Error getting enclosure count.", NULL);
        goto cleanup;
    } else if (encl_count == 0) {
        errorDialog(main_cdk_screen, "No enclosures found!", NULL);
        goto cleanup;
    } else {
        selection_size = 0;
        for (i = 0; i < encl_count; i++) {
            mr_enclosures[i] = getMREnclosure(adp_choice, i);
            if (!mr_enclosures[i]) {
                asprintf(&error_msg,
                        "Couldn't get data from MegaRAID enclosure # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            for (j = 0; j < mr_enclosures[i]->slots; j++) {
                mr_disks[i][j] = getMRDisk(adp_choice,
                        mr_enclosures[i]->device_id, j);
                if (!mr_disks[i][j]) {
                    asprintf(&error_msg, "Couldn't get disk information for slot %d, enclosure %d!", j, i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    goto cleanup;
                } else {
                    if (mr_disks[i][j]->present == TRUE &&
                            mr_disks[i][j]->part_of_ld == FALSE) {
                            /* Fill selection list */
                            asprintf(&selection_list[selection_size],
                                    "<C>Enclosure %3.3d, Slot %3.3d: %40.40s", i, j,
                                    mr_disks[i][j]->inquiry);
                            disk_selection[selection_size] = mr_disks[i][j];
                            selection_size++;
                    }
                }
            }
        }
    }

    /* If we don't have any available disks, then display a message saying so
     * and return */
    if (selection_size == 0) {
        asprintf(&error_msg, "There are no available physical disks on adapter # %d.", adp_choice);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    
    /* Selection widget for disks */
    asprintf(&dsk_select_title,
            "<C></31/B>Select Disks for New LD (MegaRAID Adapter # %d)\n",
            adp_choice);
    disk_select = newCDKSelection(main_cdk_screen, CENTER, CENTER, NONE,
            18, 74, dsk_select_title, selection_list, selection_size,
            choice_char, 2, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!disk_select) {
        errorDialog(main_cdk_screen, "Couldn't create selection widget!", NULL);
        goto cleanup;
    }
    setCDKSelectionBoxAttribute(disk_select, COLOR_DIALOG_BOX);
    setCDKSelectionBackgroundAttrib(disk_select, COLOR_DIALOG_TEXT);

    /* Activate the widget */
    activateCDKSelection(disk_select, 0);

    /* User hit escape, so we get out of this */
    if (disk_select->exitType == vESCAPE_HIT) {
        destroyCDKSelection(disk_select);
        refreshCDKScreen(main_cdk_screen);
        goto cleanup;

    /* User hit return/tab so we can continue on and get what was selected */
    } else if (disk_select->exitType == vNORMAL) {
        chosen_disk_cnt = 0;
        for (i = 0; i < selection_size; i++) {
            if (disk_select->selections[i] == 1) {
                chosen_disks[chosen_disk_cnt] = disk_selection[i];
                chosen_disk_cnt++;
            }
        }
        destroyCDKSelection(disk_select);
        refreshCDKScreen(main_cdk_screen);
    }

    /* Check and make sure some disks were actually selected */
    if (chosen_disk_cnt == 0) {
        errorDialog(main_cdk_screen, "No physical disks selected!", NULL);
        goto cleanup;
    }

    /* Setup a new CDK screen for required input (to create new LD) */
    new_ld_window_lines = 18;
    new_ld_window_cols = 70;
    window_y = ((LINES / 2) - (new_ld_window_lines / 2));
    window_x = ((COLS / 2) - (new_ld_window_cols / 2));
    new_ld_window = newwin(new_ld_window_lines, new_ld_window_cols,
            window_y, window_x);
    if (new_ld_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    new_ld_screen = initCDKScreen(new_ld_window);
    if (new_ld_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(new_ld_window, COLOR_DIALOG_BOX);
    wbkgd(new_ld_window, COLOR_DIALOG_TEXT);
    wrefresh(new_ld_window);

    /* Put selected physical disk "enclosure:slot" information into a string */
    for (i = 0; i < chosen_disk_cnt; i++) {
        if (i == (chosen_disk_cnt - 1))
            asprintf(&temp_pstr, "[%d:%d]", chosen_disks[i]->enclosure_id,
                    chosen_disks[i]->slot_num);
        else
            asprintf(&temp_pstr, "[%d:%d], ", chosen_disks[i]->enclosure_id,
                chosen_disks[i]->slot_num);
        /* We add one extra for the null byte */
        pd_info_size = strlen(temp_pstr) + 1;
        pd_info_line_size = pd_info_line_size + pd_info_size;
        // TODO: This totes needs to be tested (strcat)
        if (pd_info_line_size >= MAX_PD_INFO_LINE_BUFF) {
            errorDialog(main_cdk_screen, "The maximum PD info. line buffer size has been reached!", NULL);
            freeChar(temp_pstr);
            goto cleanup;
        } else {
            strcat(pd_info_line_buffer, temp_pstr);
            freeChar(temp_pstr);
        }
    }

    /* Make a new label for the add-logical-drive screen */
    asprintf(&new_ld_msg[0],
            "</31/B>Creating new MegaRAID LD (on adapter # %d)...",
            adp_choice);
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&new_ld_msg[1], " ");
    asprintf(&new_ld_msg[2], "Selected Disks [ENCL:SLOT] - %.35s", pd_info_line_buffer);
    new_ld_label = newCDKLabel(new_ld_screen, (window_x + 1), (window_y + 1),
            new_ld_msg, NEW_LD_INFO_LINES, FALSE, FALSE);
    if (!new_ld_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(new_ld_label, COLOR_DIALOG_TEXT);

    /* RAID level radio list */
    raid_lvl = newCDKRadio(new_ld_screen, (window_x + 1), (window_y + 5),
            NONE, 5, 10, "</B>RAID Level", raid_opts, 4,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!raid_lvl) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(raid_lvl, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(raid_lvl, 0);

    /* Strip size radio list */
    strip_size = newCDKRadio(new_ld_screen, (window_x + 15), (window_y + 5),
            NONE, 9, 10, "</B>Strip Size", strip_opts, 8,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!strip_size) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(strip_size, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(strip_size, 3);

    /* Write cache radio list */
    write_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 5),
            NONE, 3, 11, "</B>Write Cache", write_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!write_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(write_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(write_cache, 1);

    /* Read cache radio list */
    read_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 9),
            NONE, 4, 10, "</B>Read Cache", read_opts, 3,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!read_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(read_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(read_cache, 2);

    /* Cache policy radio list */
    cache_policy = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 5),
            NONE, 3, 12, "</B>Cache Policy", cache_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!cache_policy) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(cache_policy, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(cache_policy, 1);

    /* BBU cache policy radio list */
    bbu_cache = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 9),
            NONE, 3, 16, "</B>BBU Cache Policy", bbu_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!bbu_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(bbu_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(bbu_cache, 1);

    /* Buttons */
    ok_button = newCDKButton(new_ld_screen, (window_x + 26), (window_y + 16),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(new_ld_screen, (window_x + 36), (window_y + 16),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(new_ld_screen);
    traverse_ret = traverseCDKScreen(new_ld_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Set new LD properties */
        new_ld_props = (MRLDPROPS *) calloc(1, sizeof(MRLDPROPS));
        new_ld_props->adapter_id = adp_choice;
        temp_int = getCDKRadioSelectedItem(cache_policy);
        strncpy(new_ld_props->cache_policy, cache_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(write_cache);
        strncpy(new_ld_props->write_policy, write_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(read_cache);
        strncpy(new_ld_props->read_policy, read_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(bbu_cache);
        strncpy(new_ld_props->bbu_cache_policy, bbu_opts[temp_int], MAX_MR_ATTR_SIZE);

        /* RAID level and stripe size */
        // TODO: Should we check for a valid RAID level + disk combination?
        temp_int = getCDKRadioSelectedItem(raid_lvl);
        strncpy(new_ld_raid_lvl, raid_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(strip_size);
        strncpy(new_ld_strip_size, strip_opts[temp_int], MAX_MR_ATTR_SIZE);

        /* Create the new logical drive */
        temp_int = addMRLogicalDrive(new_ld_props, chosen_disk_cnt,
                chosen_disks, new_ld_raid_lvl, new_ld_strip_size);
        if (temp_int != 0) {
            asprintf(&error_msg, "Error creating new logical drive; MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    }

    /* Done -- free everything and clean up */
    cleanup:
    free(new_ld_props);
    freeChar(dsk_select_title);
    for (i = 0; i < MAX_DISKS; i++) {
        freeChar(selection_list[i]);
    }
    for (i = 0; i < NEW_LD_INFO_LINES; i++) {
        freeChar(new_ld_msg[i]);
    }
    for (i = 0; i < MAX_ADAPTERS; i++) {
        free(mr_adapters[i]);
    }
    for (i = 0; i < MAX_ENCLOSURES; i++) {
        free(mr_enclosures[i]);
        for (j = 0; j < MAX_DISKS; j++) {
            /* disk_selection and chosen_disks are not free'd
             * since mr_disks holds all possible and that one is free'd */
            free(mr_disks[i][j]);
        }
    }
    if (new_ld_screen != NULL) {
        destroyCDKScreenObjects(new_ld_screen);
        destroyCDKScreen(new_ld_screen);
    }
    delwin(new_ld_window);
    return;
}


/*
 * Run the Delete Volume dialog
 */
void delVolumeDialog(CDKSCREEN *main_cdk_screen) {
    MRADAPTER *mr_adapters[MAX_ADAPTERS] = {NULL};
    MRLDRIVE *mr_ldrives[MAX_MR_LDS] = {NULL};
    CDKSCROLL *ld_list = 0;
    static char *ld_list_title = "<C></31/B>Choose a Logical Drive\n";
    char *logical_drives[MAX_MR_LDS] = {NULL};
    char *error_msg = NULL, *confirm_msg = NULL;
    int mr_ld_ids[MAX_MR_LDS] = {0};
    int adp_count = 0, adp_choice = 0, i = 0, ld_count = 0, ld_choice = 0, temp_int = 0;
    boolean confirm = FALSE;

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    // TODO: We assume MegaCLI works at this point if we got this far (probably bad)
    adp_count = getMRAdapterCount();

    /* Get MegaRAID logical drives */
    ld_count = getMRLDCount(adp_choice);
    if (ld_count == 0) {
        errorDialog(main_cdk_screen, "No logical drives found!", NULL);
        goto cleanup;
    } else if (ld_count == -1) {
        asprintf(&error_msg, "Error getting LD count for adapter # %d!", adp_choice);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Get MegaRAID LD ID numbers */
    if (getMRLDIDNums(adp_choice, ld_count, mr_ld_ids) != 0) {
        errorDialog(main_cdk_screen, "Couldn't get logical drive ID numbers!", NULL);
        goto cleanup;
    } else {
        for (i = 0; i < ld_count; i++) {
            mr_ldrives[i] = getMRLogicalDrive(adp_choice, mr_ld_ids[i]);
            if (!mr_ldrives[i]) {
                asprintf(&error_msg,
                        "Couldn't get data from MegaRAID logical drive # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            } else {
                asprintf(&logical_drives[i],
                        "<C>MegaRAID Virtual Drive # %d (on adapter %d)",
                        mr_ldrives[i]->ldrive_id, adp_choice);
            }
        }
    }

    /* Get logical drive choice from user */
    ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 12, 50,
            ld_list_title, logical_drives, ld_count,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!ld_list) {
        errorDialog(main_cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(ld_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(ld_list, COLOR_DIALOG_TEXT);
    ld_choice = activateCDKScroll(ld_list, 0);

    if (ld_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
        goto cleanup;

    } else if (ld_list->exitType == vNORMAL) {
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
    }

    /* Get a final confirmation from user before we delete */
    asprintf(&confirm_msg, "logical drive # %d on adapter %d?",
            mr_ldrives[ld_choice]->ldrive_id, adp_choice);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you want to delete", confirm_msg);
    freeChar(confirm_msg);
    if (confirm) {
        temp_int = delMRLogicalDrive(adp_choice, mr_ldrives[ld_choice]->ldrive_id);
        if (temp_int != 0) {
            asprintf(&error_msg, "Error deleting logical drive; MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    cleanup:
    for (i = 0; i < MAX_ADAPTERS; i++) {
        free(mr_adapters[i]);
    }
    for (i = 0; i < MAX_MR_LDS; i++) {
        free(mr_ldrives[i]);
        freeChar(logical_drives[i]);
    }
    return;
}


/*
 * Run the Volume Properties dialog
 */
void volPropsDialog(CDKSCREEN *main_cdk_screen) {
    MRADAPTER *mr_adapters[MAX_ADAPTERS] = {NULL};
    MRLDRIVE *mr_ldrives[MAX_MR_LDS] = {NULL};
    MRLDPROPS *mr_ld_props = NULL;
    WINDOW *ld_window = 0;
    CDKSCROLL *ld_list = 0;
    CDKSCREEN *ld_screen = 0;
    CDKLABEL *ld_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *name_field = 0;
    CDKRADIO *cache_policy = 0, *write_cache = 0, *read_cache = 0,
            *bbu_cache = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int ld_window_lines = 0, ld_window_cols = 0, window_y = 0, window_x = 0,
            traverse_ret = 0, temp_int = 0, adp_count = 0, adp_choice = 0,
            i = 0, ld_count = 0, ld_choice = 0, pd_info_size = 0,
            pd_info_line_size = 0;
    int ld_encl_ids[MAX_MR_DISKS] = {0}, ld_slots[MAX_MR_DISKS] = {0},
            mr_ld_ids[MAX_MR_LDS] = {0};
    static char *ld_list_title = "<C></31/B>Choose a Logical Drive\n";
    static char *cache_opts[] = {"Cached", "Direct"};
    static char *write_opts[] = {"WT", "WB"};
    static char *read_opts[] = {"NORA", "RA", "ADRA"};
    static char *bbu_opts[] = {"CachedBadBBU", "NoCachedBadBBU"};
    char ld_name_buff[MAX_MR_ATTR_SIZE] = {0},
            pd_info_line_buffer[MAX_PD_INFO_LINE_BUFF] = {0};
    char *temp_pstr = NULL, *error_msg = NULL;
    char *logical_drives[MAX_MR_LDS] = {NULL},
            *ld_info_msg[LD_PROPS_INFO_LINES] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    // TODO: We assume MegaCLI works at this point if we got this far (probably bad)
    adp_count = getMRAdapterCount();

    /* Get MegaRAID logical drives */
    ld_count = getMRLDCount(adp_choice);
    if (ld_count == 0) {
        errorDialog(main_cdk_screen, "No logical drives found!", NULL);
        goto cleanup;
    } else if (ld_count == -1) {
        asprintf(&error_msg, "Error getting LD count for adapter # %d!", adp_choice);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Get MegaRAID LD ID numbers */
    if (getMRLDIDNums(adp_choice, ld_count, mr_ld_ids) != 0) {
        errorDialog(main_cdk_screen, "Couldn't get logical drive ID numbers!", NULL);
        goto cleanup;
    } else {
        for (i = 0; i < ld_count; i++) {
            mr_ldrives[i] = getMRLogicalDrive(adp_choice, mr_ld_ids[i]);
            if (!mr_ldrives[i]) {
                asprintf(&error_msg,
                        "Couldn't get data from MegaRAID logical drive # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            } else {
                asprintf(&logical_drives[i],
                        "<C>MegaRAID Virtual Drive # %d (on adapter %d)",
                        mr_ldrives[i]->ldrive_id, adp_choice);
            }
        }
    }

    /* Get logical drive choice from user */
    ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 12, 50,
            ld_list_title, logical_drives, ld_count,
            FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!ld_list) {
        errorDialog(main_cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(ld_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(ld_list, COLOR_DIALOG_TEXT);
    ld_choice = activateCDKScroll(ld_list, 0);

    if (ld_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
        goto cleanup;

    } else if (ld_list->exitType == vNORMAL) {
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
    }

    /* Get the logical drive properties */
    mr_ld_props = getMRLDProps(adp_choice, mr_ldrives[ld_choice]->ldrive_id);
    if (!mr_ld_props) {
        errorDialog(main_cdk_screen, "Error getting logical drive properties.", NULL);
        goto cleanup;
    }

    /* New CDK screen for selected LD */
    ld_window_lines = 20;
    ld_window_cols = 70;
    window_y = ((LINES / 2) - (ld_window_lines / 2));
    window_x = ((COLS / 2) - (ld_window_cols / 2));
    ld_window = newwin(ld_window_lines, ld_window_cols, window_y, window_x);
    if (ld_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    ld_screen = initCDKScreen(ld_window);
    if (ld_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(ld_window, COLOR_DIALOG_BOX);
    wbkgd(ld_window, COLOR_DIALOG_TEXT);
    wrefresh(ld_window);

    /* Get enclosure/slot (disk) information for selected LD */
    getMRLDDisks(adp_choice, mr_ldrives[ld_choice]->ldrive_id, ld_encl_ids, ld_slots);
    for (i = 0; i < mr_ldrives[ld_choice]->drive_cnt; i++) {
        if (i == (mr_ldrives[ld_choice]->drive_cnt - 1))
            asprintf(&temp_pstr, "[%d:%d]", ld_encl_ids[i], ld_slots[i]);
        else
            asprintf(&temp_pstr, "[%d:%d], ", ld_encl_ids[i], ld_slots[i]);
        /* We add one extra for the null byte */
        pd_info_size = strlen(temp_pstr) + 1;
        pd_info_line_size = pd_info_line_size + pd_info_size;
        // TODO: This totes needs to be tested (strcat)
        if (pd_info_line_size >= MAX_PD_INFO_LINE_BUFF) {
            errorDialog(main_cdk_screen, "The maximum PD info. line buffer size has been reached!", NULL);
            freeChar(temp_pstr);
            goto cleanup;
        } else {
            strcat(pd_info_line_buffer, temp_pstr);
            freeChar(temp_pstr);
        }
    }

    /* Logical drive info. label */
    asprintf(&ld_info_msg[0], "</31/B>Properties for MegaRAID LD # %d (on adapter # %d)...",
            mr_ldrives[ld_choice]->ldrive_id, adp_choice);
    asprintf(&ld_info_msg[1], " ");
    asprintf(&ld_info_msg[2], "</B>RAID Level:<!B>\t%s",
            mr_ldrives[ld_choice]->raid_lvl);
    asprintf(&ld_info_msg[3], "</B>Size:<!B>\t\t%s",
            mr_ldrives[ld_choice]->size);
    asprintf(&ld_info_msg[4], "</B>State:<!B>\t\t%s",
            mr_ldrives[ld_choice]->state);
    asprintf(&ld_info_msg[5], "</B>Strip Size:<!B>\t%s",
            mr_ldrives[ld_choice]->strip_size);
    asprintf(&ld_info_msg[6], "</B>Drive Count:<!B>\t%d",
            mr_ldrives[ld_choice]->drive_cnt);
    asprintf(&ld_info_msg[7], " ");
    asprintf(&ld_info_msg[8], "Disks [ENCL:SLOT] - %.60s", pd_info_line_buffer);
    ld_info = newCDKLabel(ld_screen, (window_x + 1), (window_y + 1),
            ld_info_msg, LD_PROPS_INFO_LINES, FALSE, FALSE);
    if (!ld_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(ld_info, COLOR_DIALOG_TEXT);

    /* Name field */
    name_field = newCDKEntry(ld_screen, (window_x + 1), (window_y + 11),
            NULL, "</B>Logical drive name (no spaces): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vUMIXED,
            MAX_LD_NAME, 0, MAX_LD_NAME, FALSE, FALSE);
    if (!name_field) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(name_field, COLOR_DIALOG_INPUT);
    setCDKEntryValue(name_field, mr_ld_props->name);

    /* Radio lists */
    cache_policy = newCDKRadio(ld_screen, (window_x + 1), (window_y + 13),
            NONE, 3, 10, "</B>Cache Policy", cache_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!cache_policy) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(cache_policy, COLOR_DIALOG_TEXT);
    if (strcmp(mr_ld_props->cache_policy, cache_opts[0]) == 0)
        setCDKRadioCurrentItem(cache_policy, 0);
    else if (strcmp(mr_ld_props->cache_policy, cache_opts[1]) == 0)
        setCDKRadioCurrentItem(cache_policy, 1);

    write_cache = newCDKRadio(ld_screen, (window_x + 16), (window_y + 13),
            NONE, 3, 10, "</B>Write Cache", write_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!write_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(write_cache, COLOR_DIALOG_TEXT);
    if (strcmp(mr_ld_props->write_policy, write_opts[0]) == 0)
        setCDKRadioCurrentItem(write_cache, 0);
    else if (strcmp(mr_ld_props->write_policy, write_opts[1]) == 0)
        setCDKRadioCurrentItem(write_cache, 1);

    read_cache = newCDKRadio(ld_screen, (window_x + 30), (window_y + 13),
            NONE, 4, 10, "</B>Read Cache", read_opts, 3,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!read_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(read_cache, COLOR_DIALOG_TEXT);
    if (strcmp(mr_ld_props->read_policy, read_opts[0]) == 0)
        setCDKRadioCurrentItem(read_cache, 0);
    else if (strcmp(mr_ld_props->read_policy, read_opts[1]) == 0)
        setCDKRadioCurrentItem(read_cache, 1);
    else if (strcmp(mr_ld_props->read_policy, read_opts[2]) == 0)
        setCDKRadioCurrentItem(read_cache, 2);

    bbu_cache = newCDKRadio(ld_screen, (window_x + 45), (window_y + 13),
            NONE, 3, 12, "</B>BBU Cache Policy", bbu_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!bbu_cache) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(bbu_cache, COLOR_DIALOG_TEXT);
    if (strcmp(mr_ld_props->bbu_cache_policy, bbu_opts[0]) == 0)
        setCDKRadioCurrentItem(bbu_cache, 0);
    else if (strcmp(mr_ld_props->bbu_cache_policy, bbu_opts[1]) == 0)
        setCDKRadioCurrentItem(bbu_cache, 1);

    /* Buttons */
    ok_button = newCDKButton(ld_screen, (window_x + 26), (window_y + 18),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(ld_screen, (window_x + 36), (window_y + 18),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(ld_screen);
    traverse_ret = traverseCDKScreen(ld_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check name (field entry) */
        strncpy(ld_name_buff, getCDKEntryValue(name_field), MAX_MR_ATTR_SIZE);
        i = 0;
        while (ld_name_buff[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_NAME_CHAR(ld_name_buff[i])) {
                errorDialog(main_cdk_screen,
                        "The name entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }
        strncpy(mr_ld_props->name, getCDKEntryValue(name_field), MAX_MR_ATTR_SIZE);

        /* Set radio inputs */
        temp_int = getCDKRadioSelectedItem(cache_policy);
        strncpy(mr_ld_props->cache_policy, cache_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(write_cache);
        strncpy(mr_ld_props->write_policy, write_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(read_cache);
        strncpy(mr_ld_props->read_policy, read_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(bbu_cache);
        strncpy(mr_ld_props->bbu_cache_policy, bbu_opts[temp_int], MAX_MR_ATTR_SIZE);

        /* Set logical drive properties */
        temp_int = setMRLDProps(mr_ld_props);
        if (temp_int != 0) {
            asprintf(&error_msg, "Couldn't update logical drive properties; MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done -- free everything and clean up */
    cleanup:
    free(mr_ld_props);
    for (i = 0; i < MAX_ADAPTERS; i++) {
        free(mr_adapters[i]);
    }
    for (i = 0; i < MAX_MR_LDS; i++) {
        free(mr_ldrives[i]);
        freeChar(logical_drives[i]);
    }
    for (i = 0; i < LD_PROPS_INFO_LINES; i++) {
        freeChar(ld_info_msg[i]);
    }
    if (ld_screen != NULL) {
        destroyCDKScreenObjects(ld_screen);
        destroyCDKScreen(ld_screen);
    }
    delwin(ld_window);
    return;
}


/*
 * Run the DRBD Status dialog
 */
void drbdStatDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *drbd_info = 0;
    char *swindow_info[MAX_DRBD_INFO_LINES] = {NULL};
    char *error_msg = NULL;
    int i = 0, line_pos = 0;
    char line[DRBD_INFO_COLS] = {0};
    FILE *drbd_file = NULL;

    /* Open the file */
    if ((drbd_file = fopen(PROC_DRBD, "r")) == NULL) {
        asprintf(&error_msg, "fopen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    } else {
        /* Setup scrolling window widget */
        drbd_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                DRBD_INFO_ROWS+2, DRBD_INFO_COLS+2,
                "<C></31/B>Distributed Replicated Block Device (DRBD) Information\n",
                MAX_DRBD_INFO_LINES, TRUE, FALSE);
        if (!drbd_info) {
            errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
            return;
        }
        setCDKSwindowBackgroundAttrib(drbd_info, COLOR_DIALOG_TEXT);
        setCDKSwindowBoxAttribute(drbd_info, COLOR_DIALOG_BOX);

        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), drbd_file) != NULL) {
            if (line_pos < MAX_DRBD_INFO_LINES) {
                asprintf(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }
        fclose(drbd_file);

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_DRBD_INFO_LINES) {
            asprintf(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_DRBD_INFO_LINES) {
            asprintf(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Set the scrolling window content */
        setCDKSwindowContents(drbd_info, swindow_info, line_pos);

        /* The 'g' makes the swindow widget scroll to the top, then activate */
        injectCDKSwindow(drbd_info, 'g');
        activateCDKSwindow(drbd_info, 0);

        /* We fell through -- the user exited the widget, but we don't care how */
        destroyCDKSwindow(drbd_info);
    }

    /* Done */
    for (i = 0; i < MAX_DRBD_INFO_LINES; i++ )
        freeChar(swindow_info[i]);
    return;
}


/*
 * Run the Software RAID Status dialog
 */
void softRAIDStatDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *mdstat_info = 0;
    char *swindow_info[MAX_MDSTAT_INFO_LINES] = {NULL};
    char *error_msg = NULL;
    int i = 0, line_pos = 0;
    char line[MDSTAT_INFO_COLS] = {0};
    FILE *mdstat_file = NULL;

    /* Open the file */
    if ((mdstat_file = fopen(PROC_MDSTAT, "r")) == NULL) {
        asprintf(&error_msg, "fopen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    } else {
        /* Setup scrolling window widget */
        mdstat_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                MDSTAT_INFO_ROWS+2, MDSTAT_INFO_COLS+2,
                "<C></31/B>Linux Software RAID (md) Status\n",
                MAX_MDSTAT_INFO_LINES, TRUE, FALSE);
        if (!mdstat_info) {
            errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
            return;
        }
        setCDKSwindowBackgroundAttrib(mdstat_info, COLOR_DIALOG_TEXT);
        setCDKSwindowBoxAttribute(mdstat_info, COLOR_DIALOG_BOX);

        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), mdstat_file) != NULL) {
            if (line_pos < MAX_MDSTAT_INFO_LINES) {
                asprintf(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }
        fclose(mdstat_file);

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_MDSTAT_INFO_LINES) {
            asprintf(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_MDSTAT_INFO_LINES) {
            asprintf(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }

        /* Set the scrolling window content */
        setCDKSwindowContents(mdstat_info, swindow_info, line_pos);

        /* The 'g' makes the swindow widget scroll to the top, then activate */
        injectCDKSwindow(mdstat_info, 'g');
        activateCDKSwindow(mdstat_info, 0);

        /* We fell through -- the user exited the widget, but we don't care how */
        destroyCDKSwindow(mdstat_info);
    }

    /* Done */
    for (i = 0; i < MAX_MDSTAT_INFO_LINES; i++ )
        freeChar(swindow_info[i]);
    return;
}


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
    asprintf(&lvdisplay_cmd, "%s --all 2>&1", LVDISPLAY_BIN);
    if ((lvdisplay_proc = popen(lvdisplay_cmd, "r")) == NULL) {
        asprintf(&error_msg, "Couldn't open process for the %s command!", LVDISPLAY_BIN);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    } else {
        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), lvdisplay_proc) != NULL) {
            if (line_pos < MAX_LVM2_INFO_LINES) {
                asprintf(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_LVM2_INFO_LINES) {
            asprintf(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_LVM2_INFO_LINES) {
            asprintf(&swindow_info[line_pos], CONTINUE_MSG);
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
                errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(lvm2_info, COLOR_DIALOG_TEXT);
            setCDKSwindowBoxAttribute(lvm2_info, COLOR_DIALOG_BOX);

            /* Set the scrolling window content */
            setCDKSwindowContents(lvm2_info, swindow_info, line_pos);

            /* The 'g' makes the swindow widget scroll to the top, then activate */
            injectCDKSwindow(lvm2_info, 'g');
            activateCDKSwindow(lvm2_info, 0);

            /* We fell through -- the user exited the widget, but we don't care how */
            destroyCDKSwindow(lvm2_info);
        } else {
            asprintf(&error_msg, "The %s command exited with %d.", LVDISPLAY_BIN, ret_val);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    for (i = 0; i < MAX_LVM2_INFO_LINES; i++ )
        freeChar(swindow_info[i]);
    return;
}


/*
 * Run the Create File System dialog
 */
void createFSDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *fs_window = 0;
    CDKSCREEN *fs_screen = 0;
    CDKLABEL *add_fs_info = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *fs_label = 0;
    CDKRADIO *fs_type = 0;
    CDKSWINDOW *make_fs_info = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char fs_label_buff[MAX_FS_LABEL] = {0}, mkfs_cmd[MAX_SHELL_CMD_LEN] = {0},
            mount_cmd[MAX_SHELL_CMD_LEN] = {0},
            new_mnt_point[MAX_FS_ATTR_LEN] = {0},
            new_blk_dev_node[MAX_FS_ATTR_LEN] = {0},
            real_blk_dev_node[MAX_SYSFS_PATH_SIZE] = {0};
    char *block_dev = NULL, *error_msg = NULL, *confirm_msg = NULL,
            *dev_node = NULL, *device_size = NULL, *tmp_str_ptr = NULL;
    char *fs_dialog_msg[MAX_FS_DIALOG_INFO_LINES] = {NULL},
            *swindow_info[MAX_MAKE_FS_INFO_LINES] = {NULL};
    static char *transport[] = {"unknown", "scsi", "ide", "dac960", "cpqarray",
    "file", "ataraid", "i2o", "ubd", "dasd", "viodasd", "sx8", "dm"};
    static char *fs_type_opts[] = {"xfs", "ext2", "ext3", "ext4"};
    FILE *fstab_file = NULL, *new_fstab_file = NULL;
    struct mntent *fstab_entry = NULL,
            addtl_fstab_entry; /* Not a pointer */
    int temp_int = 0, window_y = 0, window_x = 0, info_line_cnt = 0,
            traverse_ret = 0, i = 0, exit_stat = 0, ret_val = 0,
            fs_window_lines = 0, fs_window_cols = 0;
    PedDevice *device = NULL;
    PedDiskType *disk_type = NULL;
    PedDisk *disk = NULL;
    PedPartition *partition = NULL;
    PedFileSystemType *file_system_type = NULL;
    PedConstraint *constraint = NULL;
    boolean confirm = FALSE, question = FALSE;

    /* Get block device choice from user */
    if ((block_dev = getBlockDevChoice(main_cdk_screen)) == NULL)
        return;

    if (strstr(block_dev, "/dev/disk-by-id") != NULL) {
        /* If its a SCSI disk, we need the real block device node */
        if (readlink(block_dev, real_blk_dev_node, MAX_SYSFS_PATH_SIZE) == -1) {
            asprintf(&error_msg, "readlink(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    } else {
        /* Its not, so use what we have */
        snprintf(real_blk_dev_node, MAX_SYSFS_PATH_SIZE, "%s", block_dev);
    }

    /* Open the file system tab file */
    if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
        asprintf(&error_msg, "setmntent(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        return;
    }

    /* Loop over fstab entries and check if our chosen block device from
     * above matches; this check obviously isn't 100% fail-safe */
    while ((fstab_entry = getmntent(fstab_file)) != NULL) {
        /* We consider the FS device/name from the fstab file the haystack since 
         * these entries would typically have a number at the end for the partition */
        if (strstr(fstab_entry->mnt_fsname, real_blk_dev_node) != NULL) {
            errorDialog(main_cdk_screen,
                    "It appears the selected block device already has a fstab entry.", NULL);
            endmntent(fstab_file);
            return;
        } else if ((strstr(fstab_entry->mnt_fsname, "LABEL=") != NULL) ||
                (strstr(fstab_entry->mnt_fsname, "UUID=") != NULL)) {
            /* Find the device node for the given file system */
            if ((dev_node = blkid_get_devname(NULL, fstab_entry->mnt_fsname, NULL)) == NULL) {
                // TODO: It seems this function returns NULL for an error AND if the FS wasn't found
            }
            if ((strstr(dev_node, real_blk_dev_node) != NULL)) {
                errorDialog(main_cdk_screen,
                        "It appears the selected block device already has a fstab entry.", NULL);
                endmntent(fstab_file);
                return;
            }
        }
    }

    /* Close the fstab file */
    endmntent(fstab_file);

    /* Setup a new CDK screen for required input */
    fs_window_lines = 21;
    fs_window_cols = 70;
    window_y = ((LINES / 2) - (fs_window_lines / 2));
    window_x = ((COLS / 2) - (fs_window_cols / 2));
    fs_window = newwin(fs_window_lines, fs_window_cols,
            window_y, window_x);
    if (fs_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    fs_screen = initCDKScreen(fs_window);
    if (fs_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(fs_window, COLOR_DIALOG_BOX);
    wbkgd(fs_window, COLOR_DIALOG_TEXT);
    wrefresh(fs_window);

    /* Grab the device information */
    if ((device = ped_device_get(real_blk_dev_node)) == NULL) {
        errorDialog(main_cdk_screen, "Calling ped_device_get() failed.", NULL);
        goto cleanup;
    }
    if ((device_size = ped_unit_format_byte(device,
            device->length * device->sector_size)) == NULL) {
        errorDialog(main_cdk_screen, "Calling ped_unit_format_byte() failed.", NULL);
        goto cleanup;
    }
    /* Its okay if this one returns NULL (eg, no disk label) */
    if ((disk_type = ped_disk_probe(device)) != NULL) {
        /* We only read the disk label if it actually has one */
        if ((disk = ped_disk_new(device)) == NULL) {
            errorDialog(main_cdk_screen, "Calling ped_disk_new() failed.", NULL);
            goto cleanup;
        }
    }
    
    /* Information label */
    asprintf(&fs_dialog_msg[0],
            "</31/B>Creating new file system (on block device %.20s)...", real_blk_dev_node);
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&fs_dialog_msg[1], " ");
    asprintf(&fs_dialog_msg[2], "</B>Model:<!B>\t%-20.20s </B>Transport:<!B>\t%s",
            device->model, transport[device->type]);
    asprintf(&fs_dialog_msg[3], "</B>Size:<!B>\t%-20.20s </B>Disk label:<!B>\t%s",
            device_size, (disk_type) ? disk_type->name : "none");
    asprintf(&fs_dialog_msg[4], "</B>Sector size (logical/physical):<!B>\t%lldB/%lldB",
            device->sector_size, device->phys_sector_size);
    asprintf(&fs_dialog_msg[5], " ");
    /* Add partition information (if any) */
    info_line_cnt = 6;
    if (disk && (ped_disk_get_last_partition_num(disk) > 0)) {
        asprintf(&fs_dialog_msg[info_line_cnt], "</B>Current layout: %5s %12s %12s %15s<!B>",
                "No.", "Start", "Size", "FS Type");
        info_line_cnt++;
        for (partition = ped_disk_next_partition(disk, NULL);
                partition && (info_line_cnt < MAX_FS_DIALOG_INFO_LINES);
                partition = ped_disk_next_partition(disk, partition)) {
            if (partition->num < 0)
                continue;
            asprintf(&fs_dialog_msg[info_line_cnt], "\t\t%5d %12lld %12lld %15.15s",
                    partition->num, partition->geom.start, partition->geom.length,
                    (partition->fs_type) ? partition->fs_type->name : "unknown");
            info_line_cnt++;
        }
    } else {
        asprintf(&fs_dialog_msg[info_line_cnt], "</B><No partitions found.><!B>");
        info_line_cnt++;
    }
    add_fs_info = newCDKLabel(fs_screen, (window_x + 1), (window_y + 1),
            fs_dialog_msg, info_line_cnt, FALSE, FALSE);
    if (!add_fs_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(add_fs_info, COLOR_DIALOG_TEXT);

    /* Clean up the libparted stuff */
    free(device_size);
    if (disk != NULL)
        ped_disk_destroy(disk);
    ped_device_destroy(device);

    /* FS label field */
    fs_label = newCDKEntry(fs_screen, (window_x + 1), (window_y + 13),
            "</B>File System Label", NULL,
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vLMIXED,
            MAX_FS_LABEL, 0, MAX_FS_LABEL, FALSE, FALSE);
    if (!fs_label) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(fs_label, COLOR_DIALOG_INPUT);

    /* FS type radio list */
    fs_type = newCDKRadio(fs_screen, (window_x + 22), (window_y + 13),
            NONE, 5, 10, "</B>File System Type", fs_type_opts, 4,
            '#' | COLOR_DIALOG_SELECT, 0,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!fs_type) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(fs_type, COLOR_DIALOG_TEXT);

    /* Buttons */
    ok_button = newCDKButton(fs_screen, (window_x + 26), (window_y + 19),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(fs_screen, (window_x + 36), (window_y + 19),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(fs_screen);
    traverse_ret = traverseCDKScreen(fs_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check FS label value (field entry) */
        strncpy(fs_label_buff, getCDKEntryValue(fs_label), MAX_FS_LABEL);
        i = 0;
        while (fs_label_buff[i] != '\0') {
            /* If the user didn't input an acceptable value, then cancel out */
            if (!VALID_NAME_CHAR(fs_label_buff[i])) {
                errorDialog(main_cdk_screen,
                        "The label entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }
        if (i == 0) {
            errorDialog(main_cdk_screen, "A value for the FS label is required!", NULL);
            goto cleanup;
        }

        /* We should probably also use the findfs tool to see if any devices 
         * actually use the given file system label, but for now we only consider
         * the fstab file as the source of truth */
        if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
            asprintf(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
        while ((fstab_entry = getmntent(fstab_file)) != NULL) {
            if (strstr(fstab_entry->mnt_fsname, "LABEL=") != NULL) {
                /* The strchr function returns NULL if not found */
                if ((tmp_str_ptr = strchr(fstab_entry->mnt_fsname, '=')) != NULL) {
                    tmp_str_ptr++;
                    if (strcmp(tmp_str_ptr, fs_label_buff) == 0) {
                        errorDialog(main_cdk_screen,
                                "It appears this file system label is already in use!", NULL);
                        endmntent(fstab_file);
                        goto cleanup;
                    }
                }
            }
        }
        endmntent(fstab_file);

        /* Get FS type choice */
        temp_int = getCDKRadioSelectedItem(fs_type);

        /* Get confirmation before applying block device changes */
        asprintf(&confirm_msg, "You are about to write a new file system to '%s';",
                real_blk_dev_node);
        confirm = confirmDialog(main_cdk_screen, confirm_msg,
                "this will destroy all data on the device. Are you sure?");
        freeChar(confirm_msg);
        if (confirm) {
            /* A scroll window to show progress */
            make_fs_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                    MAKE_FS_INFO_ROWS+2, MAKE_FS_INFO_COLS+2,
                    "<C></31/B>Setting up new file system...\n",
                    MAX_MAKE_FS_INFO_LINES, TRUE, FALSE);
            if (!make_fs_info) {
                errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(make_fs_info, COLOR_DIALOG_TEXT);
            setCDKSwindowBoxAttribute(make_fs_info, COLOR_DIALOG_BOX);
            drawCDKSwindow(make_fs_info, TRUE);
            i = 0;

            /* Setup the new partition -- only on SCSI block devices, not the others;
             * using the transport type may be a better check for the device nodes */
            if ((strstr(real_blk_dev_node, "/dev/md") == NULL) &&
                    (strstr(real_blk_dev_node, "/dev/mapper") == NULL) &&
                    (strstr(real_blk_dev_node, "/dev/drbd") == NULL)) {
                if (i < MAX_MAKE_FS_INFO_LINES) {
                    asprintf(&swindow_info[i], "<C>Creating new disk label and partition...");
                    addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                    i++;
                }
                if ((device = ped_device_get(real_blk_dev_node)) == NULL) {
                    errorDialog(main_cdk_screen, "Calling ped_device_get() failed.", NULL);
                    goto cleanup;
                }
                disk_type = ped_disk_type_get("gpt");
                if ((disk = ped_disk_new_fresh(device, disk_type)) == NULL) {
                    errorDialog(main_cdk_screen, "Calling ped_disk_new_fresh() failed.", NULL);
                    goto cleanup;
                }
                file_system_type = ped_file_system_type_get(fs_type_opts[temp_int]);
                if ((partition = ped_partition_new(disk, PED_PARTITION_NORMAL,
                        file_system_type, 0, (device->length - 1))) == NULL) {
                    errorDialog(main_cdk_screen, "Calling ped_partition_new() failed.", NULL);
                    goto cleanup;
                }
                constraint = ped_constraint_new_from_max(&partition->geom);
                if (ped_disk_add_partition(disk, partition, constraint) == 0) {
                    errorDialog(main_cdk_screen, "Calling ped_disk_add_partition() failed.", NULL);
                    goto cleanup;
                }
                if (ped_disk_commit_to_dev(disk) == 0) {
                    errorDialog(main_cdk_screen, "Calling ped_disk_commit_to_dev() failed.", NULL);
                    goto cleanup;
                }
                if (ped_disk_commit_to_os(disk) == 0) {
                    errorDialog(main_cdk_screen, "Calling ped_disk_commit_to_os() failed.", NULL);
                    goto cleanup;
                }
                ped_disk_destroy(disk);
                snprintf(new_blk_dev_node, MAX_FS_ATTR_LEN, "%s1", real_blk_dev_node);
            } else {
                /* For other block devices that are not a SCSI disk */
                snprintf(new_blk_dev_node, MAX_FS_ATTR_LEN, "%s", real_blk_dev_node);
            }

            /* Create the file system */
            if (i < MAX_MAKE_FS_INFO_LINES) {
                asprintf(&swindow_info[i], "<C>Creating new %s file system...", fs_type_opts[temp_int]);
                addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                i++;
            }
            snprintf(mkfs_cmd, MAX_SHELL_CMD_LEN, "mkfs.%s -L %s -%s %s > /dev/null 2>&1",
                    fs_type_opts[temp_int], fs_label_buff,
                    ((strcmp(fs_type_opts[temp_int], "xfs") == 0) ? "f" : "F"),
                    new_blk_dev_node);
            ret_val = system(mkfs_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                asprintf(&error_msg, "Running '%s' failed; exited with %d.", mkfs_cmd, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }

            /* Add the new file system entry to the fstab file */
            if (i < MAX_MAKE_FS_INFO_LINES) {
                asprintf(&swindow_info[i], "<C>Adding new entry to fstab file...");
                addCDKSwindow(make_fs_info, swindow_info[i], BOTTOM);
                i++;
            }
            /* Open the original file system tab file */
            if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
                asprintf(&error_msg, "setmntent(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            /* Open the new/temporary file system tab file */
            if ((new_fstab_file = setmntent(FSTAB_TMP, "w+")) == NULL) {
                asprintf(&error_msg, "setmntent(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            /* Loop over the original fstab file, and add each entry to the new file */
            while ((fstab_entry = getmntent(fstab_file)) != NULL) {
                addmntent(new_fstab_file, fstab_entry);
            }
            /* New fstab entry */
            asprintf(&addtl_fstab_entry.mnt_fsname, "LABEL=%s", fs_label_buff);
            asprintf(&addtl_fstab_entry.mnt_dir, "%s/%s", VDISK_MNT_BASE, fs_label_buff);
            asprintf(&addtl_fstab_entry.mnt_type, "%s", fs_type_opts[temp_int]);
            asprintf(&addtl_fstab_entry.mnt_opts, "defaults");
            addtl_fstab_entry.mnt_freq = 1;
            addtl_fstab_entry.mnt_passno = 1;
            addmntent(new_fstab_file, &addtl_fstab_entry);
            fflush(new_fstab_file);
            endmntent(new_fstab_file);
            endmntent(fstab_file);
            freeChar(addtl_fstab_entry.mnt_fsname);
            freeChar(addtl_fstab_entry.mnt_dir);
            freeChar(addtl_fstab_entry.mnt_type);
            freeChar(addtl_fstab_entry.mnt_opts);
            if ((rename(FSTAB_TMP, FSTAB)) == -1) {
                asprintf(&error_msg, "rename(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            /* Make the mount point directory */
            snprintf(new_mnt_point, MAX_FS_ATTR_LEN, "%s/%s", VDISK_MNT_BASE, fs_label_buff);
            if ((mkdir(new_mnt_point, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) == -1) {
                asprintf(&error_msg, "mkdir(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }

            if (make_fs_info)
                destroyCDKSwindow(make_fs_info);
            refreshCDKScreen(main_cdk_screen);

            /* Ask if user would like to mount the new FS */
            question = questionDialog(main_cdk_screen,
                    "Would you like to mount the new file system now?", NULL);
            if (question) {
                /* Run mount */
                snprintf(mount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1", MOUNT_BIN, new_mnt_point);
                ret_val = system(mount_cmd);
                if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                    asprintf(&error_msg, "Running %s failed; exited with %d.", MOUNT_BIN, exit_stat);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    return;
                }
            }
        }
    }

    /* All done */
    cleanup:
    if (make_fs_info)
        destroyCDKSwindow(make_fs_info);
    for (i = 0; i < MAX_MAKE_FS_INFO_LINES; i++)
        freeChar(swindow_info[i]);
    for (i = 0; i < info_line_cnt; i++)
        freeChar(fs_dialog_msg[i]);
    if (fs_screen != NULL) {
        destroyCDKScreenObjects(fs_screen);
        destroyCDKScreen(fs_screen);
    }
    delwin(fs_window);
    return;
}


/*
 * Run the Remove File System dialog
 */
void removeFSDialog(CDKSCREEN *main_cdk_screen) {
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0}, umount_cmd[MAX_SHELL_CMD_LEN] = {0};
    char *confirm_msg = NULL, *error_msg = NULL;
    boolean mounted = FALSE, question = FALSE, confirm = FALSE;
    FILE *fstab_file = NULL, *new_fstab_file = NULL;
    struct mntent *fstab_entry = NULL;
    int ret_val = 0, exit_stat = 0;

    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    /* If the selected file system is mounted, ask to try un-mounting it */
    if (mounted) {
        question = questionDialog(main_cdk_screen,
                "It appears that file system is mounted; would you like to try un-mounting it now?",
                "(The file system must be un-mounted before proceeding.)");
        if (question) {
            /* Run umount */
            snprintf(umount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1", UMOUNT_BIN, fs_path);
            ret_val = system(umount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                asprintf(&error_msg, "Running %s failed; exited with %d.", UMOUNT_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                return;
            }
        } else {
            return;
        }
    }
    
    /* Get confirmation before removing the file system */
    asprintf(&confirm_msg, "'%s' file system?", fs_name);
    confirm = confirmDialog(main_cdk_screen, "Are you sure you would like to remove the", confirm_msg);
    freeChar(confirm_msg);

    /* Remove file system entry from fstab file */
    if (confirm) {
        /* Open the original file system tab file */
        if ((fstab_file = setmntent(FSTAB, "r")) == NULL) {
            asprintf(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
        /* Open the new/temporary file system tab file */
        if ((new_fstab_file = setmntent(FSTAB_TMP, "w+")) == NULL) {
            asprintf(&error_msg, "setmntent(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
        /* Loop over the original fstab file, and skip the entry we are removing
         when writing out the new file */
        while ((fstab_entry = getmntent(fstab_file)) != NULL) {
            if (strcmp(fs_name, fstab_entry->mnt_fsname) != 0) {
                addmntent(new_fstab_file, fstab_entry);
            }
        }
        fflush(new_fstab_file);
        endmntent(new_fstab_file);
        endmntent(fstab_file);
        if ((rename(FSTAB_TMP, FSTAB)) == -1) {
            asprintf(&error_msg, "rename(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
        
        /* Remove the mount point directory */
        if ((rmdir(fs_path)) == -1) {
            asprintf(&error_msg, "rmdir(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
        
        // TODO: Should we also remove/erase the file system label from disk?
    }
    
    /* Done */
    return;
}


/*
 * Run the Add Virtual Disk File dialog
 */
void addVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *vdisk_window = 0;
    CDKSCREEN *vdisk_screen = 0;
    CDKLABEL *vdisk_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *vdisk_name = 0, *vdisk_size = 0;
    CDKHISTOGRAM *vdisk_progress = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0}, mount_cmd[MAX_SHELL_CMD_LEN] = {0},
            vdisk_name_buff[MAX_VDISK_NAME] = {0}, gib_free_str[MISC_STRING_LEN] = {0},
            gib_total_str[MISC_STRING_LEN] = {0}, zero_buff[VDISK_WRITE_SIZE] = {0},
            new_vdisk_file[MAX_VDISK_PATH_LEN] = {0};
    char *error_msg = NULL;
    char *vdisk_dialog_msg[ADD_VDISK_INFO_LINES] = {NULL};
    boolean mounted = FALSE, question = FALSE;
    struct statvfs *fs_info = NULL;
    int window_y = 0, window_x = 0, traverse_ret = 0, i = 0, exit_stat = 0,
            ret_val = 0, vdisk_size_int = 0, new_vdisk_fd = 0,
            vdisk_window_lines = 0, vdisk_window_cols = 0;
    long long bytes_free = 0ll, bytes_total = 0ll, new_vdisk_bytes = 0ll,
            new_vdisk_mib = 0ll;
    off_t position = 0;
    ssize_t bytes_written = 0, write_length = 0;
    
    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    if (!mounted) {
        question = questionDialog(main_cdk_screen,
                "It appears that file system is not mounted; would you like to try mounting it now?",
                "(The file system must be mounted before proceeding.)");
        if (question) {
            /* Run mount */
            snprintf(mount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1", MOUNT_BIN, fs_path);
            ret_val = system(mount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                asprintf(&error_msg, "Running %s failed; exited with %d.", MOUNT_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                return;
            }
        } else {
            return;
        }
    }
    
    /* Setup a new small CDK screen for virtual disk information */
    vdisk_window_lines = 12;
    vdisk_window_cols = 70;
    window_y = ((LINES / 2) - (vdisk_window_lines / 2));
    window_x = ((COLS / 2) - (vdisk_window_cols / 2));
    vdisk_window = newwin(vdisk_window_lines, vdisk_window_cols,
            window_y, window_x);
    if (vdisk_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    vdisk_screen = initCDKScreen(vdisk_window);
    if (vdisk_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(vdisk_window, COLOR_DIALOG_BOX);
    wbkgd(vdisk_window, COLOR_DIALOG_TEXT);
    wrefresh(vdisk_window);

    /* Get the file system information */
    if (!(fs_info = (struct statvfs *) malloc(sizeof (struct statvfs)))) {
        errorDialog(main_cdk_screen, "Calling malloc() failed.", NULL);
        goto cleanup;
    }
    if ((statvfs(fs_path, fs_info)) == -1) {
        asprintf(&error_msg, "statvfs(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    bytes_free = fs_info->f_bavail * fs_info->f_bsize;
    snprintf(gib_free_str, MISC_STRING_LEN, "%lld GiB", (bytes_free / GIBIBYTE_SIZE));
    bytes_total = fs_info->f_blocks * fs_info->f_bsize;
    snprintf(gib_total_str, MISC_STRING_LEN, "%lld GiB", (bytes_total / GIBIBYTE_SIZE));
    
    /* Fill the information label */
    asprintf(&vdisk_dialog_msg[0], "</31/B>Adding new virtual disk file...");
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&vdisk_dialog_msg[1], " ");
    asprintf(&vdisk_dialog_msg[2], "</B>File System:<!B>\t%-20.20s </B>Type:<!B>\t\t%s",
            fs_path, fs_type);
    asprintf(&vdisk_dialog_msg[3], "</B>Size:<!B>\t\t%-20.20s </B>Available Space:<!B>\t%s",
            gib_total_str, gib_free_str);
    free(fs_info);
    vdisk_label = newCDKLabel(vdisk_screen, (window_x + 1), (window_y + 1),
            vdisk_dialog_msg, ADD_VDISK_INFO_LINES, FALSE, FALSE);
    if (!vdisk_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(vdisk_label, COLOR_DIALOG_TEXT);

    /* Virtual disk file name */
    vdisk_name = newCDKEntry(vdisk_screen, (window_x + 1), (window_y + 6),
            "</B>Virtual Disk File Name", NULL,
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vLMIXED,
            20, 0, MAX_VDISK_NAME, FALSE, FALSE);
    if (!vdisk_name) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(vdisk_name, COLOR_DIALOG_INPUT);

    /* Virtual disk size */
    vdisk_size = newCDKEntry(vdisk_screen, (window_x + 30), (window_y + 6),
            "</B>Virtual Disk Size (GiB)", NULL,
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT,
            12, 0, 12, FALSE, FALSE);
    if (!vdisk_size) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(vdisk_size, COLOR_DIALOG_INPUT);

    /* Buttons */
    ok_button = newCDKButton(vdisk_screen, (window_x + 26), (window_y + 10),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(vdisk_screen, (window_x + 36), (window_y + 10),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(vdisk_screen);
    traverse_ret = traverseCDKScreen(vdisk_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check virtual disk name value (field entry) */
        strncpy(vdisk_name_buff, getCDKEntryValue(vdisk_name), MAX_VDISK_NAME);
        i = 0;
        while (vdisk_name_buff[i] != '\0') {
            /* If the user didn't input an acceptable value, then cancel out */
            if (!VALID_NAME_CHAR(vdisk_name_buff[i])) {
                errorDialog(main_cdk_screen,
                        "The name entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }
        if (i == 0) {
            errorDialog(main_cdk_screen, "A value for the virtual disk name is required!", NULL);
            goto cleanup;
        }

        /* Check virtual disk size value (field entry) */
        vdisk_size_int = atoi(getCDKEntryValue(vdisk_size));
        if (vdisk_size_int <= 0) {
            errorDialog(main_cdk_screen, "The size value must be greater than zero.", NULL);
            goto cleanup;
        } 
        new_vdisk_bytes = vdisk_size_int * GIBIBYTE_SIZE;
        if (new_vdisk_bytes > bytes_free) {
            errorDialog(main_cdk_screen, "The given size is greater than the available space!", NULL);
            goto cleanup;
        }

        /* Check if the new (potential) virtual disk file exists already */
        snprintf(new_vdisk_file, MAX_VDISK_PATH_LEN, "%s/%s", fs_path, vdisk_name_buff);
        if (access(new_vdisk_file, F_OK) != -1) {
            asprintf(&error_msg, "It appears the '%s'", new_vdisk_file);
            errorDialog(main_cdk_screen, error_msg, "file already exists!");
            freeChar(error_msg);
            goto cleanup;
        }
        
        /* Clean up the screen */
        destroyCDKScreenObjects(vdisk_screen);
        destroyCDKScreen(vdisk_screen);
        vdisk_screen = NULL;
        delwin(vdisk_window);
        refreshCDKScreen(main_cdk_screen);

        /* Make a new histogram widget to display the virtual disk creation progress */
        vdisk_progress = newCDKHistogram(main_cdk_screen, CENTER, CENTER,
                1, 50, HORIZONTAL, "<C></31/B>Writing new virtual disk file (units = MiB):\n",
                TRUE, FALSE);
        if (!vdisk_progress) {
            errorDialog(main_cdk_screen, "Couldn't create histogram widget!", NULL);
            goto cleanup;
        }
        setCDKScrollBoxAttribute(vdisk_progress, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(vdisk_progress, COLOR_DIALOG_TEXT);
        drawCDKHistogram(vdisk_progress, TRUE);

        /* We'll use mebibyte as our unit for the histogram widget; need to
         * make sure an int can handle it, which is used by the histogram widget */
        new_vdisk_mib = new_vdisk_bytes / MEBIBYTE_SIZE;
        if (new_vdisk_mib > INT_MAX) {
            errorDialog(main_cdk_screen, "An integer will not hold the virtual",
                    "disk size (MiB) on this system!");
            goto cleanup;
        }

        /* Open our (new) virtual disk file */
        if ((new_vdisk_fd = open(new_vdisk_file, O_WRONLY | O_CREAT)) == -1) {
            asprintf(&error_msg, "open(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }

        /* Zero-out the new virtual disk file to the length specified (size) */
        memset(zero_buff, 0, VDISK_WRITE_SIZE);
        for (position = 0; position < new_vdisk_bytes; position += write_length) {
            write_length = MIN((new_vdisk_bytes - position), VDISK_WRITE_SIZE);
            /* Use fallocate() for modern file systems, and write() for others */
            if ((strcmp(fs_type, "xfs") == 0) || (strcmp(fs_type, "ext4") == 0)) {
                if (fallocate(new_vdisk_fd, 0, position,
                        write_length) == -1) {
                    asprintf(&error_msg, "fallocate(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    close(new_vdisk_fd);
                    goto cleanup;
                }
            } else {
                bytes_written = write(new_vdisk_fd, zero_buff, write_length);
                if (bytes_written == -1) {
                    asprintf(&error_msg, "write(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    close(new_vdisk_fd);
                    goto cleanup;
                } else if (bytes_written != write_length) {
                    errorDialog(main_cdk_screen,
                            "The number of bytes written is different",
                            "than the requested write length!");
                    close(new_vdisk_fd);
                    goto cleanup;
                }
            }
            /* This controls how often the progress bar is updated; it can
             * adversely affect performance of the write operation if its
             * updated too frequently */
            if ((position % (VDISK_WRITE_SIZE * 1000)) == 0) {
                /* Since our maximum size was checked above against an int type,
                 * we'll assume we're safe if we made it this far */
                setCDKHistogram(vdisk_progress, vPERCENT, CENTER,
                        COLOR_DIALOG_TEXT, 0, new_vdisk_mib,
                        (position / MEBIBYTE_SIZE), ' ' | A_REVERSE, TRUE);
                drawCDKHistogram(vdisk_progress, TRUE);
            }
        }

        /* We've completed writing the new file; update the progress bar */
        setCDKHistogram(vdisk_progress, vPERCENT, CENTER, COLOR_DIALOG_TEXT,
                0, new_vdisk_mib, new_vdisk_mib,
                ' ' | A_REVERSE, TRUE);
        drawCDKHistogram(vdisk_progress, TRUE);
        
        // TODO: Do we actually need to flush to disk before returning?
        if (fsync(new_vdisk_fd) == -1) {
            asprintf(&error_msg, "fsync(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            close(new_vdisk_fd);
            goto cleanup;
        }
        if (close(new_vdisk_fd) == -1) {
            asprintf(&error_msg, "close(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
        destroyCDKHistogram(vdisk_progress);
        vdisk_progress = NULL;
    }

    /* Done */
    cleanup:
    for (i = 0; i < ADD_VDISK_INFO_LINES; i++)
        freeChar(vdisk_dialog_msg[i]);
    if (vdisk_progress != NULL)
        destroyCDKHistogram(vdisk_progress);
    if (vdisk_screen != NULL) {
        destroyCDKScreenObjects(vdisk_screen);
        destroyCDKScreen(vdisk_screen);
    }
    delwin(vdisk_window);
    return;
}


/*
 * Run the Delete Virtual Disk File dialog
 */
void delVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    CDKFSELECT *file_select = 0;
    char fs_name[MAX_FS_ATTR_LEN] = {0}, fs_path[MAX_FS_ATTR_LEN] = {0},
            fs_type[MAX_FS_ATTR_LEN] = {0}, mount_cmd[MAX_SHELL_CMD_LEN] = {0};
    char *error_msg = NULL, *selected_file = NULL, *confirm_msg = NULL;
    boolean mounted = FALSE, question = FALSE, confirm = FALSE;
    int exit_stat = 0, ret_val = 0;
    
    /* Have the user select a file system to remove */
    getFSChoice(main_cdk_screen, fs_name, fs_path, fs_type, &mounted);
    if (fs_name[0] == '\0')
        return;

    if (!mounted) {
        question = questionDialog(main_cdk_screen,
                "It appears that file system is not mounted; would you like to try mounting it now?",
                "(The file system must be mounted before proceeding.)");
        if (question) {
            /* Run mount */
            snprintf(mount_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1", MOUNT_BIN, fs_path);
            ret_val = system(mount_cmd);
            if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
                asprintf(&error_msg, "Running %s failed; exited with %d.", MOUNT_BIN, exit_stat);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                return;
            }
        } else {
            return;
        }
    }

    /* Create the file selector widget */
    file_select = newCDKFselect(main_cdk_screen, CENTER, CENTER, 20, 40,
            "<C></31/B>Choose a virtual disk file to delete:\n", "VDisk File: ",
            COLOR_DIALOG_INPUT, '_' | COLOR_DIALOG_INPUT,
            A_REVERSE, "</N>", "</B>", "</N>", "</N>", TRUE, FALSE);
    if (!file_select) {
        errorDialog(main_cdk_screen, "Couldn't create file selector widget!", NULL);
        return;
    }
    setCDKFselectBoxAttribute(file_select, COLOR_DIALOG_BOX);
    setCDKFselectBackgroundAttrib(file_select, COLOR_DIALOG_TEXT);
    setCDKFselectDirectory(file_select, fs_path);

    /* Activate the widget and let the user choose a file */
    selected_file = activateCDKFselect(file_select, 0);
    if (file_select->exitType == vNORMAL) {
        /* Get confirmation before deleting the virtual disk file */
        asprintf(&confirm_msg, "'%s'?", selected_file);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to delete virtual disk file", confirm_msg);
        freeChar(confirm_msg);
        if (confirm) {
            /* Delete (unlink) the virtual disk file */
            if (unlink(selected_file) == -1) {
                asprintf(&error_msg, "unlink(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
            }
        }
    }

    /* Done */
    destroyCDKFselect(file_select);
    /* Using the file selector widget changes the CWD -- fix it */
    if ((chdir(getenv("HOME"))) == -1) {
        asprintf(&error_msg, "chdir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    }
    return;
}
