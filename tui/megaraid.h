/*
 * $Id$
 */

#ifndef _MEGARAID_H
#define	_MEGARAID_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <cdk.h>

#define MEGACLI "/opt/sbin/MegaCli64 2>&1"
#define MAX_MC_LINE 120
#define MAX_MR_ATTR_SIZE 80
#define MAX_MR_DISKS 300
#define MAX_MR_LDS 100
#define MAX_MR_ENCLS 50
#define MAX_ADAPTERS 10
#define MAX_ENCLOSURES 30
#define MAX_DISKS 300
#define MAX_LD_NAME 15

// This is now defined in cdk.h so there isn't really a need for this; delete later after testing.
/*typedef int boolean;*/

#define TRUE 1
#define FALSE 0

typedef struct megaraid_adapter MRADAPTER;
struct megaraid_adapter {
    int adapter_id;
    char prod_name[MAX_MR_ATTR_SIZE];       /* Product Name */
    char serial[MAX_MR_ATTR_SIZE];          /* Serial No */
    char firmware[MAX_MR_ATTR_SIZE];        /* FW Package Build */
    char bbu[MAX_MR_ATTR_SIZE];             /* BBU */
    char memory[MAX_MR_ATTR_SIZE];          /* Memory Size */
    char interface[MAX_MR_ATTR_SIZE];       /* Host Interface */
    int logical_drv_cnt;                    /* Virtual Drives */
    int disk_cnt;                           /* Disks */
    char cluster[MAX_MR_ATTR_SIZE];         /* Cluster Permitted */
    char cluster_on[MAX_MR_ATTR_SIZE];      /* Cluster Active */
};

typedef struct megaraid_disk MRDISK;
struct megaraid_disk {
    int adapter_id;
    boolean present;
    int enclosure_id;                   /* Enclosure Device ID */
    int slot_num;                       /* Slot Number */
    char pd_type[MAX_MR_ATTR_SIZE];     /* PD Type */
    char raw_size[MAX_MR_ATTR_SIZE];    /* Raw Size */
    char state[MAX_MR_ATTR_SIZE];       /* Firmware state */
    char inquiry[MAX_MR_ATTR_SIZE];     /* Inquiry Data */
    char speed[MAX_MR_ATTR_SIZE];       /* Link Speed */
    boolean part_of_ld;                 /* Belongs to an LD, or no */
};

typedef struct megaraid_logical_drive MRLDRIVE;
struct megaraid_logical_drive {
    int adapter_id;
    int ldrive_id;
    char raid_lvl[MAX_MR_ATTR_SIZE];    /* RAID Level */
    char size[MAX_MR_ATTR_SIZE];        /* Size */
    char state[MAX_MR_ATTR_SIZE];       /* State */
    char strip_size[MAX_MR_ATTR_SIZE];  /* Strip Size */
    int drive_cnt;                      /* Number Of Drives */
};

typedef struct megaraid_enclosure MRENCL;
struct megaraid_enclosure {
    int adapter_id;
    int device_id;                  /* Device ID */
    int slots;                      /* Number of Slots */
    int power_supps;                /* Number of Power Supplies */
    int fans;                       /* Number of Fans */
    char status[MAX_MR_ATTR_SIZE];  /* Status */
    char vendor[MAX_MR_ATTR_SIZE];  /* Vendor Identification */
    char product[MAX_MR_ATTR_SIZE]; /* Product Identification */
};

typedef struct megaraid_adp_props MRADPPROPS;
struct megaraid_adp_props {
    int adapter_id;
    int cache_flush;    /* Cache flush interval in seconds. Values: 0 to 255. */
    int rebuild_rate;   /* Rebuild rate. Values: 0 to 100. */
    boolean cluster;    /* Cluster is enabled or disabled. Values: 0 - Disabled, 1 - Enabled. */
    boolean ncq;        /* Enable/Disable native command queueing. */
};

typedef struct megaraid_ld_props MRLDPROPS;
struct megaraid_ld_props {
    int adapter_id;
    int ldrive_id;
    char cache_policy[MAX_MR_ATTR_SIZE];        /* Cached, Direct: Selects cache policy. */
    char write_policy[MAX_MR_ATTR_SIZE];        /* WT (Write through), WB (Write back): Selects write policy. */
    char read_policy[MAX_MR_ATTR_SIZE];         /* NORA (No read ahead), RA (Read ahead), ADRA (Adaptive read ahead): Selects read policy. */
    char bbu_cache_policy[MAX_MR_ATTR_SIZE];    /* CachedBadBBU, NoCachedBadBBU: Specifies whether to use write cache when the BBU is bad. */
    char name[MAX_MR_ATTR_SIZE];                /* Name String (no spaces) */
};

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

