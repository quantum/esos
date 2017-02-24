/**
 * @file utility.c
 * @brief Contains common utility functions used by other functions.
 * @author Copyright (c) 2012-2017 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <syslog.h>
#include <inttypes.h>
#include <netdb.h>
#include <cdk.h>
#include <blkid/blkid.h>
#include <assert.h>
#include <fcntl.h>

#include "prototypes.h"
#include "system.h"


/**
 * @brief Strip whitespace and trailing newline from a string. Originally
 * from Linux kernel lib/string.c (strim()).
 */
char *strStrip(char *string) {
    size_t size = 0;
    char *end = NULL;
    while (isspace(*string))
        ++string;
    size = strlen(string);
    if (!size)
        return string;
    end = string + size - 1;
    while (end >= string && isspace(*end))
        end--;
    *(end + 1) = '\0';
    return string;
}


/**
 * @brief Reads a sysfs attribute value. Open the specified file and read its
 * contents. If an error occurs, fill the character array with the error.
 */
void readAttribute(char sysfs_attr[], char attr_value[]) {
     FILE *sysfs_file = NULL;
     char *remove_me = NULL;

     /* Open the file and retrieve the value */
     if ((sysfs_file = fopen(sysfs_attr, "r")) == NULL) {
         snprintf(attr_value, MAX_SYSFS_ATTR_SIZE,
                 "fopen(): %s", strerror(errno));
         return;
     } else {
         fgets(attr_value, MAX_SYSFS_ATTR_SIZE, sysfs_file);
         fclose(sysfs_file);
     }

     /* Get rid of the newline character */
     remove_me = strrchr(attr_value, '\n');
     if (remove_me) {
         *remove_me = '\0';
     }

     /* Done */
     return;
}


/**
 * @brief Write a sysfs attribute value. If an error is encountered, we return
 * the errno value, otherwise we return 0 (zero).
 */
int writeAttribute(char sysfs_attr[], char attr_value[]) {
     FILE *sysfs_file = NULL;
     int ret_val = 0;

     /* Open the file and write the value */
     if ((sysfs_file = fopen(sysfs_attr, "w")) == NULL) {
         return errno;
     } else {
         fprintf(sysfs_file, "%s", attr_value);
         if ((ret_val = fclose(sysfs_file)) == 0) {
             return ret_val;
         } else {
             return errno;
         }
     }
}


/**
 * @brief Test if SCST is loaded on the current machine. For now we have a very
 * simple test using the SCST sysfs directory; return TRUE (1) if the
 * directory exists and return FALSE (0) if it doesn't.
 */
boolean isSCSTLoaded() {
    struct stat scst_test = {0};
    if (stat(SYSFS_SCST_TGT, &scst_test) == 0)
        return TRUE;
    else
        return FALSE;
}


/**
 * @brief Check if the given initiator name is already in use in the group name
 * specified. Return TRUE if it does exist, and FALSE if it does not. If any
 * errors occur in this function, we return FALSE.
 */
boolean isSCSTInitInGroup(char tgt_name[], char tgt_driver[],
        char group_name[], char init_name[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    boolean found_init = FALSE;

    /* Open the group initiator directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/targets/%s/%s/ini_groups/%s/initiators",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, group_name);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        DEBUG_LOG("opendir(): %s", strerror(errno));
        return FALSE;
    }

    /* Loop over each entry in the directory */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The initiators are files */
        if ((dir_entry->d_type == DT_REG) &&
                (strcmp(dir_entry->d_name, init_name) == 0)) {
            found_init = TRUE;
            break;
        }
    }

    /* Done */
    closedir(dir_stream);
    return found_init;
}


/**
 * @brief Walk the group directories in sysfs for the specified target and count
 * the number of times the initiator is used. Return -1 if an error occurs.
 */
int countSCSTInitUses(char tgt_name[], char tgt_driver[], char init_name[]) {
    DIR *grp_dir_stream = NULL, *init_dir_stream = NULL;
    struct dirent *grp_dir_entry = NULL, *init_dir_entry = NULL;
    char groups_dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            inits_dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    int use_count = 0;

    /* Open the target group's directory (to get a list) */
    snprintf(groups_dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/targets/%s/%s/ini_groups",
            SYSFS_SCST_TGT, tgt_driver, tgt_name);
    if ((grp_dir_stream = opendir(groups_dir_name)) == NULL) {
        DEBUG_LOG("opendir(): %s", strerror(errno));
        return -1;
    }

    /* Loop over each entry in the directory */
    while ((grp_dir_entry = readdir(grp_dir_stream)) != NULL) {
        /* The groups are directories; skip '.' and '..' */
        if ((grp_dir_entry->d_type == DT_DIR) &&
                (strcmp(grp_dir_entry->d_name, ".") != 0) &&
                (strcmp(grp_dir_entry->d_name, "..") != 0)) {
            /* Now for each group, read through their initiators */
            snprintf(inits_dir_name, MAX_SYSFS_PATH_SIZE,
                    "%s/targets/%s/%s/ini_groups/%s/initiators",
                    SYSFS_SCST_TGT, tgt_driver, tgt_name,
                    grp_dir_entry->d_name);
            if ((init_dir_stream = opendir(inits_dir_name)) == NULL) {
                DEBUG_LOG("opendir(): %s", strerror(errno));
                closedir(grp_dir_stream);
                return -1;
            }
            /* Loop over each entry in the directory */
            while ((init_dir_entry = readdir(init_dir_stream)) != NULL) {
                /* The initiators are files */
                if ((init_dir_entry->d_type == DT_REG) &&
                        (strcmp(init_dir_entry->d_name, init_name) == 0)) {
                    use_count++;
                }
            }
            closedir(init_dir_stream);

        }
    }

    /* Done */
    closedir(grp_dir_stream);
    return use_count;
}


/**
 * @brief Fill the SCST target drivers list and set driver count for all target
 * drivers detected in the sysfs structure. If something fails internally in
 * the function, return FALSE, otherwise return TRUE.
 */
boolean listSCSTTgtDrivers(char tgt_drivers[][MISC_STRING_LEN],
        int *driver_cnt) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int i = 0;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory for target drivers */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets", SYSFS_SCST_TGT);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        return FALSE;
    }

    /* Loop over each directory and add it to the list */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The driver names are directories */
        if ((dir_entry->d_type == DT_DIR) &&
                (strcmp(dir_entry->d_name, ".") != 0) &&
                (strcmp(dir_entry->d_name, "..") != 0)) {
            /* Skip the copy_manager driver */
            if (strcmp(dir_entry->d_name, "copy_manager") == 0)
                continue;
            if (i < MAX_SCST_DRIVERS) {
                snprintf(tgt_drivers[i], MISC_STRING_LEN, dir_entry->d_name);
                i++;
            }
        }
    }

    /* Close the driver directory stream */
    closedir(dir_stream);

    /* Done */
    *driver_cnt = i;
    return TRUE;
}


/**
 * @brief Count the number of LUNs for a session with the given target, driver,
 * and initiator combination. Return -1 if an error occurs.
 */
int countSCSTSessLUNs(char tgt_name[], char tgt_driver[], char init_name[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    int lun_count = 0;

    /* Open the session's "luns" directory (to get a count) */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/targets/%s/%s/sessions/%s/luns",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, init_name);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        DEBUG_LOG("opendir(): %s", strerror(errno));
        return -1;
    }

    /* Loop over each entry in the directory */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The LUNs are directories; skip '.' and '..' */
        if ((dir_entry->d_type == DT_DIR) &&
                    (strcmp(dir_entry->d_name, ".") != 0) &&
                    (strcmp(dir_entry->d_name, "..") != 0)) {
            lun_count++;
        }
    }

    /* Done */
    closedir(dir_stream);
    return lun_count;
}


/**
 * @brief This function takes a number of bytes and formats/converts the value
 * for a "human-readable" representation of the number (eg, 2048 returns
 * "2 KiB").
 * Originally taken from here: http://stackoverflow.com/questions/3898840
 */
char *prettyFormatBytes(uint64_t size) {
    static const char *sizes[] = {"EiB", "PiB", "TiB",
    "GiB", "MiB", "KiB", "B"};
    uint64_t multiplier = (1024ULL * 1024ULL * 1024ULL *
    1024ULL * 1024ULL * 1024ULL);
    int i = 0;
    char *result = (char *) malloc(sizeof (char) * 20);

    for (i = 0; i < (int)(sizeof (sizes) / sizeof (*(sizes)));
            i++, multiplier /= 1024) {
        if (size < multiplier)
            continue;
        if ((size % multiplier) == 0)
            sprintf(result, "%" PRIu64 " %s", size / multiplier, sizes[i]);
        else
            sprintf(result, "%.1f %s", (float) size / multiplier, sizes[i]);
        return result;
    }
    strcpy(result, "0");
    return result;
}


/**
 * @brief Test and see if the Internet is reachable; we do this by attempting
 * to resolve a DNS entry. The GLIBC getaddrinfo_a() function is used so we
 * can timeout if the query takes too long. We simply assume the Internet
 * access works if we're able to successfully resolve the entry.
 */
boolean checkInetAccess() {
    struct gaicb **requests = NULL;
    struct timespec timeout = {0};
    int ret_val = 0, loop_cnt = 0;
    boolean has_inet = FALSE;

    /* Setup our request */
    requests = realloc(requests, (1 * sizeof requests[0]));
    requests[0] = calloc(1, sizeof *requests[0]);
    requests[0]->ar_name = INET_TEST_HOST;

    /* Queue the request */
    ret_val = getaddrinfo_a(GAI_NOWAIT, &requests[0], 1, NULL);
    if (ret_val != 0) {
        DEBUG_LOG("getaddrinfo_a(): %s", gai_strerror(ret_val));
        return FALSE;
    }

    /* Wait for the request to complete, or hit the timeout */
    timeout.tv_nsec = 250000000;
    loop_cnt = 1;
    while (1) {
        /* Don't wait too long */
        if (loop_cnt >= 10) {
            DEBUG_LOG("Timeout value reached, returning...");
            has_inet = FALSE;
            break;
        }
        ++loop_cnt;
        /* Check the request */
        ret_val = gai_error(requests[0]);
        if (ret_val == EAI_INPROGRESS) {
            ;
        } else if (ret_val == 0) {
            has_inet = TRUE;
            break;
        } else {
            DEBUG_LOG("gai_error(): %s", gai_strerror(ret_val));
            has_inet = FALSE;
            break;
        }
        /* Sleep for a bit */
        if (nanosleep(&timeout, NULL) != 0) {
            DEBUG_LOG("nanosleep(): %s", strerror(errno));
            has_inet = FALSE;
            break;
        }
    }

    /* Done */
    //freeaddrinfo(requests[0]->ar_request);
    freeaddrinfo(requests[0]->ar_result);
    free(requests);
    return has_inet;
}


/**
 * @brief The utility function for strings takes a "maximum string size"
 * argument and a string argument; if the length of the given string is less
 * than the maximum, return the string as is. If the string is longer, then
 * remove enough of the string from the center to accommodate the maximum size
 * and include several periods to show the string was abbreviated.
 */
char *prettyShrinkStr(size_t max_len, char *string) {
    size_t curr_size = 0, char_to_del = 0, left_len = 0, right_len = 0;
    char *right_half = NULL, *curr_pos = NULL;
    char str_buff[MISC_STRING_LEN] = {0};
    static char ret_buff[MAX_SYSFS_ATTR_SIZE] = {0};
    int i = 0;

    /* Since ret_buff is re-used between calls, we reset the first character */
    ret_buff[0] = '\0';

    /* Maybe it is already an acceptable length */
    curr_size = strlen(string);
    if (curr_size <= max_len) {
        strncpy(ret_buff, string, MAX_SYSFS_ATTR_SIZE);
        return ret_buff;

    } else {
        /* Guess not, so lets shrink it */
        strncpy(ret_buff, string, MAX_SYSFS_ATTR_SIZE);
        char_to_del = curr_size - max_len + 3;
        left_len = right_len = (curr_size - char_to_del) / 2;
        while ((left_len + char_to_del + right_len) < max_len)
            ++right_len;
        right_half = ret_buff + left_len + char_to_del;
        strncpy(str_buff, right_half, MISC_STRING_LEN);
        curr_pos = ret_buff + left_len;
        *curr_pos = '.';
        ++curr_pos;
        *curr_pos = '.';
        ++curr_pos;
        *curr_pos = '.';
        ++curr_pos;
        for (i = 0; i < MISC_STRING_LEN; i++) {
            if (str_buff[i] == '\0') {
                break;
            } else {
                *curr_pos = str_buff[i];
                ++curr_pos;
            }
        }
        *curr_pos = '\0';
        return ret_buff;
    }
}


/**
 * @brief Get all of the "usable" (eg, not the ESOS boot device, and other
 * block devices that appear to be in use) on the system, and fill the arrays
 * (by reference) with that information. We return the number of block devices
 * found (even zero) or -1 if an error occurred. This function will display
 * any error information to the screen itself.
 */
int getUsableBlockDevs(CDKSCREEN *cdk_screen,
        char blk_dev_name[MAX_BLOCK_DEVS][MISC_STRING_LEN],
        char blk_dev_info[MAX_BLOCK_DEVS][MISC_STRING_LEN],
        char blk_dev_size[MAX_BLOCK_DEVS][MISC_STRING_LEN]) {
    int dev_cnt = 0, blk_dev_fd = 0;
    char *error_msg = NULL, *boot_dev_node = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            tmp_buff[MAX_SYSFS_ATTR_SIZE] = {0},
            dev_node_test[MISC_STRING_LEN] = {0};
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    boolean finished = FALSE;

    while (1) {
        /* Get the ESOS boot device node */
        if ((boot_dev_node = blkid_get_devname(NULL, "LABEL",
                ESOS_ROOT_PART)) == NULL) {
            /* The function above returns NULL if the device isn't found */
            SAFE_ASPRINTF(&boot_dev_node, " ");
        } else {
            /* Found the device so chop off the partition number */
            *(boot_dev_node + strlen(boot_dev_node) - 1) = '\0';
        }

        /* Open the directory to get block devices */
        if ((dir_stream = opendir(SYSFS_BLOCK)) == NULL) {
            SAFE_ASPRINTF(&error_msg, "opendir(): %s", strerror(errno));
            errorDialog(cdk_screen, error_msg, NULL);
            FREE_NULL(error_msg);
            break;
        }

        /* Loop over each entry in the directory (block devices) */
        while ((dir_entry = readdir(dir_stream)) != NULL) {
            if (dir_entry->d_type == DT_LNK) {
                snprintf(dev_node_test, MISC_STRING_LEN,
                        "/dev/%s", dir_entry->d_name);
                /* Test to see if the block device is already open */
                if ((blk_dev_fd = open(dev_node_test, O_EXCL)) == -1) {
                    continue;
                } else {
                    if (close(blk_dev_fd) == -1) {
                        SAFE_ASPRINTF(&error_msg, "close(): %s",
                                strerror(errno));
                        errorDialog(cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        finished = TRUE;
                        break;
                    }
                }

                if (strcmp(boot_dev_node, dev_node_test) == 0) {
                    /* We don't want to show the ESOS boot block
                     * device (USB drive) */
                    continue;

                } else if ((strstr(dev_node_test, "/dev/drbd")) != NULL) {
                    /* For DRBD block devices (not sure if the /dev/drbdX
                     * format is forced when using drbdadm, so this may
                     * be a problem */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        /* Nothing extra for DRBD... yet */
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "DRBD Device");
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/md")) != NULL) {
                    /* For software RAID (md) devices; it appears the mdadm
                     * tool forces the /dev/mdX device node name format */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/md/level", SYSFS_BLOCK,
                                blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Level: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/sd")) != NULL) {
                    /* For normal SCSI block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/device/model",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Model: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/dm-")) != NULL) {
                    /* For device mapper (eg, LVM2) block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/dm/name",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Name: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/cciss")) != NULL) {
                    /* For Compaq SMART array controllers */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/device/raid_level",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "RAID Level: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/zd")) != NULL) {
                    /* For ZFS block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], "%s", MISC_STRING_LEN,
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/queue/logical_block_size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Block Size: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/rbd")) != NULL) {
                    /* For RBD (Ceph) block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], "%s", MISC_STRING_LEN,
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/queue/logical_block_size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Block Size: %s", tmp_buff);
                        dev_cnt++;
                    }

                } else if ((strstr(dev_node_test, "/dev/nvme")) != NULL) {
                    /* For NVMe block devices */
                    if (dev_cnt < MAX_BLOCK_DEVS) {
                        snprintf(blk_dev_name[dev_cnt], MISC_STRING_LEN, "%s",
                                dir_entry->d_name);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/%s/size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_size[dev_cnt], MISC_STRING_LEN, "%s",
                                tmp_buff);
                        snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
                                "%s/%s/queue/logical_block_size",
                                SYSFS_BLOCK, blk_dev_name[dev_cnt]);
                        readAttribute(dir_name, tmp_buff);
                        snprintf(blk_dev_info[dev_cnt], MISC_STRING_LEN,
                                "Block Size: %s", tmp_buff);
                        dev_cnt++;
                    }
                }
                // TODO: Still more controller block devices (ida, rd)
                // need to be added but we need hardware so we can
                // confirm sysfs attributes.
            }
        }
        if (finished)
            break;

        /* Close the directory stream, we're done */
        closedir(dir_stream);
        break;
    }

    /* Done */
    FREE_NULL(boot_dev_node);
    return dev_cnt;
}
