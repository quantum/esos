/*
 * $Id$
 */

#include <stddef.h>

#include "prototypes.h"

/* Dialog radio widget options */
char *g_no_yes_opts[] = {"No", "Yes"},
        *g_auth_meth_opts[] = {"None", "Plain Text", "CRAM-MD5"},
        *g_ip_opts[] = {"Disabled", "Static", "DHCP"},
        *g_cache_opts[] = {"Cached", "Direct"},
        *g_write_opts[] = {"WT", "WB"},
        *g_read_opts[] = {"NORA", "RA", "ADRA"},
        *g_bbu_opts[] = {"CachedBadBBU", "NoCachedBadBBU"},
        *g_raid_opts[] = {"0", "1", "5", "6"},
        *g_strip_opts[] = {"8", "16", "32", "64", "128", "256", "512", "1024"},
        *g_dsbl_enbl_opts[] = {"Disabled (0)", "Enabled (1)"},
        *g_fs_type_opts[] = {"xfs", "ext2", "ext3", "ext4"};

/* Label title strings */
char *g_mail_title_msg[] = {"</31/B>System mail (SMTP) settings..."},
        *g_add_user_title_msg[] = {"</31/B>Adding a new ESOS user account..."},
        *g_date_title_msg[] = {"</31/B>Edit date and time settings..."};

/* Misc. widget related strings */
char *g_choice_char[] = {"[ ] ", "[*] "},
        *g_bonding_map[] = {"None", "Master", "Slave"},
        *g_scst_dev_types[] = {"<C>dev_disk", "<C>dev_disk_perf",
        "<C>vcdrom", "<C>vdisk_blockio", "<C>vdisk_fileio", "<C>vdisk_nullio",
        "<C>dev_changer", "<C>dev_tape", "<C>dev_tape_perf"},
        *g_scst_bs_list[] = {"512", "1024", "2048", "4096", "8192"},
        *g_fio_types[] = {"<C>File System", "<C>Block Device"},
        *g_sync_label_msg[] = {"", "",
        "</B>   Synchronizing ESOS configuration...   ", "", ""};

/* Button strings */
char *g_ok_cancel_msg[] = {"</B>   OK   ", "</B> Cancel "},
        *g_ok_msg[] = {"</B>   OK   "},
        *g_yes_no_msg[] = {"</B>   Yes   ", "</B>   No   "};

/* Other string stuff */
char *g_transports[] = {"unknown", "scsi", "ide", "dac960", "cpqarray",
        "file", "ataraid", "i2o", "ubd", "dasd", "viodasd", "sx8", "dm"},
        *g_scst_handlers[] = {"dev_disk", "dev_disk_perf", "vcdrom",
        "vdisk_blockio", "vdisk_fileio", "vdisk_nullio", "dev_changer",
        "dev_tape", "dev_tape_perf"};

/* Functions to return the sizes */
size_t g_scst_dev_types_size() {
    return (sizeof g_scst_dev_types) / (sizeof g_scst_dev_types[0]);
}
size_t g_scst_handlers_size() {
    return (sizeof g_scst_handlers) / (sizeof g_scst_handlers[0]);
}
size_t g_sync_label_msg_size() {
    return (sizeof g_sync_label_msg) / (sizeof g_sync_label_msg[0]);
}
