/**
 * @file menu_hwraid.c
 * @brief Contains the menu actions for the 'Hardware RAID' menu.
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
    char temp_str[MAX_MR_ATTR_SIZE] = {0};
    char *error_msg = NULL;
    char *adp_info_msg[ADP_PROP_INFO_LINES] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    if ((adp_count = getMRAdapterCount()) == -1) {
        errorDialog(main_cdk_screen, "Error getting adapter count.", NULL);
        return;
    }

    /* Get the adapter properties */
    mr_adp_props = getMRAdapterProps(adp_choice);
    if (!mr_adp_props) {
        errorDialog(main_cdk_screen, "Error getting adapter properties.", NULL);
        return;
    }

    while (1) {
        /* New CDK screen for selected adapter */
        adp_window_lines = 21;
        adp_window_cols = 60;
        window_y = ((LINES / 2) - (adp_window_lines / 2));
        window_x = ((COLS / 2) - (adp_window_cols / 2));
        adapter_window = newwin(adp_window_lines, adp_window_cols,
                window_y, window_x);
        if (adapter_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        adapter_screen = initCDKScreen(adapter_window);
        if (adapter_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(adapter_window, COLOR_DIALOG_BOX);
        wbkgd(adapter_window, COLOR_DIALOG_TEXT);
        wrefresh(adapter_window);

        /* Adapter info. label */
        SAFE_ASPRINTF(&adp_info_msg[0],
                "</31/B>Properties for MegaRAID adapter # %d...", adp_choice);
        SAFE_ASPRINTF(&adp_info_msg[1], " ");
        SAFE_ASPRINTF(&adp_info_msg[2], "</B>Model:<!B>\t\t\t%-30s",
                mr_adapters[adp_choice]->prod_name);
        SAFE_ASPRINTF(&adp_info_msg[3], "</B>Serial Number:<!B>\t\t%-30s",
                mr_adapters[adp_choice]->serial);
        SAFE_ASPRINTF(&adp_info_msg[4], "</B>Firmware Version:<!B>\t%-30s",
                mr_adapters[adp_choice]->firmware);
        SAFE_ASPRINTF(&adp_info_msg[5], "</B>Memory:<!B>\t\t\t%-30s",
                mr_adapters[adp_choice]->memory);
        SAFE_ASPRINTF(&adp_info_msg[6], "</B>Battery:<!B>\t\t%-30s",
                mr_adapters[adp_choice]->bbu);
        SAFE_ASPRINTF(&adp_info_msg[7], "</B>Host Interface:<!B>\t\t%-30s",
                mr_adapters[adp_choice]->interface);
        SAFE_ASPRINTF(&adp_info_msg[8], "</B>Physical Disks:<!B>\t\t%-30d",
                mr_adapters[adp_choice]->disk_cnt);
        SAFE_ASPRINTF(&adp_info_msg[9], "</B>Logical Drives:<!B>\t\t%-30d",
                mr_adapters[adp_choice]->logical_drv_cnt);
        adapter_info = newCDKLabel(adapter_screen, (window_x + 1),
                (window_y + 1), adp_info_msg, ADP_PROP_INFO_LINES,
                FALSE, FALSE);
        if (!adapter_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(adapter_info, COLOR_DIALOG_TEXT);

        /* Field entry widgets */
        cache_flush = newCDKEntry(adapter_screen, (window_x + 1),
                (window_y + 12), NULL, "</B>Cache Flush Interval (0 to 255): ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
                FALSE, FALSE);
        if (!cache_flush) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(cache_flush, COLOR_DIALOG_INPUT);
        snprintf(temp_str, MAX_MR_ATTR_SIZE, "%d", mr_adp_props->cache_flush);
        setCDKEntryValue(cache_flush, temp_str);
        rebuild_rate = newCDKEntry(adapter_screen, (window_x + 1),
                (window_y + 13), NULL, "</B>Rebuild Rate (0 to 100): ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT, 3, 1, 3,
                FALSE, FALSE);
        if (!rebuild_rate) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(rebuild_rate, COLOR_DIALOG_INPUT);
        snprintf(temp_str, MAX_MR_ATTR_SIZE, "%d", mr_adp_props->rebuild_rate);
        setCDKEntryValue(rebuild_rate, temp_str);

        /* Radio lists */
        cluster_radio = newCDKRadio(adapter_screen, (window_x + 1),
                (window_y + 15), NONE, 3, 10, "</B>Cluster",
                g_dsbl_enbl_opts, 2, '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!cluster_radio) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(cluster_radio, COLOR_DIALOG_TEXT);
        setCDKRadioCurrentItem(cluster_radio, (int) mr_adp_props->cluster);
        ncq_radio = newCDKRadio(adapter_screen, (window_x + 20),
                (window_y + 15), NONE, 3, 10, "</B>NCQ", g_dsbl_enbl_opts, 2,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!ncq_radio) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(ncq_radio, COLOR_DIALOG_TEXT);
        setCDKRadioCurrentItem(ncq_radio, (int) mr_adp_props->ncq);

        /* Buttons */
        ok_button = newCDKButton(adapter_screen, (window_x + 20),
                (window_y + 19), g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
        cancel_button = newCDKButton(adapter_screen, (window_x + 30),
                (window_y + 19), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
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
                    break;
                } else {
                    mr_adp_props->cache_flush = temp_int;
                }
            }
            temp_int = atoi(getCDKEntryValue(rebuild_rate));
            if (temp_int != mr_adp_props->rebuild_rate) {
                if (temp_int < 0 || temp_int > 100) {
                    errorDialog(adapter_screen,
                            "Rebuild rate value must be 0 to 100.", NULL);
                    break;
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
                SAFE_ASPRINTF(&error_msg, "Couldn't update adapter properties; "
                        "MegaCLI exited with %d.", temp_int);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* Done */
    FREE_NULL(mr_adp_props);
    for (i = 0; i < ADP_PROP_INFO_LINES; i++)
        FREE_NULL(adp_info_msg[i]);
    for (i = 0; i < MAX_ADAPTERS; i++)
        FREE_NULL(mr_adapters[i]);
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
    int adp_count = 0, adp_choice = 0, i = 0, encl_count = 0, j = 0,
            line_pos = 0;
    char *encl_title = NULL;
    char *error_msg = NULL;
    char *swindow_info[MAX_ADP_INFO_LINES] = {NULL};

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    if ((adp_count = getMRAdapterCount()) == -1) {
        errorDialog(main_cdk_screen, "Error getting adapter count.", NULL);
        return;
    }

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
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't get data from MegaRAID enclosure # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
            for (j = 0; j < mr_enclosures[i]->slots; j++) {
                mr_disks[i][j] = getMRDisk(adp_choice,
                        mr_enclosures[i]->device_id, j);
                if (!mr_disks[i][j]) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't get disk information for "
                            "slot %d, enclosure %d!", j, i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
            }
        }
    }

    /* Setup scrolling window widget */
    SAFE_ASPRINTF(&encl_title, "<C></31/B>Enclosures/Slots on MegaRAID "
            "Adapter # %d\n", adp_choice);
    encl_swindow = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            (ADP_INFO_ROWS + 2), (ADP_INFO_COLS + 2), encl_title,
            MAX_ADP_INFO_LINES, TRUE, FALSE);
    if (!encl_swindow) {
        errorDialog(main_cdk_screen, SWINDOW_ERR_MSG, NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(encl_swindow, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(encl_swindow, COLOR_DIALOG_BOX);

    /* Add enclosure/disk information */
    line_pos = 0;
    for (i = 0; i < encl_count; i++) {
        if (line_pos < MAX_ADP_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos],
                    "</B>Enclosure %d (%s %s)",
                    i, mr_enclosures[i]->vendor, mr_enclosures[i]->product);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
        if (line_pos < MAX_ADP_INFO_LINES) {
            SAFE_ASPRINTF(&swindow_info[line_pos],
                    "</B>Device ID: %d, Status: %s, Slots: %d",
                    mr_enclosures[i]->device_id, mr_enclosures[i]->status,
                    mr_enclosures[i]->slots);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
        for (j = 0; (j < mr_enclosures[i]->slots) &&
                (line_pos < MAX_ADP_INFO_LINES); j++) {
            if (mr_disks[i][j]->present)
                SAFE_ASPRINTF(&swindow_info[line_pos],
                        "\tSlot %3d: %s", j, mr_disks[i][j]->inquiry);
            else
                SAFE_ASPRINTF(&swindow_info[line_pos], "\tSlot %3d: Not Present", j);
            addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
            line_pos++;
        }
    }

    /* Add a message to the bottom explaining how to close the dialog */
    if (line_pos < MAX_ADP_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], " ");
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        line_pos++;
    }
    if (line_pos < MAX_ADP_INFO_LINES) {
        SAFE_ASPRINTF(&swindow_info[line_pos], CONTINUE_MSG);
        addCDKSwindow(encl_swindow, swindow_info[line_pos], BOTTOM);
        line_pos++;
    }

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(encl_swindow, 'g');
    activateCDKSwindow(encl_swindow, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(encl_swindow);

    /* Done */
    FREE_NULL(encl_title);
    for (i = 0; i < MAX_ADP_INFO_LINES; i++ )
        FREE_NULL(swindow_info[i]);
    for (i = 0; i < MAX_ADAPTERS; i++)
        FREE_NULL(mr_adapters[i]);
    for (i = 0; i < MAX_ENCLOSURES; i++) {
        FREE_NULL(mr_enclosures[i]);
        for (j = 0; j < MAX_DISKS; j++) {
            FREE_NULL(mr_disks[i][j]);
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
    char new_ld_raid_lvl[MAX_MR_ATTR_SIZE] = {0},
            new_ld_strip_size[MAX_MR_ATTR_SIZE] = {0},
            pd_info_line_buffer[MAX_PD_INFO_LINE_BUFF] = {0};
    char *error_msg = NULL, *dsk_select_title = NULL, *temp_pstr = NULL;
    char *selection_list[MAX_DISKS] = {NULL},
            *new_ld_msg[NEW_LD_INFO_LINES] = {NULL};
    boolean finished = FALSE;

    /* Prompt for adapter choice */
    if ((adp_choice = getAdpChoice(main_cdk_screen, mr_adapters)) == -1) {
        return;
    }

    /* Get the number of adapters */
    if ((adp_count = getMRAdapterCount()) == -1) {
        errorDialog(main_cdk_screen, "Error getting adapter count.", NULL);
        return;
    }

    while (1) {
    /* Get enclosures / disks (slots) and fill selection list */
    encl_count = getMREnclCount(adp_choice);
    if (encl_count == -1) {
        errorDialog(main_cdk_screen, "Error getting enclosure count.", NULL);
        break;
    } else if (encl_count == 0) {
        errorDialog(main_cdk_screen, "No enclosures found!", NULL);
        break;
    } else {
        selection_size = 0;
        for (i = 0; i < encl_count; i++) {
            mr_enclosures[i] = getMREnclosure(adp_choice, i);
            if (!mr_enclosures[i]) {
                SAFE_ASPRINTF(&error_msg,
                        "Couldn't get data from MegaRAID enclosure # %d!", i);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                finished = TRUE;
                break;
            }
            for (j = 0; j < mr_enclosures[i]->slots; j++) {
                mr_disks[i][j] = getMRDisk(adp_choice,
                        mr_enclosures[i]->device_id, j);
                if (!mr_disks[i][j]) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't get disk information "
                            "for slot %d, enclosure %d!", j, i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    finished = TRUE;
                    break;
                } else {
                    if (mr_disks[i][j]->present == TRUE &&
                            mr_disks[i][j]->part_of_ld == FALSE) {
                        /* Fill selection list */
                        if (selection_size < MAX_DISKS) {
                            SAFE_ASPRINTF(&selection_list[selection_size],
                                    "<C>Enclosure %3.3d, Slot %3.3d: %40.40s",
                                    i, j, mr_disks[i][j]->inquiry);
                            disk_selection[selection_size] = mr_disks[i][j];
                            selection_size++;
                        }
                    }
                }
            }
            if (finished)
                break;
        }
        if (finished)
            break;
    }

    /* If we don't have any available disks, then display a message saying so
     * and return */
    if (selection_size == 0) {
        SAFE_ASPRINTF(&error_msg, "There are no available physical disks "
                "on adapter # %d.", adp_choice);
        errorDialog(main_cdk_screen, error_msg, NULL);
        FREE_NULL(error_msg);
        break;
    }

    /* Selection widget for disks */
    SAFE_ASPRINTF(&dsk_select_title,
            "<C></31/B>Select Disks for New LD (MegaRAID Adapter # %d)\n",
            adp_choice);
    disk_select = newCDKSelection(main_cdk_screen, CENTER, CENTER, NONE,
            18, 74, dsk_select_title, selection_list, selection_size,
            g_choice_char, 2, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!disk_select) {
        errorDialog(main_cdk_screen, SELECTION_ERR_MSG, NULL);
        break;
    }
    setCDKSelectionBoxAttribute(disk_select, COLOR_DIALOG_BOX);
    setCDKSelectionBackgroundAttrib(disk_select, COLOR_DIALOG_TEXT);

    /* Activate the widget */
    activateCDKSelection(disk_select, 0);

    /* User hit escape, so we get out of this */
    if (disk_select->exitType == vESCAPE_HIT) {
        destroyCDKSelection(disk_select);
        refreshCDKScreen(main_cdk_screen);
        break;

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
        break;
    }

    /* Setup a new CDK screen for required input (to create new LD) */
    new_ld_window_lines = 18;
    new_ld_window_cols = 70;
    window_y = ((LINES / 2) - (new_ld_window_lines / 2));
    window_x = ((COLS / 2) - (new_ld_window_cols / 2));
    new_ld_window = newwin(new_ld_window_lines, new_ld_window_cols,
            window_y, window_x);
    if (new_ld_window == NULL) {
        errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
        break;
    }
    new_ld_screen = initCDKScreen(new_ld_window);
    if (new_ld_screen == NULL) {
        errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
        break;
    }
    boxWindow(new_ld_window, COLOR_DIALOG_BOX);
    wbkgd(new_ld_window, COLOR_DIALOG_TEXT);
    wrefresh(new_ld_window);

    /* Put selected physical disk "enclosure:slot" information into a string */
    for (i = 0; i < chosen_disk_cnt; i++) {
        if (i == (chosen_disk_cnt - 1))
            SAFE_ASPRINTF(&temp_pstr, "[%d:%d]", chosen_disks[i]->enclosure_id,
                    chosen_disks[i]->slot_num);
        else
            SAFE_ASPRINTF(&temp_pstr, "[%d:%d], ", chosen_disks[i]->enclosure_id,
                chosen_disks[i]->slot_num);
        /* We add one extra for the null byte */
        pd_info_size = strlen(temp_pstr) + 1;
        pd_info_line_size = pd_info_line_size + pd_info_size;
        if (pd_info_line_size >= MAX_PD_INFO_LINE_BUFF) {
            errorDialog(main_cdk_screen, "The maximum PD info. line buffer "
                    "size has been reached!", NULL);
            FREE_NULL(temp_pstr);
            finished = TRUE;
            break;
        } else {
            strcat(pd_info_line_buffer, temp_pstr);
            FREE_NULL(temp_pstr);
        }
    }
    if (finished)
        break;

    /* Make a new label for the add-logical-drive screen */
    SAFE_ASPRINTF(&new_ld_msg[0],
            "</31/B>Creating new MegaRAID LD (on adapter # %d)...",
            adp_choice);
    /* Using asprintf() for a blank space makes it easier on clean-up (free) */
    SAFE_ASPRINTF(&new_ld_msg[1], " ");
    SAFE_ASPRINTF(&new_ld_msg[2], "Selected Disks [ENCL:SLOT] - %.35s",
            pd_info_line_buffer);
    new_ld_label = newCDKLabel(new_ld_screen, (window_x + 1), (window_y + 1),
            new_ld_msg, NEW_LD_INFO_LINES, FALSE, FALSE);
    if (!new_ld_label) {
        errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
        break;
    }
    setCDKLabelBackgroundAttrib(new_ld_label, COLOR_DIALOG_TEXT);

    /* RAID level radio list */
    raid_lvl = newCDKRadio(new_ld_screen, (window_x + 1), (window_y + 5),
            NONE, 5, 10, "</B>RAID Level", g_raid_opts, 4,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!raid_lvl) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(raid_lvl, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(raid_lvl, 0);

    /* Strip size radio list */
    strip_size = newCDKRadio(new_ld_screen, (window_x + 15), (window_y + 5),
            NONE, 9, 10, "</B>Strip Size", g_strip_opts, 8,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!strip_size) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(strip_size, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(strip_size, 3);

    /* Write cache radio list */
    write_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 5),
            NONE, 3, 11, "</B>Write Cache", g_write_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!write_cache) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(write_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(write_cache, 1);

    /* Read cache radio list */
    read_cache = newCDKRadio(new_ld_screen, (window_x + 30), (window_y + 9),
            NONE, 4, 10, "</B>Read Cache", g_read_opts, 3,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!read_cache) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(read_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(read_cache, 2);

    /* Cache policy radio list */
    cache_policy = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 5),
            NONE, 3, 12, "</B>Cache Policy", g_cache_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!cache_policy) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(cache_policy, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(cache_policy, 1);

    /* BBU cache policy radio list */
    bbu_cache = newCDKRadio(new_ld_screen, (window_x + 46), (window_y + 9),
            NONE, 3, 16, "</B>BBU Cache Policy", g_bbu_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!bbu_cache) {
        errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
        break;
    }
    setCDKRadioBackgroundAttrib(bbu_cache, COLOR_DIALOG_TEXT);
    setCDKRadioCurrentItem(bbu_cache, 1);

    /* Buttons */
    ok_button = newCDKButton(new_ld_screen, (window_x + 26), (window_y + 16),
            g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
        break;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(new_ld_screen, (window_x + 36),
            (window_y + 16), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
        break;
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
        strncpy(new_ld_props->cache_policy, g_cache_opts[temp_int],
                MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(write_cache);
        strncpy(new_ld_props->write_policy, g_write_opts[temp_int],
                MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(read_cache);
        strncpy(new_ld_props->read_policy, g_read_opts[temp_int],
                MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(bbu_cache);
        strncpy(new_ld_props->bbu_cache_policy, g_bbu_opts[temp_int],
                MAX_MR_ATTR_SIZE);

        /* RAID level and stripe size */
        // TODO: Should we check for a valid RAID level + disk combination?
        temp_int = getCDKRadioSelectedItem(raid_lvl);
        strncpy(new_ld_raid_lvl, g_raid_opts[temp_int], MAX_MR_ATTR_SIZE);
        temp_int = getCDKRadioSelectedItem(strip_size);
        strncpy(new_ld_strip_size, g_strip_opts[temp_int], MAX_MR_ATTR_SIZE);

        /* Create the new logical drive */
        temp_int = addMRLogicalDrive(new_ld_props, chosen_disk_cnt,
                chosen_disks, new_ld_raid_lvl, new_ld_strip_size);
        if (temp_int != 0) {
            SAFE_ASPRINTF(&error_msg, "Error creating new logical drive; "
                    "MegaCLI exited with %d.", temp_int);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }
    }
    break;
}

    /* Done -- free everything and clean up */
    FREE_NULL(new_ld_props);
    FREE_NULL(dsk_select_title);
    for (i = 0; i < MAX_DISKS; i++)
        FREE_NULL(selection_list[i]);
    for (i = 0; i < NEW_LD_INFO_LINES; i++)
        FREE_NULL(new_ld_msg[i]);
    for (i = 0; i < MAX_ADAPTERS; i++)
        FREE_NULL(mr_adapters[i]);
    for (i = 0; i < MAX_ENCLOSURES; i++) {
        FREE_NULL(mr_enclosures[i]);
        for (j = 0; j < MAX_DISKS; j++) {
            /* disk_selection and chosen_disks are not free'd
             * since mr_disks holds all possible and that one is free'd */
            FREE_NULL(mr_disks[i][j]);
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
    char *logical_drives[MAX_MR_LDS] = {NULL};
    char *error_msg = NULL, *confirm_msg = NULL;
    int mr_ld_ids[MAX_MR_LDS] = {0};
    int adp_count = 0, adp_choice = 0, i = 0, ld_count = 0,
            ld_choice = 0, temp_int = 0;
    boolean confirm = FALSE, finished = FALSE;

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    if ((adp_count = getMRAdapterCount()) == -1) {
        errorDialog(main_cdk_screen, "Error getting adapter count.", NULL);
        return;
    }

    while (1) {
        /* Get MegaRAID logical drives */
        ld_count = getMRLDCount(adp_choice);
        if (ld_count == 0) {
            errorDialog(main_cdk_screen, "No logical drives found!", NULL);
            break;
        } else if (ld_count == -1) {
            SAFE_ASPRINTF(&error_msg, "Error getting LD count for adapter # %d!",
                    adp_choice);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Get MegaRAID LD ID numbers */
        if (getMRLDIDNums(adp_choice, ld_count, mr_ld_ids) != 0) {
            errorDialog(main_cdk_screen,
                    "Couldn't get logical drive ID numbers!", NULL);
            break;
        } else {
            for (i = 0; i < ld_count; i++) {
                mr_ldrives[i] = getMRLogicalDrive(adp_choice, mr_ld_ids[i]);
                if (!mr_ldrives[i]) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't get data from "
                            "MegaRAID logical drive # %d!", i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    finished = TRUE;
                    break;
                } else {
                    SAFE_ASPRINTF(&logical_drives[i],
                            "<C>MegaRAID Virtual Drive # %d (on adapter %d)",
                            mr_ldrives[i]->ldrive_id, adp_choice);
                }
            }
            if (finished)
                break;
        }

        /* Get logical drive choice from user */
        ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 12, 50,
                "<C></31/B>Choose a Logical Drive\n", logical_drives, ld_count,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!ld_list) {
            errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(ld_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(ld_list, COLOR_DIALOG_TEXT);
        ld_choice = activateCDKScroll(ld_list, 0);

        /* Done with the scroll widget */
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
        if (ld_list->exitType == vESCAPE_HIT)
            break;

        /* Get a final confirmation from user before we delete */
        SAFE_ASPRINTF(&confirm_msg, "logical drive # %d on adapter %d?",
                mr_ldrives[ld_choice]->ldrive_id, adp_choice);
        confirm = confirmDialog(main_cdk_screen,
                "Are you sure you want to delete", confirm_msg);
        FREE_NULL(confirm_msg);
        if (confirm) {
            temp_int = delMRLogicalDrive(adp_choice,
                    mr_ldrives[ld_choice]->ldrive_id);
            if (temp_int != 0) {
                SAFE_ASPRINTF(&error_msg, "Error deleting logical drive; "
                        "MegaCLI exited with %d.", temp_int);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* Done */
    for (i = 0; i < MAX_ADAPTERS; i++)
        FREE_NULL(mr_adapters[i]);
    for (i = 0; i < MAX_MR_LDS; i++) {
        FREE_NULL(mr_ldrives[i]);
        FREE_NULL(logical_drives[i]);
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
    char pd_info_line_buffer[MAX_PD_INFO_LINE_BUFF] = {0};
    char *temp_pstr = NULL, *error_msg = NULL;
    char *logical_drives[MAX_MR_LDS] = {NULL},
            *ld_info_msg[LD_PROPS_INFO_LINES] = {NULL};
    boolean finished = FALSE;

    /* Prompt for adapter choice */
    adp_choice = getAdpChoice(main_cdk_screen, mr_adapters);
    if (adp_choice == -1) {
        return;
    }

    /* Get the number of adapters */
    if ((adp_count = getMRAdapterCount()) == -1) {
        errorDialog(main_cdk_screen, "Error getting adapter count.", NULL);
        return;
    }

    while (1) {
        /* Get MegaRAID logical drives */
        ld_count = getMRLDCount(adp_choice);
        if (ld_count == 0) {
            errorDialog(main_cdk_screen, "No logical drives found!", NULL);
            break;
        } else if (ld_count == -1) {
            SAFE_ASPRINTF(&error_msg, "Error getting LD count for adapter # %d!",
                    adp_choice);
            errorDialog(main_cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Get MegaRAID LD ID numbers */
        if (getMRLDIDNums(adp_choice, ld_count, mr_ld_ids) != 0) {
            errorDialog(main_cdk_screen,
                    "Couldn't get logical drive ID numbers!", NULL);
            break;
        } else {
            for (i = 0; i < ld_count; i++) {
                mr_ldrives[i] = getMRLogicalDrive(adp_choice, mr_ld_ids[i]);
                if (!mr_ldrives[i]) {
                    SAFE_ASPRINTF(&error_msg, "Couldn't get data from "
                            "MegaRAID logical drive # %d!", i);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    finished = TRUE;
                    break;
                } else {
                    SAFE_ASPRINTF(&logical_drives[i],
                            "<C>MegaRAID Virtual Drive # %d (on adapter %d)",
                            mr_ldrives[i]->ldrive_id, adp_choice);
                }
            }
            if (finished)
                break;
        }

        /* Get logical drive choice from user */
        ld_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 12, 50,
                "<C></31/B>Choose a Logical Drive\n", logical_drives, ld_count,
                FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
        if (!ld_list) {
            errorDialog(main_cdk_screen, SCROLL_ERR_MSG, NULL);
            break;
        }
        setCDKScrollBoxAttribute(ld_list, COLOR_DIALOG_BOX);
        setCDKScrollBackgroundAttrib(ld_list, COLOR_DIALOG_TEXT);
        ld_choice = activateCDKScroll(ld_list, 0);

        /* Clean up the screen */
        destroyCDKScroll(ld_list);
        refreshCDKScreen(main_cdk_screen);
        if (ld_list->exitType == vESCAPE_HIT)
            break;

        /* Get the logical drive properties */
        mr_ld_props = getMRLDProps(adp_choice,
                mr_ldrives[ld_choice]->ldrive_id);
        if (!mr_ld_props) {
            errorDialog(main_cdk_screen,
                    "Error getting logical drive properties.", NULL);
            break;
        }

        /* New CDK screen for selected LD */
        ld_window_lines = 20;
        ld_window_cols = 70;
        window_y = ((LINES / 2) - (ld_window_lines / 2));
        window_x = ((COLS / 2) - (ld_window_cols / 2));
        ld_window = newwin(ld_window_lines, ld_window_cols, window_y, window_x);
        if (ld_window == NULL) {
            errorDialog(main_cdk_screen, NEWWIN_ERR_MSG, NULL);
            break;
        }
        ld_screen = initCDKScreen(ld_window);
        if (ld_screen == NULL) {
            errorDialog(main_cdk_screen, CDK_SCR_ERR_MSG, NULL);
            break;
        }
        boxWindow(ld_window, COLOR_DIALOG_BOX);
        wbkgd(ld_window, COLOR_DIALOG_TEXT);
        wrefresh(ld_window);

        /* Get enclosure/slot (disk) information for selected LD */
        getMRLDDisks(adp_choice, mr_ldrives[ld_choice]->ldrive_id, ld_encl_ids,
                ld_slots);
        for (i = 0; i < mr_ldrives[ld_choice]->drive_cnt; i++) {
            if (i == (mr_ldrives[ld_choice]->drive_cnt - 1))
                SAFE_ASPRINTF(&temp_pstr, "[%d:%d]", ld_encl_ids[i], ld_slots[i]);
            else
                SAFE_ASPRINTF(&temp_pstr, "[%d:%d], ", ld_encl_ids[i], ld_slots[i]);
            /* We add one extra for the null byte */
            pd_info_size = strlen(temp_pstr) + 1;
            pd_info_line_size = pd_info_line_size + pd_info_size;
            if (pd_info_line_size >= MAX_PD_INFO_LINE_BUFF) {
                errorDialog(main_cdk_screen, "The maximum PD info. line "
                        "buffer size has been reached!", NULL);
                FREE_NULL(temp_pstr);
                finished = TRUE;
                break;
            } else {
                strcat(pd_info_line_buffer, temp_pstr);
                FREE_NULL(temp_pstr);
            }
        }
        if (finished)
            break;

        /* Logical drive info. label */
        SAFE_ASPRINTF(&ld_info_msg[0], "</31/B>Properties for MegaRAID LD # "
                "%d (on adapter # %d)...",
                mr_ldrives[ld_choice]->ldrive_id, adp_choice);
        SAFE_ASPRINTF(&ld_info_msg[1], " ");
        SAFE_ASPRINTF(&ld_info_msg[2], "</B>RAID Level:<!B>\t%s",
                mr_ldrives[ld_choice]->raid_lvl);
        SAFE_ASPRINTF(&ld_info_msg[3], "</B>Size:<!B>\t\t%s",
                mr_ldrives[ld_choice]->size);
        SAFE_ASPRINTF(&ld_info_msg[4], "</B>State:<!B>\t\t%s",
                mr_ldrives[ld_choice]->state);
        SAFE_ASPRINTF(&ld_info_msg[5], "</B>Strip Size:<!B>\t%s",
                mr_ldrives[ld_choice]->strip_size);
        SAFE_ASPRINTF(&ld_info_msg[6], "</B>Drive Count:<!B>\t%d",
                mr_ldrives[ld_choice]->drive_cnt);
        SAFE_ASPRINTF(&ld_info_msg[7], " ");
        SAFE_ASPRINTF(&ld_info_msg[8], "Disks [ENCL:SLOT] - %.60s",
                pd_info_line_buffer);
        ld_info = newCDKLabel(ld_screen, (window_x + 1), (window_y + 1),
                ld_info_msg, LD_PROPS_INFO_LINES, FALSE, FALSE);
        if (!ld_info) {
            errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
            break;
        }
        setCDKLabelBackgroundAttrib(ld_info, COLOR_DIALOG_TEXT);

        /* Name field */
        name_field = newCDKEntry(ld_screen, (window_x + 1), (window_y + 11),
                NULL, "</B>Logical drive name (no spaces): ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vUMIXED,
                MAX_LD_NAME, 0, MAX_LD_NAME, FALSE, FALSE);
        if (!name_field) {
            errorDialog(main_cdk_screen, ENTRY_ERR_MSG, NULL);
            break;
        }
        setCDKEntryBoxAttribute(name_field, COLOR_DIALOG_INPUT);
        setCDKEntryValue(name_field, mr_ld_props->name);

        /* Radio lists */
        cache_policy = newCDKRadio(ld_screen, (window_x + 1), (window_y + 13),
                NONE, 3, 10, "</B>Cache Policy", g_cache_opts, 2,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!cache_policy) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(cache_policy, COLOR_DIALOG_TEXT);
        if (strcmp(mr_ld_props->cache_policy, g_cache_opts[0]) == 0)
            setCDKRadioCurrentItem(cache_policy, 0);
        else if (strcmp(mr_ld_props->cache_policy, g_cache_opts[1]) == 0)
            setCDKRadioCurrentItem(cache_policy, 1);

        write_cache = newCDKRadio(ld_screen, (window_x + 16), (window_y + 13),
                NONE, 3, 10, "</B>Write Cache", g_write_opts, 2,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!write_cache) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(write_cache, COLOR_DIALOG_TEXT);
        if (strcmp(mr_ld_props->write_policy, g_write_opts[0]) == 0)
            setCDKRadioCurrentItem(write_cache, 0);
        else if (strcmp(mr_ld_props->write_policy, g_write_opts[1]) == 0)
            setCDKRadioCurrentItem(write_cache, 1);

        read_cache = newCDKRadio(ld_screen, (window_x + 30), (window_y + 13),
                NONE, 4, 10, "</B>Read Cache", g_read_opts, 3,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!read_cache) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(read_cache, COLOR_DIALOG_TEXT);
        if (strcmp(mr_ld_props->read_policy, g_read_opts[0]) == 0)
            setCDKRadioCurrentItem(read_cache, 0);
        else if (strcmp(mr_ld_props->read_policy, g_read_opts[1]) == 0)
            setCDKRadioCurrentItem(read_cache, 1);
        else if (strcmp(mr_ld_props->read_policy, g_read_opts[2]) == 0)
            setCDKRadioCurrentItem(read_cache, 2);

        bbu_cache = newCDKRadio(ld_screen, (window_x + 45), (window_y + 13),
                NONE, 3, 12, "</B>BBU Cache Policy", g_bbu_opts, 2,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!bbu_cache) {
            errorDialog(main_cdk_screen, RADIO_ERR_MSG, NULL);
            break;
        }
        setCDKRadioBackgroundAttrib(bbu_cache, COLOR_DIALOG_TEXT);
        if (strcmp(mr_ld_props->bbu_cache_policy, g_bbu_opts[0]) == 0)
            setCDKRadioCurrentItem(bbu_cache, 0);
        else if (strcmp(mr_ld_props->bbu_cache_policy, g_bbu_opts[1]) == 0)
            setCDKRadioCurrentItem(bbu_cache, 1);

        /* Buttons */
        ok_button = newCDKButton(ld_screen, (window_x + 26), (window_y + 18),
                g_ok_cancel_msg[0], ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
        }
        setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
        cancel_button = newCDKButton(ld_screen, (window_x + 36),
                (window_y + 18), g_ok_cancel_msg[1], cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, BUTTON_ERR_MSG, NULL);
            break;
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
            if (!checkInputStr(main_cdk_screen, NAME_CHARS,
                    getCDKEntryValue(name_field)))
                break;
            strncpy(mr_ld_props->name, getCDKEntryValue(name_field),
                    MAX_MR_ATTR_SIZE);

            /* Set radio inputs */
            temp_int = getCDKRadioSelectedItem(cache_policy);
            strncpy(mr_ld_props->cache_policy, g_cache_opts[temp_int],
                    MAX_MR_ATTR_SIZE);
            temp_int = getCDKRadioSelectedItem(write_cache);
            strncpy(mr_ld_props->write_policy, g_write_opts[temp_int],
                    MAX_MR_ATTR_SIZE);
            temp_int = getCDKRadioSelectedItem(read_cache);
            strncpy(mr_ld_props->read_policy, g_read_opts[temp_int],
                    MAX_MR_ATTR_SIZE);
            temp_int = getCDKRadioSelectedItem(bbu_cache);
            strncpy(mr_ld_props->bbu_cache_policy, g_bbu_opts[temp_int],
                    MAX_MR_ATTR_SIZE);

            /* Set logical drive properties */
            temp_int = setMRLDProps(mr_ld_props);
            if (temp_int != 0) {
                SAFE_ASPRINTF(&error_msg, "Couldn't update logical drive "
                        "properties; MegaCLI exited with %d.", temp_int);
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
            }
        }
        break;
    }

    /* Done -- free everything and clean up */
    FREE_NULL(mr_ld_props);
    for (i = 0; i < MAX_ADAPTERS; i++)
        FREE_NULL(mr_adapters[i]);
    for (i = 0; i < MAX_MR_LDS; i++) {
        FREE_NULL(mr_ldrives[i]);
        FREE_NULL(logical_drives[i]);
    }
    for (i = 0; i < LD_PROPS_INFO_LINES; i++)
        FREE_NULL(ld_info_msg[i]);
    if (ld_screen != NULL) {
        destroyCDKScreenObjects(ld_screen);
        destroyCDKScreen(ld_screen);
    }
    delwin(ld_window);
    return;
}
