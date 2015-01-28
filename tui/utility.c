/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <syslog.h>
#include <inttypes.h>
#include <netdb.h>

#include "prototypes.h"
#include "system.h"

/*
 * Strip whitespace and trailing newline from a string. Originally
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


/*
 * Walk the group directories in sysfs for the specified target and count the
 * number of times the initiator is used. Return -1 if an error occurs.
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


/*
 * Fill the SCST target drivers list and set driver count for all target
 * drivers detected in the sysfs structure. If something fails internally
 * in the function, return FALSE, otherwise return TRUE.
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


/*
 * Count the number of LUNs for a session with the given target, driver, and
 * initiator combination. Return -1 if an error occurs.
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


/*
 * This function takes a number of bytes and formats/converts the value for
 * a "human-readable" representation of the number (eg, 2048 returns "2 KiB").
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


/*
 * Test and see if the Internet is reachable; we do this by attempting
 * to resolve a DNS entry. The GLIBC getaddrinfo_a() function is used
 * so we can timeout if the query takes too long. We simply assume the
 * Internet access works if we're able to successfully resolve the entry.
 */
boolean checkInetAccess() {
    struct gaicb **requests = NULL;
    struct timespec timeout = {0};
    int ret_val = 0, loop_cnt = 0;
    
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
            return FALSE;
        }
        ++loop_cnt;
        /* Check the request */
        ret_val = gai_error(requests[0]);
        if (ret_val == EAI_INPROGRESS) {
            ;
        } else if (ret_val == 0) {
            return TRUE;
        } else {
            DEBUG_LOG("gai_error(): %s", gai_strerror(ret_val));
            return FALSE;
        }
        /* Sleep for a bit */
        if (nanosleep(&timeout, NULL) != 0) {
            DEBUG_LOG("nanosleep(): %s", strerror(errno));
            return FALSE;
        }
    }
}
