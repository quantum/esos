/*
 * $Id: menu_actions.h 139 2012-07-24 20:17:34Z marc.smith $
 */

#ifndef _MENU_ACTIONS_H
#define	_MENU_ACTIONS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "megaraid.h"

#define MAX_SCST_TGTS 256
#define MAX_SCST_GROUPS 128
#define MAX_SCST_LUNS 256
#define MAX_SCSI_DISKS 128
#define MAX_SCST_DEVS 128
#define MAX_SCST_INITS 128
#define MAX_SCST_DRIVERS 16
#define SCSTADMIN_TOOL "/usr/sbin/scstadmin"
#define SCST_CONF "/etc/scst.conf"
#define SYNC_CONF_TOOL "/usr/local/sbin/conf_sync.sh"

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

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_H */

