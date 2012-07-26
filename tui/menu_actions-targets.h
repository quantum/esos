/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_TARGETS_H
#define	_MENU_ACTIONS_TARGETS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_TGT_INFO_LINES 64
#define SCST_ISCSI_TGT_LEN 32
#define MAX_LIP_INFO_LINES 32

void tgtInfoDialog(CDKSCREEN *main_cdk_screen);
void addiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void remiSCSITgtDialog(CDKSCREEN *main_cdk_screen);
void issueLIPDialog(CDKSCREEN *main_cdk_screen);
void enblDsblTgtDialog(CDKSCREEN *main_cdk_screen);

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_TARGETS_H */

