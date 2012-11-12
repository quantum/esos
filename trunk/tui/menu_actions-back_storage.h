/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_BACK_STORAGE_H
#define	_MENU_ACTIONS_BACK_STORAGE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "megaraid.h"

#define MAX_ADP_INFO_LINES 256

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

#endif	/* _MENU_ACTIONS_BACK_STORAGE_H */

