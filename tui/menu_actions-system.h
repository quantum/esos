/*
 * $Id$
 */

#ifndef _MENU_ACTIONS_SYSTEM_H
#define	_MENU_ACTIONS_SYSTEM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_EMAIL_LEN 32
#define MAX_SMTP_LEN 32
#define MAX_SMTP_USER_LEN 32
#define MAX_SMTP_PASS_LEN 32
#define SSMTP_CONF "/etc/ssmtp.conf"
#define MAX_INI_VAL 64
#define MAX_NET_IFACE 32
#define MAX_HOSTNAME 32
#define MAX_DOMAINNAME 32
#define MAX_IPV4_ADDR_LEN 15
#define NETWORK_CONF "/etc/network.conf"
#define CHPASSWD_TOOL "/usr/sbin/chpasswd"
#define ADDUSER_TOOL "/bin/adduser"
#define DELUSER_TOOL "/bin/deluser"
#define DELGROUP_TOOL "/bin/delgroup"
#define TUI_BIN "/usr/local/bin/esos_tui"
#define MAX_SCST_INFO_ROWS 10
#define MAX_SCST_INFO_COLS 60
#define ESOS_SUPERUSER "root"
#define RC_NETWORK "/etc/rc.d/rc.network"
#define MAX_NET_RESTART_INFO_ROWS 15
#define MAX_NET_RESTART_INFO_COLS 60
#define SSMTP_BIN "/usr/sbin/ssmtp"

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

#ifdef	__cplusplus
}
#endif

#endif	/* _MENU_ACTIONS_SYSTEM_H */

