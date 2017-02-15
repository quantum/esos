/**
 * @file prototypes.h
 * @brief Contains the function prototypes for just about everything.
 * @author Copyright (c) 2012-2017 Marc A. Smith
 */

#ifndef _PROTOTYPES_H
#define	_PROTOTYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "system.h"
#include "megaraid.h"
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
int getAdpChoice(CDKSCREEN *cdk_screen, MRADAPTER *mr_adapters[]);
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
void adpPropsDialog(CDKSCREEN *main_cdk_screen);
void adpInfoDialog(CDKSCREEN *main_cdk_screen);
void addVolDialog(CDKSCREEN *main_cdk_screen);
void remVolDialog(CDKSCREEN *main_cdk_screen);
void modVolDialog(CDKSCREEN *main_cdk_screen);

/* menu_softraid.c */
void softRAIDStatDialog(CDKSCREEN *main_cdk_screen);

/* menu_lvm.c */
void lvm2InfoDialog(CDKSCREEN *main_cdk_screen);

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

/* strings.c */
size_t g_scst_dev_types_size();
size_t g_scst_handlers_size();
size_t g_sync_label_msg_size();
size_t g_usage_label_msg_size();

#ifdef	__cplusplus
}
#endif

#endif	/* _PROTOTYPES_H */
