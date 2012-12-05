/*
 * $Id$
 */

#ifndef _PROTOTYPES_H
#define	_PROTOTYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "megaraid.h"

/* main.c */
void termSize(WINDOW *screen);
void screenResize(CDKSCREEN *cdk, WINDOW *main_screen, int orig_term_x, int orig_term_y);
char *strStrip(char *string);
void statusBar(WINDOW *window);
void readAttribute(char sysfs_attr[], char attr_value[]);
int writeAttribute(char sysfs_attr[], char attr_value[]);

/* label_data.c */
void readAdapterData(char *label_msg[]);
void readDeviceData(char *label_msg[]);
void readTargetData(char *label_msg[]);

/* menu_actions.c */
void errorDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
boolean confirmDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
void okButtonCB(CDKBUTTON *button);
void cancelButtonCB(CDKBUTTON *button);
void getSCSTTgtChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[]);
void getSCSTGroupChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[]);
int getSCSTLUNChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[]);
char *getSCSIDiskChoice(CDKSCREEN *cdk_screen);
void getSCSTDevChoice(CDKSCREEN *cdk_screen, char dev_name[], char dev_handler[]);
int getAdpChoice(CDKSCREEN *cdk_screen, MRADAPTER *mr_adapters[]);
void getSCSTInitChoice(CDKSCREEN *cdk_screen, char tgt_name[], char tgt_driver[], char tgt_group[], char initiator[]);
void syncConfig(CDKSCREEN *main_cdk_screen);
void getUserAcct(CDKSCREEN *cdk_screen, char user_acct[]);
boolean questionDialog(CDKSCREEN *screen, char *msg_line_1, char *msg_line_2);
void getFSChoice(CDKSCREEN *cdk_screen, char fs_name[], char fs_path[], char fs_type[], boolean *mounted);
char *getBlockDevChoice(CDKSCREEN *cdk_screen);

/* menu_actions-system.c */
void networkDialog(CDKSCREEN *main_cdk_screen);
void restartNetDialog(CDKSCREEN *main_cdk_screen);
void mailDialog(CDKSCREEN *main_cdk_screen);
void testEmailDialog(CDKSCREEN *main_cdk_screen);
void addUserDialog(CDKSCREEN *main_cdk_screen);
void delUserDialog(CDKSCREEN *main_cdk_screen);
void chgPasswdDialog(CDKSCREEN *main_cdk_screen);
void scstInfoDialog(CDKSCREEN *main_cdk_screen);
void linuxHAStatDialog(CDKSCREEN *main_cdk_screen);
void dateTimeDialog(CDKSCREEN *main_cdk_screen);

/* menu_actions-targets.c */
void tgtInfoDialog(CDKSCREEN *main_cdk_screen);
void addiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void remiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void issueLIPDialog(CDKSCREEN *main_cdk_screen);
void enblDsblTgtDialog(CDKSCREEN *main_cdk_screen);

/* menu_actions-hosts.c */
void addGroupDialog(CDKSCREEN *main_cdk_screen);
void remGroupDialog(CDKSCREEN *main_cdk_screen);
void addInitDialog(CDKSCREEN *main_cdk_screen);
void remInitDialog(CDKSCREEN *main_cdk_screen);

/* menu_actions-devices.c */
void addDeviceDialog(CDKSCREEN *main_cdk_screen);
void delDeviceDialog(CDKSCREEN *main_cdk_screen);
void devInfoDialog(CDKSCREEN *main_cdk_screen);
void mapDeviceDialog(CDKSCREEN *main_cdk_screen);
void unmapDeviceDialog(CDKSCREEN *main_cdk_screen);

/* menu_actions-back_storage.c */
void adpPropsDialog(CDKSCREEN *main_cdk_screen);
void adpInfoDialog(CDKSCREEN *main_cdk_screen);
void addVolumeDialog(CDKSCREEN *main_cdk_screen);
void delVolumeDialog(CDKSCREEN *main_cdk_screen);
void volPropsDialog(CDKSCREEN *main_cdk_screen);
void drbdStatDialog(CDKSCREEN *main_cdk_screen);
void softRAIDStatDialog(CDKSCREEN *main_cdk_screen);
void lvm2InfoDialog(CDKSCREEN *main_cdk_screen);
void createFSDialog(CDKSCREEN *main_cdk_screen);
void removeFSDialog(CDKSCREEN *main_cdk_screen);
void addVDiskFileDialog(CDKSCREEN *main_cdk_screen);
void delVDiskFileDialog(CDKSCREEN *main_cdk_screen);
    
#ifdef	__cplusplus
}
#endif

#endif	/* _PROTOTYPES_H */

