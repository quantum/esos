/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_CONTROLLERS_H
#define	_MENU_ACTIONS_CONTROLLERS_H

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

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_CONTROLLERS_H */

