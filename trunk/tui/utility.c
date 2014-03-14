/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <syslog.h>

#include "prototypes.h"
#include "system.h"

/*
 * Strip whitespace and trailing newline from a string.
 * Originally from Linux kernel lib/string.c (strim()).
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


/*
 * Reads a sysfs attribute value. Open the specified file and read its
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


/*
 * Write a sysfs attribute value. If an error is encountered, we return
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


/*
 * Test if SCST is loaded on the current machine. For now we have a very
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


/*
 * Check if the given initiator name is already in use in the group name
 * specified. Return true if it does exist, and false if it does not. If any
 * errors occur in this function, we return false.
 */
boolean isSCSTInitInGroup(char tgt_name[], char tgt_driver[],
        char group_name[], char init_name[]) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    boolean found_init = false;

    /* Open the group initiator directory */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/targets/%s/%s/ini_groups/%s/initiators",
            SYSFS_SCST_TGT, tgt_driver, tgt_name, group_name);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        openlog(LOG_PREFIX, LOG_OPTIONS, LOG_FACILITY);
        syslog(LOG_ERR, "opendir(): %s", strerror(errno));
        closelog();
        return false;
    }

    /* Loop over each entry in the directory */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The initiators are files */
        if ((dir_entry->d_type == DT_REG) &&
                (strcmp(dir_entry->d_name, init_name) == 0)) {
            found_init = true;
            break;
        }
    }

    /* Done */
    closedir(dir_stream);
    return found_init;
}


/*
 * Walk the group directories in sysfs for the specified target and count the
 * number of times the initiator is used. Return -1 if an error occurs.
 */
int countSCSTInitUses(char tgt_name[], char tgt_driver[], char init_name[]) {
    DIR *grp_dir_stream = NULL, *init_dir_stream = NULL;
    struct dirent *grp_dir_entry = NULL, *init_dir_entry = NULL;
    char groups_dir_name[MAX_SYSFS_PATH_SIZE] = {0},
            inits_dir_name[MAX_SYSFS_PATH_SIZE] = {0};
    int i = 0, use_count = 0;

    /* Open the target group's directory (to get a list) */
    snprintf(groups_dir_name, MAX_SYSFS_PATH_SIZE,
            "%s/targets/%s/%s/ini_groups",
            SYSFS_SCST_TGT, tgt_driver, tgt_name);
    if ((grp_dir_stream = opendir(groups_dir_name)) == NULL) {
        openlog(LOG_PREFIX, LOG_OPTIONS, LOG_FACILITY);
        syslog(LOG_ERR, "opendir(): %s", strerror(errno));
        closelog();
        return -1;
    }

    /* Loop over each entry in the directory */
    while ((grp_dir_entry = readdir(grp_dir_stream)) != NULL) {
        /* The groups are directories; skip '.' and '..' */
        if (grp_dir_entry->d_type == DT_DIR) {
            if (i > 1) {
                /* Now for each group, read through their initiators */
                snprintf(inits_dir_name, MAX_SYSFS_PATH_SIZE,
                        "%s/targets/%s/%s/ini_groups/%s/initiators",
                        SYSFS_SCST_TGT, tgt_driver, tgt_name,
                        grp_dir_entry->d_name);
                if ((init_dir_stream = opendir(inits_dir_name)) == NULL) {
                    openlog(LOG_PREFIX, LOG_OPTIONS, LOG_FACILITY);
                    syslog(LOG_ERR, "opendir(): %s", strerror(errno));
                    closelog();
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
            i++;
        }
    }

    /* Done */
    closedir(grp_dir_stream);
    return use_count;
}


/*
 * Fill the SCST target drivers list and set driver count for all target
 * drivers detected in the sysfs structure. If something fails internally
 * in the function, return false, otherwise return true.
 */
boolean listSCSTTgtDrivers(char tgt_drivers[][MISC_STRING_LEN], int *driver_cnt) {
    DIR *dir_stream = NULL;
    struct dirent *dir_entry = NULL;
    int i = 0;
    char dir_name[MAX_SYSFS_PATH_SIZE] = {0};

    /* Open the directory for target drivers */
    snprintf(dir_name, MAX_SYSFS_PATH_SIZE, "%s/targets", SYSFS_SCST_TGT);
    if ((dir_stream = opendir(dir_name)) == NULL) {
        return false;
    }

    /* Loop over each directory and add it to the list */
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        /* The driver names are directories */
        if ((dir_entry->d_type == DT_DIR) &&
                (strcmp(dir_entry->d_name, ".") != 0) &&
                (strcmp(dir_entry->d_name, "..") != 0)) {
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
    return true;
}
