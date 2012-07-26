/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_DEVICES_H
#define	_MENU_ACTIONS_DEVICES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include"main.h"

#define SCST_DEV_NAME_LEN 16
#define SG_VPD "/usr/bin/sg_vpd 2>&1"
#define MAX_DEV_INFO_LINES 64

void addDeviceDialog(CDKSCREEN *main_cdk_screen);
void delDeviceDialog(CDKSCREEN *main_cdk_screen);
void devInfoDialog(CDKSCREEN *main_cdk_screen);
void mapDeviceDialog(CDKSCREEN *main_cdk_screen);
void unmapDeviceDialog(CDKSCREEN *main_cdk_screen);

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_DEVICES_H */

