/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_HOSTS_H
#define	_MENU_ACTIONS_HOSTS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define SCST_GRP_NAME_LEN 16
#define SCST_INITIATOR_LEN 64

void addGroupDialog(CDKSCREEN *main_cdk_screen);
void remGroupDialog(CDKSCREEN *main_cdk_screen);
void addInitDialog(CDKSCREEN *main_cdk_screen);
void remInitDialog(CDKSCREEN *main_cdk_screen);

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_HOSTS_H */

