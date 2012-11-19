/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>

#include "menu_actions-back_storage.h"
#include "menu_actions.h"
#include "main.h"
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
            traverse_ret = 0, temp_int = 0;
    int adp_window_lines = 20, adp_window_cols = 60;
    static char *dsbl_enbl[] = {"Disabled", "Enabled"};
    char temp_str[10] = {0};
    char *error_msg = NULL;
    char *adp_info_msg[9] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    adp_count = getMRAdapterCount();

    /* Get the adapter properties */
    mr_adp_props = getMRAdapterProps(adp_choice);
    if (!mr_adp_props) {
        errorDialog(main_cdk_screen, "Error getting adapter properties.", NULL);
        return;
    }

    /* New CDK screen for selected adapter */
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
    asprintf(&adp_info_msg[0], "</31/B>MegaRAID Adapter # %d: %s",
            adp_choice, mr_adapters[adp_choice]->prod_name);
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&adp_info_msg[1], " ");
    asprintf(&adp_info_msg[2], "Serial number:\t\t%s",
            mr_adapters[adp_choice]->serial);
    asprintf(&adp_info_msg[3], "Firmware Version:\t%s",
            mr_adapters[adp_choice]->firmware);
    asprintf(&adp_info_msg[4], "Memory:\t\t\t%s",
            mr_adapters[adp_choice]->memory);
    asprintf(&adp_info_msg[5], "Battery:\t\t%s",
            mr_adapters[adp_choice]->bbu);
    asprintf(&adp_info_msg[6], "Host Interface:\t\t%s",
            mr_adapters[adp_choice]->interface);
    asprintf(&adp_info_msg[7], "Physical Disks:\t\t%d",
            mr_adapters[adp_choice]->disk_cnt);
    asprintf(&adp_info_msg[8], "Logical Drives:\t\t%d",
            mr_adapters[adp_choice]->logical_drv_cnt);
    adapter_info = newCDKLabel(adapter_screen, (window_x + 1), (window_y + 1),
            adp_info_msg, 9, FALSE, FALSE);
    if (!adapter_info) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(adapter_info, COLOR_DIALOG_TEXT);

    /* Field entry widgets */
    cache_flush = newCDKEntry(adapter_screen, (window_x + 1), (window_y + 11),
            NULL, "</B>Cache Flush Interval (0 to 255): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
            FALSE, FALSE);
    if (!cache_flush) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(cache_flush, COLOR_DIALOG_INPUT);
    sprintf(temp_str, "%d", mr_adp_props->cache_flush);
    setCDKEntryValue(cache_flush, temp_str);
    rebuild_rate = newCDKEntry(adapter_screen, (window_x + 1), (window_y + 12),
            NULL, "</B>Rebuild Rate (0 to 100): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
            FALSE, FALSE);
    if (!rebuild_rate) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(rebuild_rate, COLOR_DIALOG_INPUT);
    sprintf(temp_str, "%d", mr_adp_props->rebuild_rate);
    setCDKEntryValue(rebuild_rate, temp_str);

    /* Radio lists */
    cluster_radio = newCDKRadio(adapter_screen, (window_x + 1), (window_y + 14),
            NONE, 3, 10, "</B>Cluster", dsbl_enbl, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!cluster_radio) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(cluster_radio, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(cluster_radio, (int) mr_adp_props->cluster);
    ncq_radio = newCDKRadio(adapter_screen, (window_x + 16), (window_y + 14),
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
    ok_button = newCDKButton(adapter_screen, (window_x + 16), (window_y + 18),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(adapter_screen, (window_x + 26), (window_y + 18),
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
    for (i = 0; i < 9; i++) {
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
    asprintf(&encl_title, "<C></31/B>Enclosures/Slots on MegaRAID Adapter # %d:\n",
            adp_choice);
    encl_swindow = newCDKSwindow(main_cdk_screen, CENTER, CENTER, 18, 74,
            encl_title, MAX_ADP_INFO_LINES, TRUE, FALSE);
    if (!encl_swindow) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(encl_swindow, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(encl_swindow, COLOR_DIALOG_BOX);

    /* Add enclosure/disk information */
    line_pos = 0;
    for (i = 0; i < encl_count; i++) {
        asprintf(&swindow_info[line_pos],
                "</B>Enclosure %d (%s %s)",
                i, mr_enclosures[i]->vendor, mr_enclosures[i]->product);
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        line_pos++;
        asprintf(&swindow_info[line_pos],
                "</B>Device ID: %d, Status: %s, Slots: %d",
                mr_enclosures[i]->device_id, mr_enclosures[i]->status,
                mr_enclosures[i]->slots);
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        for (j = 0; j < mr_enclosures[i]->slots; j++) {
            line_pos++;
            if (mr_disks[i][j]->present)
                asprintf(&swindow_info[line_pos], "\tSlot %3d: %s", j, mr_disks[i][j]->inquiry);
            else
                asprintf(&swindow_info[line_pos], "\tSlot %3d: Not Present", j);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        }
        line_pos++;
    }

    /* Add a message to the bottom explaining how to close the dialog */
    asprintf(&swindow_info[line_pos], " ");
    addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
    line_pos++;
    asprintf(&swindow_info[line_pos], CONTINUE_MSG);
    addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
    line_pos++;

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
    CDKENTRY *name_field = 0;
    CDKRADIO *cache_policy = 0, *write_cache = 0, *read_cache = 0,
            *bbu_cache = 0, *raid_lvl = 0, *strip_size = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int adp_choice = 0, adp_count = 0, i = 0, j = 0, encl_count = 0,
            selection_size = 0, chosen_disk_cnt = 0, traverse_ret = 0,
            temp_int = 0, window_y = 0, window_x = 0;
    int new_ld_window_lines = 20, new_ld_window_cols = 70;
    static char *choice_char[] = {" ", "*"};
    static char *cache_opts[] = {"Cached", "Direct"};
    static char *write_opts[] = {"WT", "WB"};
    static char *read_opts[] = {"NORA", "RA", "ADRA"};
    static char *bbu_opts[] = {"CachedBadBBU", "NoCachedBadBBU"};
    static char *raid_opts[] = {"0", "1", "5", "6"};
    static char *strip_opts[] = {"8", "16", "32", "64", "128", "256", "512", "1024"};
    char temp_str[20] = {0}, new_ld_disks[120] = {0},
            new_ld_raid_lvl[MAX_MR_ATTR_SIZE] = {0},
            new_ld_strip_size[MAX_MR_ATTR_SIZE] = {0};
    char *error_msg = NULL, *dsk_select_title = NULL, *temp_pstr = NULL;
    char *selection_list[MAX_DISKS] = {NULL}, *new_ld_msg[3] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
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
                                    "Enclosure %d, Slot %d: %s", i, j,
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
            "<C></31/B>Select Disks for New Logical Drive (Adapter # %d):\n",
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
        strcat(new_ld_disks, temp_pstr);
        freeChar(temp_pstr);
    }

    /* Make a new label for the add-logical-drive screen */
    asprintf(&new_ld_msg[0],
            "</31/B>Creating New MegaRAID Logical Drive (on adapter %d)",
            adp_choice);
    /* Using asprintf for a blank space makes it easier on clean-up (free) */
    asprintf(&new_ld_msg[1], " ");
    asprintf(&new_ld_msg[2], "Selected Disks [ENCL:SLOT] - %s", new_ld_disks);
    new_ld_label = newCDKLabel(new_ld_screen, (window_x + 1), (window_y + 1),
            new_ld_msg, 3, FALSE, FALSE);
    if (!new_ld_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(new_ld_label, COLOR_DIALOG_TEXT);

    /* Name field */
    name_field = newCDKEntry(new_ld_screen, (window_x + 1), (window_y + 5),
            NULL, "</B>Logical drive name (no spaces): ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vUMIXED,
            MAX_LD_NAME, 0, MAX_LD_NAME, FALSE, FALSE);
    if (!name_field) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(name_field, COLOR_DIALOG_INPUT);

    /* RAID level radio list */
    raid_lvl = newCDKRadio(new_ld_screen, (window_x + 1), (window_y + 7),
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
    strip_size = newCDKRadio(new_ld_screen, (window_x + 15), (window_y + 7),
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
    write_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 7),
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
    read_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 11),
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
    cache_policy = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 7),
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
    bbu_cache = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 11),
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
    ok_button = newCDKButton(new_ld_screen, (window_x + 26), (window_y + 18),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(new_ld_screen, (window_x + 36), (window_y + 18),
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
        /* Check name (field entry) */
        strncpy(temp_str, getCDKEntryValue(name_field), MAX_LD_NAME);
        i = 0;
        while (temp_str[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (isspace(temp_str[i]) || !isalnum(temp_str[i])) {
                errorDialog(main_cdk_screen,
                        "Name field must only contain alphanumeric characters!", NULL);
                goto cleanup;
            }
            i++;
        }

        /* Set new LD properties */
        new_ld_props = (MRLDPROPS *) calloc(1, sizeof(MRLDPROPS));
        strcpy(new_ld_props->name, getCDKEntryValue(name_field));
        new_ld_props->adapter_id = adp_choice;
        temp_int = getCDKRadioSelectedItem(cache_policy);
        strcpy(new_ld_props->cache_policy, cache_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(write_cache);
        strcpy(new_ld_props->write_policy, write_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(read_cache);
        strcpy(new_ld_props->read_policy, read_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(bbu_cache);
        strcpy(new_ld_props->bbu_cache_policy, bbu_opts[temp_int]);

        /* Check that the given RAID level and number of disks is
         * a valid combination */
        temp_int = getCDKRadioSelectedItem(raid_lvl);
        strcpy(new_ld_raid_lvl, raid_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(strip_size);
        strcpy(new_ld_strip_size, strip_opts[temp_int]);

        /* Create the new logical drive */
        temp_int = addMRLogicalDrive(new_ld_props, chosen_disk_cnt,
                chosen_disks, new_ld_raid_lvl, new_ld_strip_size);
        if (temp_int != 0) {
            asprintf(&error_msg, "Error creating new logical drive; MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done -- free everything and clean up */
    cleanup:
    free(new_ld_props);
    freeChar(dsk_select_title);
    for (i = 0; i < MAX_DISKS; i++) {
        freeChar(selection_list[i]);
    }
    for (i = 0; i < 3; i++) {
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
    static char *ld_list_title = "<C></31/B>Choose a logical drive:\n";
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
    ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 8, 50,
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
    asprintf(&confirm_msg,
            "Are you sure you want to delete logical drive # %d on adapter %d?",
            mr_ldrives[ld_choice]->ldrive_id, adp_choice);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
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
    int ld_window_lines = 20, ld_window_cols = 70;
    int window_y = 0, window_x = 0, traverse_ret = 0, temp_int = 0,
            adp_count = 0, adp_choice = 0, i = 0, ld_count = 0, ld_choice = 0;
    int ld_encl_ids[MAX_MR_DISKS], ld_slots[MAX_MR_DISKS], mr_ld_ids[MAX_MR_LDS];
    static char *ld_list_title = "<C></31/B>Choose a logical drive:\n";
    static char *cache_opts[] = {"Cached", "Direct"};
    static char *write_opts[] = {"WT", "WB"};
    static char *read_opts[] = {"NORA", "RA", "ADRA"};
    static char *bbu_opts[] = {"CachedBadBBU", "NoCachedBadBBU"};
    char temp_str[20] = {0}, ld_disks[120] = {0};
    char *temp_pstr = NULL, *error_msg = NULL;
    char *logical_drives[MAX_MR_LDS] = {NULL}, *ld_info_msg[9] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
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
    ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 8, 50,
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
        strcat(ld_disks, temp_pstr);
        freeChar(temp_pstr);
    }

    /* Logical drive info. label */
    asprintf(&ld_info_msg[0], "</31/B>MegaRAID Logical Drive # %d (on adapter %d)",
            mr_ldrives[ld_choice]->ldrive_id, adp_choice);
    asprintf(&ld_info_msg[1], " ");
    asprintf(&ld_info_msg[2], "RAID Level:\t%s",
            mr_ldrives[ld_choice]->raid_lvl);
    asprintf(&ld_info_msg[3], "Size:\t\t%s",
            mr_ldrives[ld_choice]->size);
    asprintf(&ld_info_msg[4], "State:\t\t%s",
            mr_ldrives[ld_choice]->state);
    asprintf(&ld_info_msg[5], "Strip Size:\t%s",
            mr_ldrives[ld_choice]->strip_size);
    asprintf(&ld_info_msg[6], "Drive Count:\t%d",
            mr_ldrives[ld_choice]->drive_cnt);
    asprintf(&ld_info_msg[7], " ");
    asprintf(&ld_info_msg[8], "Disks [ENCL:SLOT] - %s", ld_disks);
    ld_info = newCDKLabel(ld_screen, (window_x + 1), (window_y + 1),
            ld_info_msg, 9, FALSE, FALSE);
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
        /* Check name (field entry) */
        strncpy(temp_str, getCDKEntryValue(name_field), MAX_LD_NAME);
        i = 0;
        while (temp_str[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (isspace(temp_str[i]) || !isalnum(temp_str[i])) {
                errorDialog(main_cdk_screen,
                        "Name field must only contain alphanumeric characters!", NULL);
                goto cleanup;
            }
            i++;
        }
        strncpy(mr_ld_props->name, getCDKEntryValue(name_field), MAX_LD_NAME);

        /* Set radio inputs */
        temp_int = getCDKRadioSelectedItem(cache_policy);
        strcpy(mr_ld_props->cache_policy, cache_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(write_cache);
        strcpy(mr_ld_props->write_policy, write_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(read_cache);
        strcpy(mr_ld_props->read_policy, read_opts[temp_int]);
        temp_int = getCDKRadioSelectedItem(bbu_cache);
        strcpy(mr_ld_props->bbu_cache_policy, bbu_opts[temp_int]);

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
    for (i = 0; i < 9; i++) {
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
    char *error = NULL;
    int i = 0, line_pos = 0;
    char line[MAX_PROC_LINE] = {0};
    FILE *drbd_file = NULL;

    /* Open the file */
    if ((drbd_file = fopen(PROC_DRBD, "r")) == NULL) {
        asprintf(&error, "fopen: %s", strerror(errno));
        errorDialog(main_cdk_screen, error, NULL);
        freeChar(error);
    } else {
        /* Setup scrolling window widget */
        drbd_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER, 12, 60,
                "<C></31/B>Distributed Replicated Block Device (DRBD) Information:\n",
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
            asprintf(&swindow_info[line_pos], "%s", line);
            line_pos++;
        }
        fclose(drbd_file);

        /* Add a message to the bottom explaining how to close the dialog */
        asprintf(&swindow_info[line_pos], " ");
        line_pos++;
        asprintf(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;

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
    char *error = NULL;
    int i = 0, line_pos = 0;
    char line[MAX_PROC_LINE] = {0};
    FILE *mdstat_file = NULL;

    /* Open the file */
    if ((mdstat_file = fopen(PROC_MDSTAT, "r")) == NULL) {
        asprintf(&error, "fopen: %s", strerror(errno));
        errorDialog(main_cdk_screen, error, NULL);
        freeChar(error);
    } else {
        /* Setup scrolling window widget */
        mdstat_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER, 12, 60,
                "<C></31/B>Linux Software RAID (md) Status:\n",
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
            asprintf(&swindow_info[line_pos], "%s", line);
            line_pos++;
        }
        fclose(mdstat_file);

        /* Add a message to the bottom explaining how to close the dialog */
        asprintf(&swindow_info[line_pos], " ");
        line_pos++;
        asprintf(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;

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
    char *error = NULL, *lvdisplay_cmd = NULL;
    int i = 0, line_pos = 0, status = 0, ret_val = 0;
    char line[MAX_PROC_LINE] = {0};
    FILE *lvdisplay_proc = NULL;

    /* Run the lvdisplay command */
    asprintf(&lvdisplay_cmd, "%s --all 2>&1", LVDISPLAY_BIN);
    if ((lvdisplay_proc = popen(lvdisplay_cmd, "r")) == NULL) {
        asprintf(&error, "Couldn't open process for the %s command!", LVDISPLAY_BIN);
        errorDialog(main_cdk_screen, error, NULL);
        freeChar(error);
    } else {
        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), lvdisplay_proc) != NULL) {
            asprintf(&swindow_info[line_pos], "%s", line);
            line_pos++;
        }

        /* Add a message to the bottom explaining how to close the dialog */
        asprintf(&swindow_info[line_pos], " ");
        line_pos++;
        asprintf(&swindow_info[line_pos], CONTINUE_MSG);
        line_pos++;
        
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
            lvm2_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER, 12, 60,
                    "<C></31/B>LVM2 Logical Volume Information:\n",
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
            asprintf(&error, "The %s command exited with %d.", LVDISPLAY_BIN, ret_val);
            errorDialog(main_cdk_screen, error, NULL);
            freeChar(error);
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
    errorDialog(main_cdk_screen, NULL, "This feature has not been implemented yet.");
    return;
}


/*
 * Run the Remove File System dialog
 */
void removeFSDialog(CDKSCREEN *main_cdk_screen) {
    errorDialog(main_cdk_screen, NULL, "This feature has not been implemented yet.");
    return;
}


/*
 * Run the Add Virtual Disk File dialog
 */
void addVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    errorDialog(main_cdk_screen, NULL, "This feature has not been implemented yet.");
    return;
}


/*
 * Run the Delete Virtual Disk File dialog
 */
void delVDiskFileDialog(CDKSCREEN *main_cdk_screen) {
    errorDialog(main_cdk_screen, NULL, "This feature has not been implemented yet.");
    return;
}