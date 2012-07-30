/*
 * $Id: label_data.c 139 2012-07-24 20:17:34Z marc.smith $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "label_data.h"

/*
 * Grab the sysfs data for the adapters label
 */
void readAdapterData(char *label_msg[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int row_cnt = 0, i = 0;
    char line_buffer[ADAPTERS_LABEL_COLS];
    char sysfs_path[MAX_SYSFS_PATH_SIZE] = {0},
            fc_model[MAX_SYSFS_ATTR_SIZE] = {0},
            fc_fw_ver[MAX_SYSFS_ATTR_SIZE] = {0},
            fc_port_wwn[MAX_SYSFS_ATTR_SIZE] = {0},
            fc_link[MAX_SYSFS_ATTR_SIZE] = {0},
            fc_speed[MAX_SYSFS_ATTR_SIZE] = {0},
            ib_node_guid[MAX_SYSFS_ATTR_SIZE] = {0},
            ib_node_desc[MAX_SYSFS_ATTR_SIZE] = {0},
            ib_node_type[MAX_SYSFS_ATTR_SIZE] = {0};
    
    /* Clear the label message; we purposely start at 1 so we skip the title */
    for (i = 1; i < ADAPTERS_LABEL_ROWS; i++) {
        freeChar(label_msg[i]);
        label_msg[i] = NULL;
    }
    
    /* We start our row 2 down (1 for the title, 1 for spacing) */
    row_cnt = 2;

    /* Fibre Channel HBA information */
    if ((dir_stream = opendir(SYSFS_FC_HOST)) == NULL) {
        if (row_cnt < ADAPTERS_LABEL_ROWS) {
            snprintf(line_buffer, ADAPTERS_LABEL_COLS, "opendir: %s", strerror(errno));
            asprintf(&label_msg[row_cnt], "%s", line_buffer);
        }
        return;
    }
    i = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* We want to skip the '.' and '..' directories */
        if (i > 1) {
            /* Get the HBA model name */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/device/scsi_host/%s/model_name",
                    SYSFS_FC_HOST, dir_entry->d_name, dir_entry->d_name);
            readAttribute(sysfs_path, fc_model);

            /* Get the firmware version */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/device/scsi_host/%s/fw_version",
                    SYSFS_FC_HOST, dir_entry->d_name, dir_entry->d_name);
            readAttribute(sysfs_path, fc_fw_ver);

            /* Get the port WWN */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/port_name",
                    SYSFS_FC_HOST, dir_entry->d_name);
            readAttribute(sysfs_path, fc_port_wwn);

            /* Get link state */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/device/scsi_host/%s/link_state",
                    SYSFS_FC_HOST, dir_entry->d_name, dir_entry->d_name);
            readAttribute(sysfs_path, fc_link);

            /* Get speed (if any) */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/speed",
                    SYSFS_FC_HOST, dir_entry->d_name);
            readAttribute(sysfs_path, fc_speed);

            /* Fill the label message */
            if (row_cnt < ADAPTERS_LABEL_ROWS) {
                snprintf(line_buffer, ADAPTERS_LABEL_COLS, "%s %s %.8s- %s - %s", fc_port_wwn,
                        fc_model, fc_fw_ver, fc_link, fc_speed);
                asprintf(&label_msg[row_cnt], "%s", line_buffer);
                row_cnt++;
            }
        }
        i++;
    }
    closedir(dir_stream);

    /* InfiniBand HCA information */
    if ((dir_stream = opendir(SYSFS_INFINIBAND)) == NULL) {
        if (row_cnt < ADAPTERS_LABEL_ROWS) {
            snprintf(line_buffer, ADAPTERS_LABEL_COLS, "opendir: %s", strerror(errno));
            asprintf(&label_msg[row_cnt], "%s", line_buffer);
        }
        return;
    }
    i = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* We want to skip the '.' and '..' directories */
        if (i > 1) {
            /* Get the HCA node GUID */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/node_guid",
                    SYSFS_INFINIBAND, dir_entry->d_name);
            readAttribute(sysfs_path, ib_node_guid);

            /* Get the node description */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/node_desc",
                    SYSFS_INFINIBAND, dir_entry->d_name);
            readAttribute(sysfs_path, ib_node_desc);

            /* Get the node type */
            snprintf(sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/%s/node_type",
                    SYSFS_INFINIBAND, dir_entry->d_name);
            readAttribute(sysfs_path, ib_node_type);

            /* Fill the label message */
            if (row_cnt < ADAPTERS_LABEL_ROWS) {
                snprintf(line_buffer, ADAPTERS_LABEL_COLS, "%s %s - Type: %s",
                        ib_node_guid, ib_node_desc, ib_node_type);
                asprintf(&label_msg[row_cnt], "%s", line_buffer);
                row_cnt++;
            }
        }
        i++;
    }
    closedir(dir_stream);

    /* Done */
    return;
}


/*
 * Grab the sysfs data for the devices label
 */
void readDeviceData(char *label_msg[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int i = 0, row_cnt = 0;
    char line_buffer[DEVICES_LABEL_COLS];
    static char *handlers[] = {"dev_disk", "dev_disk_perf", "vcdrom",
    "vdisk_blockio", "vdisk_fileio", "vdisk_nullio"};
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    
    /* Clear the label message; we purposely start at 1 so we skip the title */
    for (i = 1; i < DEVICES_LABEL_ROWS; i++) {
        freeChar(label_msg[i]);
        label_msg[i] = NULL;
    }
    
    /* We start our row 2 down (1 for the title, 1 for spacing) */
    row_cnt = 2;

    /* Loop over each SCST handler type and grab any open device names */
    for (i = 0; i < 6; i++) {
        /* Open the directory */
        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/handlers/%s",
                SYSFS_SCST_TGT, handlers[i]);
        if ((dir_stream = opendir(dir_name)) == NULL) {
            if (row_cnt < DEVICES_LABEL_ROWS) {
                snprintf(line_buffer, DEVICES_LABEL_COLS, "opendir: %s", strerror(errno));
                asprintf(&label_msg[row_cnt], "%s", line_buffer);
            }
            return;
        }

        /* Loop over each entry in the directory */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK) {
                if (row_cnt < DEVICES_LABEL_ROWS) {
                    snprintf(line_buffer, DEVICES_LABEL_COLS, "%-15s (%s)",
                            dir_entry->d_name, handlers[i]);
                    asprintf(&label_msg[row_cnt], "%s", line_buffer);
                    row_cnt++;
                }
            }
        }

        /* Close the directory stream */
        closedir(dir_stream);
    }

    /* Done */
    return;
}


/*
 * Grab the sysfs data for the targets label
 */
void readTargetData(char *label_msg[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int row_cnt = 0, i = 0;
    char line_buffer[TARGETS_LABEL_COLS];
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Clear the label message; we purposely start at 1 so we skip the title */
    for (i = 1; i < TARGETS_LABEL_ROWS; i++) {
        freeChar(label_msg[i]);
        label_msg[i] = NULL;
    }
    
    /* We start our row 2 down (1 for the title, 1 for spacing) */
    row_cnt = 2;

    /* Open the iSCSI targets directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets/iscsi", SYSFS_SCST_TGT);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        if (row_cnt < TARGETS_LABEL_ROWS) {
            snprintf(line_buffer, TARGETS_LABEL_COLS, "opendir: %s", strerror(errno));
            asprintf(&label_msg[row_cnt], "%s", line_buffer);
        }
        return;
    }

    /* Loop over each entry in the directory */
    i = 0;
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        if (dir_entry->d_type == DT_DIR) {
            /* We want to skip the '.' and '..' directories */
            if ((row_cnt < TARGETS_LABEL_ROWS) && (i > 1)) {
                snprintf(line_buffer, TARGETS_LABEL_COLS, "%s", dir_entry->d_name);
                asprintf(&label_msg[row_cnt], "%s", line_buffer);
                row_cnt++;
            }
            i++;
        }
    }

    /* Close the directory stream */
    closedir(dir_stream);

    /* Done */
    return;
}