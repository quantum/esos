/**
 * @file prototypes.h
 * @brief Contains the function prototypes for just about everything.
 * @author Copyright (c) 2019 Parodyne Inc.
 */

#ifndef _PROTOTYPES_H
#define	_PROTOTYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "system.h"
#include "dialogs.h"


/* main.c */
void termSize(WINDOW *screen);
void screenResize(CDKSCREEN *cdk, WINDOW *main_screen, WINDOW *sub_screen,
        int *latest_term_y, int *latest_term_x);
void statusBar(WINDOW *window);
boolean acceptLicense(CDKSCREEN *main_cdk_screen);
void reportUsage(CDKSCREEN *main_cdk_screen);
void setTheme();
void cleanExit(int signal_num);

/* info_labels.c */
boolean updateInfoLabels(CDKSCREEN *cdk_screen,
        CDKLABEL **tgt_info, CDKLABEL **sess_info,
        char *tgt_info_msg[], char *sess_info_msg[],
        int *last_scr_y, int *last_scr_x,
        int *last_tgt_rows, int *last_sess_rows);
int readTargetData(char *label_msg[]);
int readSessionData(char *label_msg[]);

/* menu_common.c */
void errorDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
void informDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
boolean confirmDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
void okButtonCB(CDKBUTTON *button);
void cancelButtonCB(CDKBUTTON *button);
void getSCSTTgtChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[]);
void getSCSTGroupChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[], char tgt_group[]);
int getSCSTLUNChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[],
        char tgt_group[]);
char *getSCSIDiskChoice(CDKSCREEN *cdk_screen);
void getSCSTDevChoice(CDKSCREEN *cdk_screen, char dev_name[],
        char dev_handler[]);
void getSCSTInitChoice(CDKSCREEN *cdk_screen, char tgt_name[],
        char tgt_driver[], char tgt_group[], char initiator[]);
void syncConfig(CDKSCREEN *main_cdk_screen);
void getUserAcct(CDKSCREEN *cdk_screen, char user_acct[]);
boolean questionDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
void getFSChoice(CDKSCREEN *cdk_screen, char fs_name[], char fs_path[],
        char fs_type[], boolean *mounted);
char *getBlockDevChoice(CDKSCREEN *cdk_screen);
char *getSCSIDevChoice(CDKSCREEN *cdk_screen, int scsi_dev_type);
void getSCSTDevGrpChoice(CDKSCREEN *cdk_screen, char dev_group[]);
void getSCSTTgtGrpChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_tgt_group[]);
void getSCSTDevGrpDevChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_dev_grp_dev[]);
void getSCSTTgtGrpTgtChoice(CDKSCREEN *cdk_screen, char alua_dev_group[],
        char alua_tgt_group[], char alua_tgt_grp_tgt[]);
boolean checkInputStr(CDKSCREEN *cdk_screen, valid_input_t char_test_type,
        char *input_str);
void getNetConfChoice(CDKSCREEN* cdk_screen, boolean *general_opt,
        char iface_name[], char iface_mac[], char iface_speed[],
        char iface_duplex[], bonding_t *iface_bonding, boolean *iface_bridge,
        char **slaves, int *slave_cnt, char **br_members, int *br_member_cnt);
int getBlockDevSelection(CDKSCREEN *cdk_screen,
        char blk_dev_list[MAX_BLOCK_DEVS][MISC_STRING_LEN]);

/* menu_system.c */
void networkDialog(CDKSCREEN *main_cdk_screen);
void restartNetDialog(CDKSCREEN *main_cdk_screen);
void mailDialog(CDKSCREEN *main_cdk_screen);
void testEmailDialog(CDKSCREEN *main_cdk_screen);
void addUserDialog(CDKSCREEN *main_cdk_screen);
void delUserDialog(CDKSCREEN *main_cdk_screen);
void chgPasswdDialog(CDKSCREEN *main_cdk_screen);
void scstInfoDialog(CDKSCREEN *main_cdk_screen);
void crmStatusDialog(CDKSCREEN *main_cdk_screen);
void dateTimeDialog(CDKSCREEN *main_cdk_screen);
void drbdStatDialog(CDKSCREEN *main_cdk_screen);

/* menu_hardraid.c */
int getCtrlrChoice(CDKSCREEN *cdk_screen, char type[], char id_num[],
        char model[], char serial[]);
int getPDSelection(CDKSCREEN *cdk_screen, boolean avail_only, char type[],
        char id_num[], char encl_slot_list[MAX_HWRAID_PDRVS][MISC_STRING_LEN]);
int getPDChoice(CDKSCREEN *cdk_screen, boolean avail_only, char type[],
        char id_num[], char encl_id[], char slot_num[], char state[],
        char size[], char model[]);
int getLDChoice(CDKSCREEN *cdk_screen, char type[], char id_num[],
        char ld_id[], char raid_lvl[], char state[], char size[],
        char name[]);
void addVolDialog(CDKSCREEN *main_cdk_screen);
void remVolDialog(CDKSCREEN *main_cdk_screen);
void addHSPDialog(CDKSCREEN *main_cdk_screen);
void remHSPDialog(CDKSCREEN *main_cdk_screen);

/* menu_softraid.c */
int getMDArrayChoice(CDKSCREEN *cdk_screen, char level[], char dev_cnt[],
        char metadata[], char dev_path[]);
int getMDMemberDevChoice(CDKSCREEN *cdk_screen, char dev_path[],
        char member_dev[], char member_role[]);
void softRAIDStatDialog(CDKSCREEN *main_cdk_screen);
void addArrayDialog(CDKSCREEN *main_cdk_screen);
void remArrayDialog(CDKSCREEN *main_cdk_screen);
void faultDevDialog(CDKSCREEN *main_cdk_screen);
void addDevDialog(CDKSCREEN *main_cdk_screen);
void remDevDialog(CDKSCREEN *main_cdk_screen);

/* menu_lvm.c */
int getPVSelection(CDKSCREEN *cdk_screen, boolean avail_only,
        char pv_name_list[MAX_LVM_PVS][MISC_STRING_LEN]);
int getVGChoice(CDKSCREEN *cdk_screen, char name[], char size[],
        char free_space[], char pv_cnt[], char lv_cnt[]);
int getLVChoice(CDKSCREEN *cdk_screen, char path[], char size[], char attr[]);
void lvm2InfoDialog(CDKSCREEN *main_cdk_screen);
void addPVDialog(CDKSCREEN *main_cdk_screen);
void remPVDialog(CDKSCREEN *main_cdk_screen);
void addVGDialog(CDKSCREEN *main_cdk_screen);
void remVGDialog(CDKSCREEN *main_cdk_screen);
void addLVDialog(CDKSCREEN *main_cdk_screen);
void remLVDialog(CDKSCREEN *main_cdk_screen);

/* menu_filesys.c */
void createFSDialog(CDKSCREEN *main_cdk_screen);
void removeFSDialog(CDKSCREEN *main_cdk_screen);
void addVDiskFileDialog(CDKSCREEN *main_cdk_screen);
void delVDiskFileDialog(CDKSCREEN *main_cdk_screen);
void vdiskFileListDialog(CDKSCREEN *main_cdk_screen);

/* menu_hosts.c */
void addGroupDialog(CDKSCREEN *main_cdk_screen);
void remGroupDialog(CDKSCREEN *main_cdk_screen);
void addInitDialog(CDKSCREEN *main_cdk_screen);
void remInitDialog(CDKSCREEN *main_cdk_screen);

/* menu_devices.c */
void addDeviceDialog(CDKSCREEN *main_cdk_screen);
void remDeviceDialog(CDKSCREEN *main_cdk_screen);
void devInfoDialog(CDKSCREEN *main_cdk_screen);
void mapDeviceDialog(CDKSCREEN *main_cdk_screen);
void unmapDeviceDialog(CDKSCREEN *main_cdk_screen);
void lunLayoutDialog(CDKSCREEN *main_cdk_screen);

/* menu_targets.c */
void tgtInfoDialog(CDKSCREEN *main_cdk_screen);
void addiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void remiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void issueLIPDialog(CDKSCREEN *main_cdk_screen);
void enblDsblTgtDialog(CDKSCREEN *main_cdk_screen);
void setRelTgtIDDialog(CDKSCREEN *main_cdk_screen);

/* menu_alua.c */
void devTgtGrpLayoutDialog(CDKSCREEN *main_cdk_screen);
void addDevGrpDialog(CDKSCREEN *main_cdk_screen);
void remDevGrpDialog(CDKSCREEN *main_cdk_screen);
void addTgtGrpDialog(CDKSCREEN *main_cdk_screen);
void remTgtGrpDialog(CDKSCREEN *main_cdk_screen);
void addDevToGrpDialog(CDKSCREEN *main_cdk_screen);
void remDevFromGrpDialog(CDKSCREEN *main_cdk_screen);
void addTgtToGrpDialog(CDKSCREEN *main_cdk_screen);
void remTgtFromGrpDialog(CDKSCREEN *main_cdk_screen);

/* menu_interface.c */
void themeDialog(CDKSCREEN *main_cdk_screen);
void helpDialog(CDKSCREEN *main_cdk_screen);
void supportArchDialog(CDKSCREEN *main_cdk_screen);
void aboutDialog(CDKSCREEN *main_cdk_screen);

/* utility.c */
char *strStrip(char *string);
void readAttribute(char sysfs_attr[], char attr_value[]);
int writeAttribute(char sysfs_attr[], char attr_value[]);
int isSCSTLoaded();
boolean isSCSTInitInGroup(char tgt_name[], char tgt_driver[],
        char group_name[], char init_name[]);
int countSCSTInitUses(char tgt_name[], char tgt_driver[], char init_name[]);
boolean listSCSTTgtDrivers(char tgt_drivers[][MISC_STRING_LEN],
        int *driver_cnt);
int countSCSTSessLUNs(char tgt_name[], char tgt_driver[], char init_name[]);
char *prettyFormatBytes(uint64_t size);
boolean checkInetAccess();
char *prettyShrinkStr(size_t max_len, char *string);
int getUsableBlockDevs(CDKSCREEN *cdk_screen,
        char blk_dev_name[MAX_BLOCK_DEVS][MISC_STRING_LEN],
        char blk_dev_info[MAX_BLOCK_DEVS][MISC_STRING_LEN],
        char blk_dev_size[MAX_BLOCK_DEVS][MISC_STRING_LEN]);
char *rcSvcStatus(char rc_svc_name[]);
char *checkAgentLic();

/* strings.c */
size_t g_scst_dev_types_size();
size_t g_scst_handlers_size();
size_t g_sync_label_msg_size();
size_t g_add_ld_label_msg_size();
size_t g_add_array_label_msg_size();
size_t g_add_lv_label_msg_size();
size_t g_usage_label_msg_size();

#ifdef	__cplusplus
}
#endif

#endif	/* _PROTOTYPES_H */
