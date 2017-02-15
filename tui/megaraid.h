/**
 * @file megaraid.h
 * @brief Macros and structure definitions for MegaRAID functions.
 * @author Copyright (c) 2012-2017 Marc A. Smith
 */

#ifndef _MEGARAID_H
#define	_MEGARAID_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <cdk.h>

/* MegaRAID specific limits */
#define MAX_MC_LINE             120
#define MAX_MR_ATTR_SIZE        128
#define MAX_MR_DISKS            300
#define MAX_MR_LDS              100
#define MAX_MR_ENCLS            50
#define MAX_ADAPTERS            10
#define MAX_ENCLOSURES          30
#define MAX_DISKS               300
#define MAX_LD_NAME             15
#define MAX_MR_PD_LIST_BUFF     512

/* We re-use boolean from cdk.h */
/*typedef int boolean;*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Structure to hold MegaRAID adapter data */
typedef struct megaraid_adapter MRADAPTER;
struct megaraid_adapter {
    int adapter_id;
    /* Product Name */
    char prod_name[MAX_MR_ATTR_SIZE];
    /* Serial No */
    char serial[MAX_MR_ATTR_SIZE];
    /* FW Package Build */
    char firmware[MAX_MR_ATTR_SIZE];
    /* BBU */
    char bbu[MAX_MR_ATTR_SIZE];
    /* Memory Size */
    char memory[MAX_MR_ATTR_SIZE];
    /* Host Interface */
    char interface[MAX_MR_ATTR_SIZE];
    /* Virtual Drives */
    int logical_drv_cnt;
    /* Disks */
    int disk_cnt;
    /* Cluster Permitted */
    char cluster[MAX_MR_ATTR_SIZE];
    /* Cluster Active */
    char cluster_on[MAX_MR_ATTR_SIZE];
};

/* Structure to hold MegaRAID disk data */
typedef struct megaraid_disk MRDISK;
struct megaraid_disk {
    int adapter_id;
    boolean present;
    /* Enclosure Device ID */
    int enclosure_id;
    /* Slot Number */
    int slot_num;
    /* PD Type */
    char pd_type[MAX_MR_ATTR_SIZE];
    /* Raw Size */
    char raw_size[MAX_MR_ATTR_SIZE];
    /* Firmware state */
    char state[MAX_MR_ATTR_SIZE];
    /* Inquiry Data */
    char inquiry[MAX_MR_ATTR_SIZE];
    /* Link Speed */
    char speed[MAX_MR_ATTR_SIZE];
    /* Belongs to an LD, or no */
    boolean part_of_ld;
};

/* Structure to hold MegaRAID logical drive (LD) data */
typedef struct megaraid_logical_drive MRLDRIVE;
struct megaraid_logical_drive {
    int adapter_id;
    int ldrive_id;
    /* RAID Level */
    char raid_lvl[MAX_MR_ATTR_SIZE];
    /* Size */
    char size[MAX_MR_ATTR_SIZE];
    /* State */
    char state[MAX_MR_ATTR_SIZE];
    /* Strip Size */
    char strip_size[MAX_MR_ATTR_SIZE];
    /* Number Of Drives */
    int drive_cnt;
};

/* Structure to hold MegaRAID enclosure data */
typedef struct megaraid_enclosure MRENCL;
struct megaraid_enclosure {
    int adapter_id;
    /* Device ID */
    int device_id;
    /* Number of Slots */
    int slots;
    /* Number of Power Supplies */
    int power_supps;
    /* Number of Fans */
    int fans;
    /* Status */
    char status[MAX_MR_ATTR_SIZE];
    /* Vendor Identification */
    char vendor[MAX_MR_ATTR_SIZE];
    /* Product Identification */
    char product[MAX_MR_ATTR_SIZE];
};

/* Structure to hold MegaRAID adapter properties */
typedef struct megaraid_adp_props MRADPPROPS;
struct megaraid_adp_props {
    int adapter_id;
    /* Cache flush interval in seconds. Values: 0 to 255. */
    int cache_flush;
    /* Rebuild rate. Values: 0 to 100. */
    int rebuild_rate;
    /* Cluster is enabled or disabled. Values: 0 - Disabled, 1 - Enabled. */
    boolean cluster;
    /* Enable/Disable native command queueing. */
    boolean ncq;
};

/* Structure to hold MegaRAID logical drive (LD) properties */
typedef struct megaraid_ld_props MRLDPROPS;
struct megaraid_ld_props {
    int adapter_id;
    int ldrive_id;
    /* Cached, Direct: Selects cache policy. */
    char cache_policy[MAX_MR_ATTR_SIZE];
    /* WT (Write through), WB (Write back): Selects write policy. */
    char write_policy[MAX_MR_ATTR_SIZE];
    /* NORA (No read ahead), RA (Read ahead), ADRA (Adaptive read ahead):
     * Selects read policy. */
    char read_policy[MAX_MR_ATTR_SIZE];
    /* CachedBadBBU, NoCachedBadBBU:
     * Specifies whether to use write cache when the BBU is bad. */
    char bbu_cache_policy[MAX_MR_ATTR_SIZE];
    /* Name String (no spaces) */
    char name[MAX_MR_ATTR_SIZE];
};

/* Function prototypes */
char *getMegaCLIVersion();
int getMRAdapterCount();
MRADAPTER *getMRAdapter(int adapter_id);
MRDISK *getMRDisk(int adapter_id, int encl_id, int slot);
MRLDRIVE *getMRLogicalDrive(int adapter_id, int ldrive_id);
int getMRLDCount(int adapter_id);
MRENCL *getMREnclosure(int adapter_id, int encl_id);
int addMRLogicalDrive(MRLDPROPS *ld_props, int num_disks, MRDISK *disks[],
        char raid_lvl[], char strip_size[]);
int delMRLogicalDrive(int adapter_id, int ldrive_id);
char *getMRLDPath(int adapter_id, int logical_drive);
MRADPPROPS *getMRAdapterProps(int adapter_id);
int setMRAdapterProps(MRADPPROPS *adp_props);
MRLDPROPS *getMRLDProps(int adapter_id, int ldrive_id);
int setMRLDProps(MRLDPROPS *ld_props);
int getMREnclCount(int adapter_id);
int getMRLDDisks(int adapter_id, int ldrive_id, int encl_ids[], int slots[]);
boolean isLDBootDrive(int adapter_id, int ldrive_id);
int getMRLDIDNums(int adapter_id, int ld_count, int ld_ids[]);

#ifdef	__cplusplus
}
#endif

#endif	/* _MEGARAID_H */
