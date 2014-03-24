/*
 * $Id$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iniparser.h>
#include <sys/ioctl.h>
#undef CTRL /* This is defined in sys/ioctl.h which conflicts with cdk.h */
#include <cdk.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <cdk/swindow.h>
#include <sys/time.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"

/*
 * Run the Networking dialog
 * Example code for getting available network interfaces taken from here:
 * http://www.adamrisi.com/?p=84
 */
void networkDialog(CDKSCREEN *main_cdk_screen) {
    CDKSCROLL *net_conf_list = 0;
    CDKSCREEN *net_screen = 0;
    CDKLABEL *net_label = 0, *short_label = 0;
    CDKENTRY *host_name = 0, *domain_name = 0, *default_gw = 0,
            *name_server_1 = 0, *name_server_2 = 0, *ip_addy = 0, *netmask = 0,
            *broadcast = 0, *iface_mtu = 0;
    CDKMENTRY *bond_opts = 0;
    CDKRADIO *ip_config = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKSELECTION *slave_select = 0, *br_member_select = 0;
    WINDOW *net_window = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    dictionary *ini_dict = NULL;
    FILE *ini_file = NULL;
    struct ifreq ifr; /* How do we properly initialize this? */
    struct if_nameindex* if_name = NULL;
    struct ethtool_cmd edata = {0};
    unsigned char* mac_addy = NULL;
    int sock = 0, i = 0, j = 0, net_conf_choice = 0, window_y = 0, window_x = 0,
            traverse_ret = 0, net_window_lines = 0, net_window_cols = 0,
            poten_slave_cnt = 0, slaves_line_size = 0, slave_val_size = 0,
            poten_br_member_cnt = 0, br_members_line_size = 0,
            br_member_val_size = 0, temp_int = 0;
    char *net_scroll_msg[MAX_NET_IFACE] = {NULL},
            *net_if_name[MAX_NET_IFACE] = {NULL},
            *net_if_mac[MAX_NET_IFACE] = {NULL},
            *net_if_speed[MAX_NET_IFACE] = {NULL},
            *net_if_duplex[MAX_NET_IFACE] = {NULL},
            *net_info_msg[MAX_NET_INFO_LINES] = {NULL},
            *short_label_msg[NET_SHORT_INFO_LINES] = {NULL},
            *poten_slaves[MAX_NET_IFACE] = {NULL},
            *poten_br_members[MAX_NET_IFACE] = {NULL};
    char *conf_hostname = NULL, *conf_domainname = NULL, *conf_defaultgw = NULL,
            *conf_nameserver1 = NULL, *conf_nameserver2 = NULL,
            *conf_bootproto = NULL, *conf_ipaddr = NULL, *conf_netmask = NULL,
            *conf_broadcast = NULL, *error_msg = NULL, *conf_if_mtu = NULL,
            *temp_pstr = NULL, *conf_slaves = NULL, *conf_brmembers = NULL,
            *strtok_result = NULL, *conf_bondopts = NULL;
    static char *ip_opts[] = {"Disabled", "Static", "DHCP"},
            *choice_char[] = {"[ ] ", "[*] "};
    char eth_duplex[MISC_STRING_LEN] = {0}, temp_str[MISC_STRING_LEN] = {0},
            temp_ini_str[MAX_INI_VAL] = {0},
            slaves_list_line_buffer[MAX_SLAVES_LIST_BUFF] = {0},
            br_members_list_line_buffer[MAX_BR_MEMBERS_LIST_BUFF] = {0},
            bond_opts_buffer[MAX_BOND_OPTS_BUFF] = {0};
    __be16 eth_speed = 0;
    boolean question = FALSE;
    short saved_ifr_flags = 0;
    enum bonding_t net_if_bonding[MAX_NET_IFACE] = {0};
    static char *bonding_map[] = {"None", "Master", "Slave"};
    struct stat bridge_test = {0};
    boolean net_if_bridge[MAX_NET_IFACE] = {FALSE};

    /* Get socket handle */
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        errorDialog(main_cdk_screen, "Couldn't get socket handle!", NULL);
        return;
    }

    /* Get all network interface names */
    if_name = if_nameindex();

    /* We set the counter ahead since the first row is for general settings */
    j = 1;
    asprintf(&net_scroll_msg[0], "<C>General Network Settings");
    
    while ((if_name[i].if_index != 0) && (if_name[i].if_name != NULL)) {
        /* Put the interface name into the ifreq struct */
        memcpy(&ifr.ifr_name, if_name[i].if_name, IFNAMSIZ);

        /* Get interface flags */
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
            asprintf(&error_msg, "ioctl(): SIOCGIFFLAGS Error (%s)", ifr.ifr_name);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
        saved_ifr_flags = ifr.ifr_flags;

        /* We don't want to include the loopback interface */
        if (!(saved_ifr_flags & IFF_LOOPBACK)) {

            /* Get interface hardware address (MAC) */
            if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
                asprintf(&error_msg, "ioctl(): SIOCGIFHWADDR Error (%s)", ifr.ifr_name);
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                return;
            }

            if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
                /* For Ethernet interfaces */
                mac_addy = (unsigned char*) &ifr.ifr_hwaddr.sa_data;
                asprintf(&net_if_name[j], "%s", if_name[i].if_name);
                asprintf(&net_if_mac[j], "%02X:%02X:%02X:%02X:%02X:%02X",
                        mac_addy[0], mac_addy[1], mac_addy[2], mac_addy[3],
                        mac_addy[4], mac_addy[5]);

                /* Check if interface is a bridge */
                snprintf(temp_str, MISC_STRING_LEN, "%s/%s/bridge",
                        SYSFS_NET, ifr.ifr_name);
                if (stat(temp_str, &bridge_test) == 0) {
                    net_if_bridge[j] = TRUE;
                    snprintf(temp_str, MISC_STRING_LEN, "Ethernet Bridge");
                    asprintf(&net_scroll_msg[j], "<C>%-7s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], temp_str);
                    /* We can continue to the next iteration if its a bridge */
                    j++;
                    i++;
                    continue;
                } else {
                    net_if_bridge[j] = FALSE;
                }

                /* Check for NIC bonding */
                if (saved_ifr_flags & IFF_MASTER) {
                    net_if_bonding[j] = MASTER;
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            bonding_map[net_if_bonding[j]]);
                    asprintf(&net_scroll_msg[j], "<C>%-7s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], temp_str);
                    /* We can continue to the next iteration if its a master */
                    j++;
                    i++;
                    continue;
                } else if (saved_ifr_flags & IFF_SLAVE) {
                    net_if_bonding[j] = SLAVE;
                    /* Already a bonding slave interface, add it to the list */
                    asprintf(&poten_slaves[poten_slave_cnt], "%s", net_if_name[j]);
                    poten_slave_cnt++;
                } else {
                    net_if_bonding[j] = NO_BONDING;
                    /* No bonding for this interface, add it to the list */
                    asprintf(&poten_slaves[poten_slave_cnt], "%s", net_if_name[j]);
                    poten_slave_cnt++;
                    /* For now we only grab interfaces that have no part in bonding */
                    asprintf(&poten_br_members[poten_br_member_cnt], "%s", net_if_name[j]);
                    poten_br_member_cnt++;
                }

                /* Get additional Ethernet interface information */
                ifr.ifr_data = (caddr_t) & edata;
                edata.cmd = ETHTOOL_GSET;
                if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
                    asprintf(&error_msg, "ioctl(): SIOCETHTOOL Error (%s)", ifr.ifr_name);
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    return;
                }

                /* Get speed of Ethernet link; we use the returned speed value
                 * to determine the link status -- probably not the best
                 * solution long term, but its easy for now */
                eth_speed = ethtool_cmd_speed(&edata);
                if (eth_speed == 0 || eth_speed == (__be16) (-1) || eth_speed == (__be32) (-1)) {
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            bonding_map[net_if_bonding[j]]);
                    asprintf(&net_scroll_msg[j], "<C>%-7s%-21s%-16s%-26s",
                            net_if_name[j], net_if_mac[j],
                            temp_str, "No Link");
                } else {
                    switch (edata.duplex) {
                        case DUPLEX_HALF:
                            snprintf(eth_duplex, MISC_STRING_LEN, "Half Duplex");
                            break;
                        case DUPLEX_FULL:
                            snprintf(eth_duplex, MISC_STRING_LEN, "Full Duplex");
                            break;
                        default:
                            snprintf(eth_duplex, MISC_STRING_LEN, "Unknown Duplex");
                            break;
                    }
                    asprintf(&net_if_speed[j], "%u Mb/s", eth_speed);
                    asprintf(&net_if_duplex[j], "%s", eth_duplex);
                    snprintf(temp_str, MISC_STRING_LEN, "Bonding: %s",
                            bonding_map[net_if_bonding[j]]);
                    asprintf(&net_scroll_msg[j], "<C>%-7s%-21s%-16s%-12s%-14s",
                            net_if_name[j], net_if_mac[j],
                            temp_str, net_if_speed[j],
                            net_if_duplex[j]);
                }
                j++;

            } else if (ifr.ifr_hwaddr.sa_family == ARPHRD_INFINIBAND) {
                /* For InfiniBand interfaces */
                mac_addy = (unsigned char*) &ifr.ifr_hwaddr.sa_data;
                asprintf(&net_if_name[j], "%s", if_name[i].if_name);
                /* Yes, the link-layer address is 20 bytes, but we'll keep it simple */
                asprintf(&net_if_mac[j], "%02X:%02X:%02X:%02X:%02X:%02X...",
                        mac_addy[0], mac_addy[1], mac_addy[2], mac_addy[3], mac_addy[4], mac_addy[5]);
                asprintf(&net_scroll_msg[j], "<C>%-7s%-21s%-42s",
                            net_if_name[j], net_if_mac[j], "IPoIB");
                j++;
            }
        }
        i++;
    }

    /* Clean up */
    if_freenameindex(if_name);

    /* Scroll widget for network configuration choices */
    net_conf_list = newCDKScroll(main_cdk_screen, CENTER, CENTER, NONE, 15, 76,
            "<C></31/B>Choose a Network Configuration Option\n",
            net_scroll_msg, j, FALSE, COLOR_DIALOG_SELECT, TRUE, FALSE);
    if (!net_conf_list) {
        errorDialog(main_cdk_screen, "Couldn't create scroll widget!", NULL);
        goto cleanup;
    }
    setCDKScrollBoxAttribute(net_conf_list, COLOR_DIALOG_BOX);
    setCDKScrollBackgroundAttrib(net_conf_list, COLOR_DIALOG_TEXT);
    net_conf_choice = activateCDKScroll(net_conf_list, 0);

    /* Check exit from widget */
    if (net_conf_list->exitType == vESCAPE_HIT) {
        destroyCDKScroll(net_conf_list);
        refreshCDKScreen(main_cdk_screen);
        goto cleanup;

    } else if (net_conf_list->exitType == vNORMAL) {
        destroyCDKScroll(net_conf_list);
        refreshCDKScreen(main_cdk_screen);
    }

    /* Present the 'general network settings' screen of widgets */
    if (net_conf_choice == 0) {
        /* Setup a new CDK screen */
        net_window_lines = 16;
        net_window_cols = 68;
        window_y = ((LINES / 2) - (net_window_lines / 2));
        window_x = ((COLS / 2) - (net_window_cols / 2));
        net_window = newwin(net_window_lines, net_window_cols,
                window_y, window_x);
        if (net_window == NULL) {
            errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
            goto cleanup;
        }
        net_screen = initCDKScreen(net_window);
        if (net_screen == NULL) {
            errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
            goto cleanup;
        }
        boxWindow(net_window, COLOR_DIALOG_BOX);
        wbkgd(net_window, COLOR_DIALOG_TEXT);
        wrefresh(net_window);

        asprintf(&net_info_msg[0], "</31/B>General network settings...");
        asprintf(&net_info_msg[1], " ");

        /* Read network configuration file (INI file) */
        ini_dict = iniparser_load(NETWORK_CONF);
        if (ini_dict == NULL) {
            errorDialog(main_cdk_screen, "Cannot parse network configuration file!", NULL);
            return;
        }
        conf_hostname = iniparser_getstring(ini_dict, "general:hostname", "");
        conf_domainname = iniparser_getstring(ini_dict, "general:domainname", "");
        conf_defaultgw = iniparser_getstring(ini_dict, "general:defaultgw", "");
        conf_nameserver1 = iniparser_getstring(ini_dict, "general:nameserver1", "");
        conf_nameserver2 = iniparser_getstring(ini_dict, "general:nameserver2", "");
        
        /* Information label */
        net_label = newCDKLabel(net_screen, (window_x + 1), (window_y + 1),
                net_info_msg, 2, FALSE, FALSE);
        if (!net_label) {
            errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
            goto cleanup;
        }
        setCDKLabelBackgroundAttrib(net_label, COLOR_DIALOG_TEXT);

        /* Host name field */
        host_name = newCDKEntry(net_screen, (window_x + 1), (window_y + 3),
                NULL, "</B>System host name: ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_HOSTNAME, 0, MAX_HOSTNAME, FALSE, FALSE);
        if (!host_name) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(host_name, COLOR_DIALOG_INPUT);
        setCDKEntryValue(host_name, conf_hostname);

        /* Domain name field */
        domain_name = newCDKEntry(net_screen, (window_x + 1), (window_y + 5),
                NULL, "</B>System domain name: ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_DOMAINNAME, 0, MAX_DOMAINNAME, FALSE, FALSE);
        if (!domain_name) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(domain_name, COLOR_DIALOG_INPUT);
        setCDKEntryValue(domain_name, conf_domainname);

        /* Default gateway field */
        default_gw = newCDKEntry(net_screen, (window_x + 1), (window_y + 7),
                NULL, "</B>Default gateway (leave blank if using DHCP): ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!default_gw) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(default_gw, COLOR_DIALOG_INPUT);
        setCDKEntryValue(default_gw, conf_defaultgw);

        /* A very small label for instructions */
        asprintf(&short_label_msg[0], "</B>Name Servers (leave blank if using DHCP)");
        short_label = newCDKLabel(net_screen, (window_x + 1), (window_y + 9),
                short_label_msg, NET_SHORT_INFO_LINES, FALSE, FALSE);
        if (!short_label) {
            errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
            goto cleanup;
        }
        setCDKLabelBackgroundAttrib(short_label, COLOR_DIALOG_TEXT);
        
        /* Primary name server field */
        name_server_1 = newCDKEntry(net_screen, (window_x + 1), (window_y + 10),
                NULL, "</B>NS 1: ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!name_server_1) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(name_server_1, COLOR_DIALOG_INPUT);
        setCDKEntryValue(name_server_1, conf_nameserver1);

        /* Secondary name server field */
        name_server_2 = newCDKEntry(net_screen, (window_x + 1), (window_y + 11),
                NULL, "</B>NS 2: ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!name_server_2) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(name_server_2, COLOR_DIALOG_INPUT);
        setCDKEntryValue(name_server_2, conf_nameserver2);

        /* Buttons */
        ok_button = newCDKButton(net_screen, (window_x + 26), (window_y + 14),
                "</B>   OK   ", ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
            goto cleanup;
        }
        setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
        cancel_button = newCDKButton(net_screen, (window_x + 36), (window_y + 14),
                "</B> Cancel ", cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
            goto cleanup;
        }
        setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

        /* Allow user to traverse the screen */
        refreshCDKScreen(net_screen);
        traverse_ret = traverseCDKScreen(net_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check the host name value (field entry) */
            strncpy(temp_str, getCDKEntryValue(host_name), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_NAME_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The host name entry field contains invalid characters!",
                            VALID_NAME_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the domain name value (field entry) */
            strncpy(temp_str, getCDKEntryValue(domain_name), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_NAME_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The domain name entry field contains invalid characters!",
                            VALID_NAME_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the default gateway value (field entry) */
            strncpy(temp_str, getCDKEntryValue(default_gw), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The default gateway entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the name server (1) value (field entry) */
            strncpy(temp_str, getCDKEntryValue(name_server_1), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The name server (1) entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the name server (2) value (field entry) */
            strncpy(temp_str, getCDKEntryValue(name_server_2), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The name server (2) entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Write to network config. file */
            if (iniparser_set(ini_dict, "general", NULL) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            if (iniparser_set(ini_dict, "general:hostname", getCDKEntryValue(host_name)) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            if (iniparser_set(ini_dict, "general:domainname", getCDKEntryValue(domain_name)) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            if (iniparser_set(ini_dict, "general:defaultgw", getCDKEntryValue(default_gw)) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            if (iniparser_set(ini_dict, "general:nameserver1", getCDKEntryValue(name_server_1)) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            if (iniparser_set(ini_dict, "general:nameserver2", getCDKEntryValue(name_server_2)) == -1) {
                errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                goto cleanup;
            }
            ini_file = fopen(NETWORK_CONF, "w");
            if (ini_file == NULL) {
                asprintf(&error_msg, "Couldn't open network config. file for writing: %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
            } else {
                fprintf(ini_file, "# ESOS network configuration file\n");
                fprintf(ini_file, "# This file is generated by esos_tui; do not edit\n\n");
                iniparser_dump_ini(ini_dict, ini_file);
                fclose(ini_file);
                iniparser_freedict(ini_dict);
            }
        }

    /* Present the screen for a specific network interface configuration */
    } else {
        /* If an interface is enslaved, there is nothing to configure */
        if (net_if_bonding[net_conf_choice] == SLAVE) {
            asprintf(&error_msg, "Interface '%s' is currently enslaved to a master",
                    net_if_name[net_conf_choice]);
            errorDialog(main_cdk_screen, error_msg,
                    "bonding interface, so there is nothing to configure.");
            freeChar(error_msg);
            goto cleanup;
        }

        /* Setup a new CDK screen */
        net_window_lines = 18;
        net_window_cols = 70;
        window_y = ((LINES / 2) - (net_window_lines / 2));
        window_x = ((COLS / 2) - (net_window_cols / 2));
        net_window = newwin(net_window_lines, net_window_cols,
                window_y, window_x);
        if (net_window == NULL) {
            errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
            goto cleanup;
        }
        net_screen = initCDKScreen(net_window);
        if (net_screen == NULL) {
            errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
            goto cleanup;
        }
        boxWindow(net_window, COLOR_DIALOG_BOX);
        wbkgd(net_window, COLOR_DIALOG_TEXT);
        wrefresh(net_window);

        /* Make a nice, informational label */
        asprintf(&net_info_msg[0], "</31/B>Configuring interface %s...",
                net_if_name[net_conf_choice]);
        asprintf(&net_info_msg[1], " ");
        asprintf(&net_info_msg[2], "</B>MAC Address:<!B>\t%s", net_if_mac[net_conf_choice]);
        if (net_if_speed[net_conf_choice] == NULL)
            asprintf(&net_info_msg[3], "</B>Link Status:<!B>\tNone");
        else
            asprintf(&net_info_msg[3], "</B>Link Status:<!B>\t%s, %s",
                    net_if_speed[net_conf_choice], net_if_duplex[net_conf_choice]);
        asprintf(&net_info_msg[4], "</B>Bonding:<!B>\t%s",
                bonding_map[net_if_bonding[net_conf_choice]]);

        /* Read network configuration file (INI file) */
        ini_dict = iniparser_load(NETWORK_CONF);
        if (ini_dict == NULL) {
            errorDialog(main_cdk_screen, "Cannot parse network configuration file!", NULL);
            goto cleanup;
        }
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:bootproto", net_if_name[net_conf_choice]);
        conf_bootproto = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:ipaddr", net_if_name[net_conf_choice]);
        conf_ipaddr = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:netmask", net_if_name[net_conf_choice]);
        conf_netmask = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:broadcast", net_if_name[net_conf_choice]);
        conf_broadcast = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:slaves", net_if_name[net_conf_choice]);
        conf_slaves = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:brmembers", net_if_name[net_conf_choice]);
        conf_brmembers = iniparser_getstring(ini_dict, temp_ini_str, "");
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:bondopts", net_if_name[net_conf_choice]);
        conf_bondopts = iniparser_getstring(ini_dict, temp_ini_str, "");

        /* If value doesn't exist, use a default MTU based on the interface type */
        snprintf(temp_ini_str, MAX_INI_VAL, "%s:mtu", net_if_name[net_conf_choice]);
        if (strstr(net_if_name[net_conf_choice], "eth") != NULL)
            conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str, DEFAULT_ETH_MTU);
        else if (strstr(net_if_name[net_conf_choice], "ib") != NULL)
            conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str, DEFAULT_IB_MTU);
        else if (strstr(net_if_name[net_conf_choice], "bond") != NULL)
            conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str, DEFAULT_ETH_MTU);
        else if (strstr(net_if_name[net_conf_choice], "br") != NULL)
            conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str, DEFAULT_ETH_MTU);
        else
            conf_if_mtu = iniparser_getstring(ini_dict, temp_ini_str, "");

        /* Information label */
        net_label = newCDKLabel(net_screen, (window_x + 1), (window_y + 1),
                net_info_msg, 5, FALSE, FALSE);
        if (!net_label) {
            errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
            goto cleanup;
        }
        setCDKLabelBackgroundAttrib(net_label, COLOR_DIALOG_TEXT);

        /* Interface MTU field */
        iface_mtu = newCDKEntry(net_screen, (window_x + 50), (window_y + 3),
                "</B>Interface MTU:  ", NULL,
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT,
                15, 0, 15, FALSE, FALSE);
        if (!iface_mtu) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(iface_mtu, COLOR_DIALOG_INPUT);
        setCDKEntryValue(iface_mtu, conf_if_mtu);

        /* IP settings radio */
        ip_config = newCDKRadio(net_screen, (window_x + 1), (window_y + 7),
                NONE, 5, 10, "</B>IP Settings:", ip_opts, 3,
                '#' | COLOR_DIALOG_SELECT, 1,
                COLOR_DIALOG_SELECT, FALSE, FALSE);
        if (!ip_config) {
            errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
            goto cleanup;
        }
        setCDKRadioBackgroundAttrib(ip_config, COLOR_DIALOG_TEXT);
        if (strcasecmp(conf_bootproto, ip_opts[1]) == 0)
            setCDKRadioCurrentItem(ip_config, 1);
        else if (strcasecmp(conf_bootproto, ip_opts[2]) == 0)
            setCDKRadioCurrentItem(ip_config, 2);
        else
            setCDKRadioCurrentItem(ip_config, 0);

        /* A very small label for instructions */
        asprintf(&short_label_msg[0], "</B>Static IP Settings (leave blank if using DHCP)");
        short_label = newCDKLabel(net_screen, (window_x + 20), (window_y + 7),
                short_label_msg, NET_SHORT_INFO_LINES, FALSE, FALSE);
        if (!short_label) {
            errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
            goto cleanup;
        }
        setCDKLabelBackgroundAttrib(short_label, COLOR_DIALOG_TEXT);

        /* IP address field */
        ip_addy = newCDKEntry(net_screen, (window_x + 20), (window_y + 8),
                NULL, "</B>IP address: ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!ip_addy) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(ip_addy, COLOR_DIALOG_INPUT);
        setCDKEntryValue(ip_addy, conf_ipaddr);

        /* Netmask field */
        netmask = newCDKEntry(net_screen, (window_x + 20), (window_y + 9),
                NULL, "</B>Netmask:    ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!netmask) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(netmask, COLOR_DIALOG_INPUT);
        setCDKEntryValue(netmask, conf_netmask);

        /* Broadcast field */
        broadcast = newCDKEntry(net_screen, (window_x + 20), (window_y + 10),
                NULL, "</B>Broadcast:  ",
                COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                MAX_IPV4_ADDR_LEN, 0, MAX_IPV4_ADDR_LEN, FALSE, FALSE);
        if (!broadcast) {
            errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
            goto cleanup;
        }
        setCDKEntryBoxAttribute(broadcast, COLOR_DIALOG_INPUT);
        setCDKEntryValue(broadcast, conf_broadcast);

        // TODO: For now, bridging and bonding are mutually exclusive
        if (net_if_bonding[net_conf_choice] == MASTER) {
            /* If the interface is a master (bonding) they can set options */
            bond_opts = newCDKMentry(net_screen, (window_x + 1), (window_y + 11),
                    "</B>Bonding Options:", NULL,
                    COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
                    25, 2, 50, 0, TRUE, FALSE);
            if (!bond_opts) {
                errorDialog(main_cdk_screen, "Couldn't create multiple line entry widget!", NULL);
                goto cleanup;
            }
            // TODO: Some tweaking to make this widget look like the others; CDK bug?
            setCDKMentryBoxAttribute(bond_opts, COLOR_PAIR(55));
            setCDKMentryBackgroundAttrib(bond_opts, COLOR_DIALOG_TEXT);
            setCDKMentryValue(bond_opts, conf_bondopts);
            /* They can also select slaves for the bond interface */
            slave_select = newCDKSelection(net_screen, (window_x + 35), (window_y + 12),
                    RIGHT, 3, 20, "</B>Bonding Slaves:", poten_slaves, poten_slave_cnt,
                    choice_char, 2, COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!slave_select) {
                errorDialog(main_cdk_screen, "Couldn't create selection widget!", NULL);
                goto cleanup;
            }
            setCDKSelectionBackgroundAttrib(slave_select, COLOR_DIALOG_TEXT);
            /* Parse the existing slaves (if any) */
            strtok_result = strtok(conf_slaves, ",");
            while (strtok_result != NULL) {
                for (i = 0; i < poten_slave_cnt; i++) {
                    if (strstr(strStrip(strtok_result), poten_slaves[i]))
                        setCDKSelectionChoice(slave_select, i, 1);
                }
                strtok_result = strtok(NULL, ",");
            }

        } else if (net_if_bridge[net_conf_choice] == TRUE) {
            /* If the interface is a bridge interface then they can select members */
            br_member_select = newCDKSelection(net_screen, (window_x + 35), (window_y + 12),
                    RIGHT, 3, 20, "</B>Bridge Members:", poten_br_members, poten_br_member_cnt,
                    choice_char, 2, COLOR_DIALOG_SELECT, FALSE, FALSE);
            if (!br_member_select) {
                errorDialog(main_cdk_screen, "Couldn't create selection widget!", NULL);
                goto cleanup;
            }
            setCDKSelectionBackgroundAttrib(br_member_select, COLOR_DIALOG_TEXT);
            /* Parse the existing members (if any) */
            strtok_result = strtok(conf_brmembers, ",");
            while (strtok_result != NULL) {
                for (i = 0; i < poten_br_member_cnt; i++) {
                    if (strstr(strStrip(strtok_result), poten_br_members[i]))
                        setCDKSelectionChoice(br_member_select, i, 1);
                }
                strtok_result = strtok(NULL, ",");
            }
        }

        /* Buttons */
        ok_button = newCDKButton(net_screen, (window_x + 26), (window_y + 16),
                "</B>   OK   ", ok_cb, FALSE, FALSE);
        if (!ok_button) {
            errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
            goto cleanup;
        }
        setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
        cancel_button = newCDKButton(net_screen, (window_x + 36), (window_y + 16),
                "</B> Cancel ", cancel_cb, FALSE, FALSE);
        if (!cancel_button) {
            errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
            goto cleanup;
        }
        setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

        /* Allow user to traverse the screen */
        refreshCDKScreen(net_screen);
        traverse_ret = traverseCDKScreen(net_screen);

        /* User hit 'OK' button */
        if (traverse_ret == 1) {
            /* Turn the cursor off (pretty) */
            curs_set(0);

            /* Check the IP address value (field entry) */
            strncpy(temp_str, getCDKEntryValue(ip_addy), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The IP address entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the netmask value (field entry) */
            strncpy(temp_str, getCDKEntryValue(netmask), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The netmask entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            /* Check the broadcast value (field entry) */
            strncpy(temp_str, getCDKEntryValue(broadcast), MISC_STRING_LEN);
            i = 0;
            while (temp_str[i] != '\0') {
                /* If the user didn't input an acceptable name, then cancel out */
                if (!VALID_IP_ADDR_CHAR(temp_str[i])) {
                    errorDialog(main_cdk_screen,
                            "The broadcast entry field contains invalid characters!",
                            VALID_IP_ADDR_CHAR_MSG);
                    traverse_ret = 0; /* Skip the prompt */
                    goto cleanup;
                }
                i++;
            }

            if (getCDKRadioCurrentItem(ip_config) == 0) {
                /* If the user sets the interface to disabled/unconfigured
                 * then we remove the section */
                iniparser_unset(ini_dict, net_if_name[net_conf_choice]);

            } else {
                /* Network interface should be configured (static or DHCP) */
                if (iniparser_set(ini_dict, net_if_name[net_conf_choice], NULL) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:bootproto", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, ip_opts[getCDKRadioCurrentItem(ip_config)]) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:ipaddr", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, getCDKEntryValue(ip_addy)) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:netmask", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, getCDKEntryValue(netmask)) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:broadcast", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, getCDKEntryValue(broadcast)) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:mtu", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, getCDKEntryValue(iface_mtu)) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
            }

            if (net_if_bonding[net_conf_choice] == MASTER) {
                /* If its a bonding master, store bonding options */
                snprintf(bond_opts_buffer, MAX_BOND_OPTS_BUFF, "%s",
                        getCDKMentryValue(bond_opts));
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:bondopts",
                        net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, bond_opts_buffer) == -1) {
                    errorDialog(main_cdk_screen,
                            "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }

                /* For master interfaces, we need to check if any slave
                 * interfaces were selected (or removed) */
                for (i = 0; i < poten_slave_cnt; i++) {
                    if (slave_select->selections[i] == 1) {
                        asprintf(&temp_pstr, "%s,", poten_slaves[i]);
                        /* We add one extra for the null byte */
                        slave_val_size = strlen(temp_pstr) + 1;
                        slaves_line_size = slaves_line_size + slave_val_size;
                        // TODO: This totes needs to be tested (strcat)
                        if (slaves_line_size >= MAX_SLAVES_LIST_BUFF) {
                            errorDialog(main_cdk_screen,
                                    "The maximum slaves list line buffer size has been reached!",
                                    NULL);
                            freeChar(temp_pstr);
                            goto cleanup;
                        } else {
                            strcat(slaves_list_line_buffer, temp_pstr);
                            freeChar(temp_pstr);
                        }
                        /* Remove the slave interface sections from the INI file */
                        iniparser_unset(ini_dict, poten_slaves[i]);
                    }
                }
                /* Remove the trailing comma (if any) */
                if (slaves_list_line_buffer[0] != '\0') {
                    temp_int = strlen(slaves_list_line_buffer) - 1;
                    if (slaves_list_line_buffer[temp_int] == ',')
                        slaves_list_line_buffer[temp_int] = '\0';
                }
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:slaves", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, slaves_list_line_buffer) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
            }

            if (net_if_bridge[net_conf_choice] == TRUE) {
                /* For bridge interfaces, we need to check if any member
                 * interfaces were selected (or removed) */
                for (i = 0; i < poten_br_member_cnt; i++) {
                    if (br_member_select->selections[i] == 1) {
                        asprintf(&temp_pstr, "%s,", poten_br_members[i]);
                        /* We add one extra for the null byte */
                        br_member_val_size = strlen(temp_pstr) + 1;
                        br_members_line_size = br_members_line_size + br_member_val_size;
                        // TODO: This totes needs to be tested (strcat)
                        if (br_members_line_size >= MAX_BR_MEMBERS_LIST_BUFF) {
                            errorDialog(main_cdk_screen,
                                    "The maximum bridge members list line buffer size has been reached!",
                                    NULL);
                            freeChar(temp_pstr);
                            goto cleanup;
                        } else {
                            strcat(br_members_list_line_buffer, temp_pstr);
                            freeChar(temp_pstr);
                        }
                    }
                }
                /* Remove the trailing comma (if any) */
                if (br_members_list_line_buffer[0] != '\0') {
                    temp_int = strlen(br_members_list_line_buffer) - 1;
                    if (br_members_list_line_buffer[temp_int] == ',')
                        br_members_list_line_buffer[temp_int] = '\0';
                }
                /* Set the INI value */
                snprintf(temp_ini_str, MAX_INI_VAL, "%s:brmembers", net_if_name[net_conf_choice]);
                if (iniparser_set(ini_dict, temp_ini_str, br_members_list_line_buffer) == -1) {
                    errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
                    goto cleanup;
                }
            }

            /* Write to network config. file */
            ini_file = fopen(NETWORK_CONF, "w");
            if (ini_file == NULL) {
                asprintf(&error_msg, "Couldn't open network config. file for writing: %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
            } else {
                fprintf(ini_file, "# ESOS network configuration file\n");
                fprintf(ini_file, "# This file is generated by esos_tui; do not edit\n\n");
                iniparser_dump_ini(ini_dict, ini_file);
                fclose(ini_file);
                iniparser_freedict(ini_dict);
            }
        }
    }

    cleanup:
    for (i = 0; i < MAX_NET_IFACE; i++) {
        freeChar(net_scroll_msg[i]);
        freeChar(net_if_name[i]);
        freeChar(net_if_mac[i]);
        freeChar(net_if_speed[i]);
        freeChar(net_if_duplex[i]);
        freeChar(poten_slaves[i]);
    }
    if (net_screen != NULL) {
        destroyCDKScreenObjects(net_screen);
        destroyCDKScreen(net_screen);
    }
    for (i = 0; i < MAX_NET_INFO_LINES; i++)
        freeChar(net_info_msg[i]);
    for (i = 0; i < NET_SHORT_INFO_LINES; i++)
        freeChar(short_label_msg[i]);
    delwin(net_window);
    curs_set(0);
    refreshCDKScreen(main_cdk_screen);
    
    /* Ask user if they want to restart networking */
    if (traverse_ret == 1) {
        question = questionDialog(main_cdk_screen,
                "Would you like to restart networking now?", NULL);
        if (question)
            restartNetDialog(main_cdk_screen);
    }
    return;
}


/*
 * Run the Restart Networking dialog
 */
void restartNetDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *net_restart_info = 0;
    char *swindow_info[MAX_NET_RESTART_INFO_LINES] = {NULL};
    char *error_msg = NULL;
    char net_rc_cmd[MAX_SHELL_CMD_LEN] = {0}, line[NET_RESTART_INFO_COLS] = {0};
    int i = 0, status = 0;
    FILE *net_rc = NULL;
    boolean confirm = FALSE;

    /* Get confirmation (and warn user) before restarting network */
    confirm = confirmDialog(main_cdk_screen,
            "If you are connected via SSH, you may lose your session!",
            "Are you sure you want to restart networking?");
    if (!confirm)
        return;

    /* Setup scrolling window widget */
    net_restart_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            NET_RESTART_INFO_ROWS+2, NET_RESTART_INFO_COLS+2,
            "<C></31/B>Restarting networking services...\n", MAX_NET_RESTART_INFO_LINES,
            TRUE, FALSE);
    if (!net_restart_info) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(net_restart_info, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(net_restart_info, COLOR_DIALOG_BOX);
    drawCDKSwindow(net_restart_info, TRUE);

    /* Set our line counter */
    i = 0;

    /* Stop networking */
    if (i < MAX_NET_RESTART_INFO_LINES) {
        asprintf(&swindow_info[i], "</B>Stopping network:<!B>");
        addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
        i++;
    }
    snprintf(net_rc_cmd, MAX_SHELL_CMD_LEN, "%s stop", RC_NETWORK);
    net_rc = popen(net_rc_cmd, "r");
    if (!net_rc) {
        asprintf(&error_msg, "popen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    } else {
        while (fgets(line, sizeof (line), net_rc) != NULL) {
            if (i < MAX_NET_RESTART_INFO_LINES) {
                asprintf(&swindow_info[i], "%s", line);
                addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
                i++;
            }
        }
        status = pclose(net_rc);
        if (status == -1) {
            asprintf(&error_msg, "pclose(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        } else {
            if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                asprintf(&error_msg, "The %s command exited with %d.",
                        RC_NETWORK, WEXITSTATUS(status));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
        }
    }

    /* Start networking */
    if (i < MAX_NET_RESTART_INFO_LINES) {
        asprintf(&swindow_info[i], " ");
        addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
        i++;
    }
    if (i < MAX_NET_RESTART_INFO_LINES) {
        asprintf(&swindow_info[i], "</B>Starting network:<!B>");
        addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
        i++;
    }
    snprintf(net_rc_cmd, MAX_SHELL_CMD_LEN, "%s start", RC_NETWORK);
    net_rc = popen(net_rc_cmd, "r");
    if (!net_rc) {
        asprintf(&error_msg, "popen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    } else {
        while (fgets(line, sizeof (line), net_rc) != NULL) {
            if (i < MAX_NET_RESTART_INFO_LINES) {
                asprintf(&swindow_info[i], "%s", line);
                addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
                i++;
            }
        }
        status = pclose(net_rc);
        if (status == -1) {
            asprintf(&error_msg, "pclose(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        } else {
            if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                asprintf(&error_msg, "The %s command exited with %d.",
                        RC_NETWORK, WEXITSTATUS(status));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
        }
    }
    if (i < MAX_NET_RESTART_INFO_LINES) {
        asprintf(&swindow_info[i], " ");
        addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
        i++;
    }
    if (i < MAX_NET_RESTART_INFO_LINES) {
        asprintf(&swindow_info[i], CONTINUE_MSG);
        addCDKSwindow(net_restart_info, swindow_info[i], BOTTOM);
        i++;
    }

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    // TODO: Lets not scroll to the top in this one for now, its kind of annoying
    //injectCDKSwindow(net_restart_info, 'g');
    activateCDKSwindow(net_restart_info, 0);

    /* Done */
    cleanup:
    if (net_restart_info)
        destroyCDKSwindow(net_restart_info);
    for (i = 0; i < MAX_NET_RESTART_INFO_LINES; i++) {
        freeChar(swindow_info[i]);
    }
    return;
}


/*
 * Run the Mail Setup dialog
 */
void mailDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *mail_window = 0;
    CDKSCREEN *mail_screen = 0;
    CDKLABEL *mail_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *email_addr = 0, *smtp_host = 0, *smtp_port = 0,
            *auth_user = 0, *auth_pass = 0;
    CDKRADIO *use_tls = 0, *use_starttls = 0, *auth_method = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, traverse_ret = 0, window_y = 0, window_x = 0,
            mail_window_lines = 0, mail_window_cols = 0;
    static char *no_yes[] = {"No", "Yes"};
    static char *auth_method_opts[] = {"Plain Text", "CRAM-MD5"};
    static char *mail_title_msg[] = {"</31/B>System mail (SMTP) settings..."};
    char tmp_email_addr[MAX_EMAIL_LEN] = {0}, tmp_smtp_host[MAX_SMTP_LEN] = {0},
            tmp_auth_user[MAX_SMTP_USER_LEN] = {0},
            tmp_auth_pass[MAX_SMTP_PASS_LEN] = {0},
            new_mailhub[MAX_INI_VAL] = {0}, new_authmethod[MAX_INI_VAL] = {0},
            new_usetls[MAX_INI_VAL] = {0}, new_usestarttls[MAX_INI_VAL] = {0},
            hostname[MISC_STRING_LEN] = {0};
    char *conf_root = NULL, *conf_mailhub = NULL, *conf_authuser = NULL,
            *conf_authpass = NULL, *conf_authmethod = NULL,
            *conf_usetls = NULL, *conf_usestarttls = NULL,
            *mailhub_host = NULL, *mailhub_port = NULL,
            *error_msg = NULL;
    dictionary *ini_dict = NULL;
    FILE *ini_file = NULL;
    boolean question = FALSE;

    /* Read sSMTP configuration file (INI file) */
    ini_dict = iniparser_load(SSMTP_CONF);
    if (ini_dict == NULL) {
        errorDialog(main_cdk_screen, "Cannot parse sSMTP configuration file!", NULL);
        return;
    }
    conf_root = iniparser_getstring(ini_dict, ":root", "");
    conf_mailhub = iniparser_getstring(ini_dict, ":mailhub", "");
    conf_authuser = iniparser_getstring(ini_dict, ":authuser", "");
    conf_authpass = iniparser_getstring(ini_dict, ":authpass", "");
    conf_authmethod = iniparser_getstring(ini_dict, ":authmethod", "");
    conf_usetls = iniparser_getstring(ini_dict, ":usetls", "");
    conf_usestarttls = iniparser_getstring(ini_dict, ":usestarttls", "");

    /* Get the host name here (used below in sSMTP configuration) */
    if (gethostname(hostname, ((sizeof hostname) - 1)) == -1) {
        asprintf(&error_msg, "gethostname(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    
    /* Setup a new CDK screen for mail setup */
    mail_window_lines = 17;
    mail_window_cols = 68;
    window_y = ((LINES / 2) - (mail_window_lines / 2));
    window_x = ((COLS / 2) - (mail_window_cols / 2));
    mail_window = newwin(mail_window_lines, mail_window_cols,
            window_y, window_x);
    if (mail_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    mail_screen = initCDKScreen(mail_window);
    if (mail_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(mail_window, COLOR_DIALOG_BOX);
    wbkgd(mail_window, COLOR_DIALOG_TEXT);
    wrefresh(mail_window);

    /* Screen title label */
    mail_label = newCDKLabel(mail_screen, (window_x + 1), (window_y + 1),
            mail_title_msg, 1, FALSE, FALSE);
    if (!mail_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(mail_label, COLOR_DIALOG_TEXT);

    /* Email address (to send alerts to) field */
    email_addr = newCDKEntry(mail_screen, (window_x + 1), (window_y + 3),
            NULL, "</B>Alert Email Address: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            MAX_EMAIL_LEN, 0, MAX_EMAIL_LEN, FALSE, FALSE);
    if (!email_addr) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(email_addr, COLOR_DIALOG_INPUT);
    setCDKEntryValue(email_addr, conf_root);

    /* Split up mailhub string from configuration */
    mailhub_host = conf_mailhub;
    if ((mailhub_port = strchr(conf_mailhub, ':')) != NULL) {
        *mailhub_port = '\0';
        mailhub_port++;
    }

    /* SMTP host field */
    smtp_host = newCDKEntry(mail_screen, (window_x + 1), (window_y + 5),
            NULL, "</B>SMTP Host: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            24, 0, MAX_SMTP_LEN, FALSE, FALSE);
    if (!smtp_host) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(smtp_host, COLOR_DIALOG_INPUT);
    setCDKEntryValue(smtp_host, mailhub_host);

    /* SMTP port field */
    smtp_port = newCDKEntry(mail_screen, (window_x + 38), (window_y + 5),
            NULL, "</B>SMTP Port: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vINT,
            5, 0, 5, FALSE, FALSE);
    if (!smtp_port) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(smtp_port, COLOR_DIALOG_INPUT);
    if (mailhub_port != NULL)
        setCDKEntryValue(smtp_port, mailhub_port);

    /* TLS radio */
    use_tls = newCDKRadio(mail_screen, (window_x + 1), (window_y + 7),
            NONE, 5, 10, "</B>Use TLS", no_yes, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!use_tls) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(use_tls, COLOR_DIALOG_TEXT);
    if (strcasecmp(conf_usetls, "yes") == 0)
        setCDKRadioCurrentItem(use_tls, 1);
    else
        setCDKRadioCurrentItem(use_tls, 0);

    /* STARTTLS radio */
    use_starttls = newCDKRadio(mail_screen, (window_x + 15), (window_y + 7),
            NONE, 5, 10, "</B>Use STARTTLS", no_yes, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!use_starttls) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(use_starttls, COLOR_DIALOG_TEXT);
    if (strcasecmp(conf_usestarttls, "yes") == 0)
        setCDKRadioCurrentItem(use_starttls, 1);
    else
        setCDKRadioCurrentItem(use_starttls, 0);

    /* Auth. Method radio */
    auth_method = newCDKRadio(mail_screen, (window_x + 29), (window_y + 7),
            NONE, 9, 10, "</B>Auth. Method", auth_method_opts, 2,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!auth_method) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(auth_method, COLOR_DIALOG_TEXT);
    if (strcasecmp(conf_authmethod, "cram-md5") == 0)
        setCDKRadioCurrentItem(auth_method, 1);
    else
        setCDKRadioCurrentItem(auth_method, 0);

    /* Auth. User field */
    auth_user = newCDKEntry(mail_screen, (window_x + 1), (window_y + 11),
            NULL, "</B>Auth. User: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            15, 0, MAX_SMTP_USER_LEN, FALSE, FALSE);
    if (!auth_user) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(auth_user, COLOR_DIALOG_INPUT);
    setCDKEntryValue(auth_user, conf_authuser);

    /* Auth. Password field */
    auth_pass = newCDKEntry(mail_screen, (window_x + 30), (window_y + 11),
            NULL, "</B>Auth. Password: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            15, 0, MAX_SMTP_PASS_LEN, FALSE, FALSE);
    if (!auth_pass) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(auth_pass, COLOR_DIALOG_INPUT);
    setCDKEntryValue(auth_pass, conf_authpass);

    /* Buttons */
    ok_button = newCDKButton(mail_screen, (window_x + 26), (window_y + 15),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(mail_screen, (window_x + 36), (window_y + 15),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(mail_screen);
    traverse_ret = traverseCDKScreen(mail_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check email address (field entry) */
        strncpy(tmp_email_addr, getCDKEntryValue(email_addr), MAX_EMAIL_LEN);
        i = 0;
        while (tmp_email_addr[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_EMAIL_CHAR(tmp_email_addr[i])) {
                errorDialog(main_cdk_screen,
                        "The email address entry field contains invalid characters!",
                        VALID_EMAIL_CHAR_MSG);
                traverse_ret = 0; /* Skip the prompt */
                goto cleanup;
            }
            i++;
        }

        /* Check SMTP host (field entry) */
        strncpy(tmp_smtp_host, getCDKEntryValue(smtp_host), MAX_SMTP_LEN);
        i = 0;
        while (tmp_smtp_host[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_NAME_CHAR(tmp_smtp_host[i])) {
                errorDialog(main_cdk_screen,
                        "The SMTP host entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                traverse_ret = 0; /* Skip the prompt */
                goto cleanup;
            }
            i++;
        }

        /* Check auth. user (field entry) */
        strncpy(tmp_auth_user, getCDKEntryValue(auth_user), MAX_SMTP_USER_LEN);
        i = 0;
        while (tmp_auth_user[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_EMAIL_CHAR(tmp_auth_user[i])) {
                errorDialog(main_cdk_screen,
                        "The auth. user entry field contains invalid characters!",
                        VALID_EMAIL_CHAR_MSG);
                traverse_ret = 0; /* Skip the prompt */
                goto cleanup;
            }
            i++;
        }

        /* Check auth. password (field entry) */
        strncpy(tmp_auth_pass, getCDKEntryValue(auth_pass), MAX_SMTP_PASS_LEN);
        i = 0;
        while (tmp_auth_pass[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_ASCII_CHAR(tmp_auth_pass[i])) {
                errorDialog(main_cdk_screen,
                        "The auth. password entry field contains invalid characters!",
                        VALID_ASCII_CHAR_MSG);
                traverse_ret = 0; /* Skip the prompt */
                goto cleanup;
            }
            i++;
        }

        /* Set config. file */
        if (iniparser_set(ini_dict, ":root", getCDKEntryValue(email_addr)) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        snprintf(new_mailhub, MAX_INI_VAL, "%s:%s", getCDKEntryValue(smtp_host), getCDKEntryValue(smtp_port));
        if (iniparser_set(ini_dict, ":mailhub", new_mailhub) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (iniparser_set(ini_dict, ":authuser", getCDKEntryValue(auth_user)) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (iniparser_set(ini_dict, ":authpass", getCDKEntryValue(auth_pass)) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (getCDKRadioSelectedItem(auth_method) == 1)
            snprintf(new_authmethod, MAX_INI_VAL, "CRAM-MD5");
        else
            snprintf(new_authmethod, MAX_INI_VAL, " ");
        if (iniparser_set(ini_dict, ":authmethod", new_authmethod) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (getCDKRadioSelectedItem(use_tls) == 1)
            snprintf(new_usetls, MAX_INI_VAL, "YES");
        else
            snprintf(new_usetls, MAX_INI_VAL, "NO");
        if (iniparser_set(ini_dict, ":usetls", new_usetls) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (getCDKRadioSelectedItem(use_starttls) == 1)
            snprintf(new_usestarttls, MAX_INI_VAL, "YES");
        else
            snprintf(new_usestarttls, MAX_INI_VAL, "NO");
        if (iniparser_set(ini_dict, ":usestarttls", new_usestarttls) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }
        if (iniparser_set(ini_dict, ":hostname", hostname) == -1) {
            errorDialog(main_cdk_screen, "Couldn't set configuration file value!", NULL);
            goto cleanup;
        }

        /* Write the configuration file */
        ini_file = fopen(SSMTP_CONF, "w");
        if (ini_file == NULL) {
            asprintf(&error_msg, "Couldn't open sSMTP config. file for writing: %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        } else {
            /* sSMTP is very picky about its configuration file, so we can't
             * use the iniparser_dump_ini function */
            fprintf(ini_file, "# sSMTP configuration file\n");
            fprintf(ini_file, "# This file is generated by esos_tui; do not edit\n\n");
            for (i = 0; i < ini_dict->size; i++) {
                if (ini_dict->key[i] == NULL)
                    continue;
                fprintf(ini_file, "%s=%s\n", ini_dict->key[i]+1,
                        ini_dict->val[i] ? ini_dict->val[i] : "");
            }
            fclose(ini_file);
        }
    }

    cleanup:
    iniparser_freedict(ini_dict);
    if (mail_screen != NULL) {
        destroyCDKScreenObjects(mail_screen);
        destroyCDKScreen(mail_screen);
    }
    delwin(mail_window);
    curs_set(0);
    refreshCDKScreen(main_cdk_screen);
    
    /* Ask user if they want to send a test email message */
    if (traverse_ret == 1) {
        question = questionDialog(main_cdk_screen,
                "Would you like to send a test email message?", NULL);
        if (question)
            testEmailDialog(main_cdk_screen);
    }
    return;
}


/*
 * Run the Send Test Email dialog
 */
void testEmailDialog(CDKSCREEN *main_cdk_screen) {
    CDKLABEL *test_email_label = 0;
    char ssmtp_cmd[MAX_SHELL_CMD_LEN] = {0}, email_addy[MAX_EMAIL_LEN] = {0};
    char *message[5] = {NULL};
    char *error_msg = NULL, *conf_root = NULL;
    int i = 0, status = 0;
    dictionary *ini_dict = NULL;
    FILE *ssmtp = NULL;

    /* Get the email address from the config. file */
    ini_dict = iniparser_load(SSMTP_CONF);
    if (ini_dict == NULL) {
        errorDialog(main_cdk_screen, "Cannot parse sSMTP configuration file!", NULL);
        return;
    }
    conf_root = iniparser_getstring(ini_dict, ":root", "");
    if (strcmp(conf_root, "") == 0) {
        errorDialog(main_cdk_screen, "No email address is set in the configuration file!", NULL);
        return;
    }

    /* Display a nice short label message while we sync */
    snprintf(email_addy, MAX_EMAIL_LEN, "%s", conf_root);
    asprintf(&message[0], " ");
    asprintf(&message[1], " ");
    asprintf(&message[2], "</B>   Sending a test email to %s...   ", email_addy);;
    asprintf(&message[3], " ");
    asprintf(&message[4], " ");
    test_email_label = newCDKLabel(main_cdk_screen, CENTER, CENTER,
            message, 5, TRUE, FALSE);
    if (!test_email_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(test_email_label, COLOR_DIALOG_TEXT);
    setCDKLabelBoxAttribute(test_email_label, COLOR_DIALOG_BOX);
    refreshCDKScreen(main_cdk_screen);

    /* Send the test email message */
    snprintf(ssmtp_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1", SSMTP_BIN, email_addy);
    ssmtp = popen(ssmtp_cmd, "w");
    if (!ssmtp) {
        asprintf(&error_msg, "popen(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    } else {
        fprintf(ssmtp, "To: %s\n", email_addy);
        fprintf(ssmtp, "From: root\n");
        fprintf(ssmtp, "Subject: ESOS Test Email Message\n\n");
        fprintf(ssmtp, "This is an email from ESOS to verify/confirm your email settings.");
        status = pclose(ssmtp);
        if (status == -1) {
            asprintf(&error_msg, "pclose(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        } else {
            if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
                asprintf(&error_msg, "The %s command exited with %d.",
                        SSMTP_BIN, WEXITSTATUS(status));
                errorDialog(main_cdk_screen, error_msg,
                        "Check the mail log for more information.");
                freeChar(error_msg);
                goto cleanup;
            }
        }
    }

    /* Done */
    cleanup:
    iniparser_freedict(ini_dict);
    if (test_email_label)
        destroyCDKLabel(test_email_label);
    for (i = 0; i < 5; i++)
        freeChar(message[i]);
    return;
}


/*
 * Run the Add User dialog
 */
void addUserDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *add_user_window = 0;
    CDKSCREEN *add_user_screen = 0;
    CDKLABEL *add_user_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *uname_field = 0, *pass_1_field = 0, *pass_2_field = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, traverse_ret = 0, window_y = 0, window_x = 0, ret_val = 0,
            exit_stat = 0, add_user_window_lines = 0, add_user_window_cols = 0;
    static char *screen_title[] = {"</31/B>Adding a new ESOS user account..."};
    char add_user_cmd[MAX_SHELL_CMD_LEN] = {0},
            chg_pass_cmd[MAX_SHELL_CMD_LEN] = {0}, username[MAX_UNAME_LEN] = {0},
            password_1[MAX_PASSWD_LEN] = {0}, password_2[MAX_PASSWD_LEN] = {0};
    char *error_msg = NULL;
    
    /* Setup a new CDK screen for adding a new user account */
    add_user_window_lines = 12;
    add_user_window_cols = 50;
    window_y = ((LINES / 2) - (add_user_window_lines / 2));
    window_x = ((COLS / 2) - (add_user_window_cols / 2));
    add_user_window = newwin(add_user_window_lines, add_user_window_cols,
            window_y, window_x);
    if (add_user_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    add_user_screen = initCDKScreen(add_user_window);
    if (add_user_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(add_user_window, COLOR_DIALOG_BOX);
    wbkgd(add_user_window, COLOR_DIALOG_TEXT);
    wrefresh(add_user_window);

    /* Screen title label */
    add_user_label = newCDKLabel(add_user_screen, (window_x + 1), (window_y + 1),
            screen_title, 1, FALSE, FALSE);
    if (!add_user_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(add_user_label, COLOR_DIALOG_TEXT);

    /* Username field */
    uname_field = newCDKEntry(add_user_screen, (window_x + 1), (window_y + 3),
            NULL, "</B>Username: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED,
            MAX_UNAME_LEN, 0, MAX_UNAME_LEN, FALSE, FALSE);
    if (!uname_field) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(uname_field, COLOR_DIALOG_INPUT);

    /* Password field (1) */
    pass_1_field = newCDKEntry(add_user_screen, (window_x + 1), (window_y + 5),
            NULL, "</B>User Password:   ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vHMIXED,
            25, 0, MAX_PASSWD_LEN, FALSE, FALSE);
    if (!pass_1_field) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(pass_1_field, COLOR_DIALOG_INPUT);
    setCDKEntryHiddenChar(pass_1_field, '*' | COLOR_DIALOG_SELECT);
    
    /* Password field (2) */
    pass_2_field = newCDKEntry(add_user_screen, (window_x + 1), (window_y + 6),
            NULL, "</B>Retype Password: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vHMIXED,
            25, 0, MAX_PASSWD_LEN, FALSE, FALSE);
    if (!pass_2_field) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(pass_2_field, COLOR_DIALOG_INPUT);
    setCDKEntryHiddenChar(pass_2_field, '*' | COLOR_DIALOG_SELECT);

    /* Buttons */
    ok_button = newCDKButton(add_user_screen, (window_x + 17), (window_y + 10),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(add_user_screen, (window_x + 27), (window_y + 10),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(add_user_screen);
    traverse_ret = traverseCDKScreen(add_user_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check username (field entry) */
        strncpy(username, getCDKEntryValue(uname_field), MAX_UNAME_LEN);
        i = 0;
        while (username[i] != '\0') {
            /* If the user didn't input an acceptable name, then cancel out */
            if (!VALID_NAME_CHAR(username[i])) {
                errorDialog(main_cdk_screen,
                        "The username entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }
        
        /* Make sure the password fields match */
        strncpy(password_1, getCDKEntryValue(pass_1_field), MAX_PASSWD_LEN);
        strncpy(password_2, getCDKEntryValue(pass_2_field), MAX_PASSWD_LEN);
        if (strcmp(password_1, password_2) != 0) {
            errorDialog(main_cdk_screen, "The given passwords do not match!", NULL);
            goto cleanup;
        }
        
        /* Check first password field (we assume both match if we got this far) */
        i = 0;
        while (password_1[i] != '\0') {
            if (!VALID_ASCII_CHAR(password_1[i])) {
                errorDialog(main_cdk_screen,
                        "The password entry field contains invalid characters!",
                        VALID_ASCII_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }

        /* Add the new user account */
        snprintf(add_user_cmd, MAX_SHELL_CMD_LEN, "%s -h /tmp -g 'ESOS User' -s %s -G %s -D %s > /dev/null 2>&1",
                ADDUSER_BIN, SHELL_BIN, ESOS_GROUP, username);
        ret_val = system(add_user_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            asprintf(&error_msg, "Running %s failed; exited with %d.", ADDUSER_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
        
        /* Set the password for the new account */
        snprintf(chg_pass_cmd, MAX_SHELL_CMD_LEN, "echo '%s:%s' | %s -m > /dev/null 2>&1",
                username, password_1, CHPASSWD_BIN);
        ret_val = system(chg_pass_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            asprintf(&error_msg, "Running %s failed; exited with %d.", CHPASSWD_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    }

    /* Done */
    cleanup:
    if (add_user_screen != NULL) {
        destroyCDKScreenObjects(add_user_screen);
        destroyCDKScreen(add_user_screen);
    }
    delwin(add_user_window);
    return;
}


/*
 * Run the Delete User dialog
 */
void delUserDialog(CDKSCREEN *main_cdk_screen) {
    int ret_val = 0, exit_stat = 0;
    char del_user_cmd[MAX_SHELL_CMD_LEN] = {0}, del_grp_cmd[MAX_SHELL_CMD_LEN] = {0},
            user_acct[MAX_UNAME_LEN] = {0};
    char *error_msg = NULL, *confirm_msg = NULL;
    uid_t ruid = 0, euid = 0, suid = 0;
    struct passwd *passwd_entry = NULL;
    boolean confirm = FALSE;
    
    /* Have the user choose a user account */
    getUserAcct(main_cdk_screen, user_acct);
    if (user_acct[0] == '\0')
        return;
    
    /* Make sure we are not trying to delete the user that is currently logged in */
    getresuid(&ruid, &euid, &suid);
    passwd_entry = getpwuid(suid);
    if (strcmp(passwd_entry->pw_name, user_acct) == 0) {
        errorDialog(main_cdk_screen, "Can't delete the user that is currently logged in!", NULL);
        return;
    }

    /* Can't delete the ESOS superuser account */
    if (strcmp(ESOS_SUPERUSER, user_acct) == 0) {
        errorDialog(main_cdk_screen, "Can't delete the ESOS superuser account!", NULL);
        return;
    }

    /* Get a final confirmation from user before we delete */
    asprintf(&confirm_msg, "Are you sure you want to delete user %s?", user_acct);
    confirm = confirmDialog(main_cdk_screen, confirm_msg, NULL);
    freeChar(confirm_msg);
    if (confirm) {
        /* Remove the user from the ESOS users group (deluser doesn't seem to do this) */
        snprintf(del_grp_cmd, MAX_SHELL_CMD_LEN, "%s %s %s > /dev/null 2>&1",
                DELGROUP_BIN, user_acct, ESOS_GROUP);
        ret_val = system(del_grp_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            asprintf(&error_msg, "Running %s failed; exited with %d.",
                    DELGROUP_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }

        /* Delete the user account */
        snprintf(del_user_cmd, MAX_SHELL_CMD_LEN, "%s %s > /dev/null 2>&1",
                DELUSER_BIN, user_acct);
        ret_val = system(del_user_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            asprintf(&error_msg, "Running %s failed; exited with %d.",
                    DELUSER_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            return;
        }
    }
    
    /* Done */
    return;
}


/*
 * Run the Change Password dialog
 */
void chgPasswdDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *chg_pass_window = 0;
    CDKSCREEN *chg_pass_screen = 0;
    CDKLABEL *passwd_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *new_pass_1 = 0, *new_pass_2 = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, traverse_ret = 0, window_y = 0, window_x = 0, ret_val = 0,
            exit_stat = 0, chg_pass_window_lines = 0, chg_pass_window_cols = 0;
    char *screen_title[CHG_PASSWD_INFO_LINES] = {NULL};
    char chg_pass_cmd[MAX_SHELL_CMD_LEN] = {0}, password_1[MAX_PASSWD_LEN] = {0},
            password_2[MAX_PASSWD_LEN] = {0}, user_acct[MAX_UNAME_LEN] = {0};
    char *error_msg = NULL;
    
    /* Have the user choose a user account */
    getUserAcct(main_cdk_screen, user_acct);
    if (user_acct[0] == '\0')
        return;
    
    /* Setup a new CDK screen for password change */
    chg_pass_window_lines = 10;
    chg_pass_window_cols = 45;
    window_y = ((LINES / 2) - (chg_pass_window_lines / 2));
    window_x = ((COLS / 2) - (chg_pass_window_cols / 2));
    chg_pass_window = newwin(chg_pass_window_lines, chg_pass_window_cols,
            window_y, window_x);
    if (chg_pass_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    chg_pass_screen = initCDKScreen(chg_pass_window);
    if (chg_pass_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(chg_pass_window, COLOR_DIALOG_BOX);
    wbkgd(chg_pass_window, COLOR_DIALOG_TEXT);
    wrefresh(chg_pass_window);

    /* Screen title label */
    asprintf(&screen_title[0], "</31/B>Changing password for user %s...", user_acct);
    passwd_label = newCDKLabel(chg_pass_screen, (window_x + 1), (window_y + 1),
            screen_title, CHG_PASSWD_INFO_LINES, FALSE, FALSE);
    if (!passwd_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(passwd_label, COLOR_DIALOG_TEXT);

    /* New password field (1) */
    new_pass_1 = newCDKEntry(chg_pass_screen, (window_x + 1), (window_y + 3),
            NULL, "</B>New Password:    ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vHMIXED,
            20, 0, MAX_PASSWD_LEN, FALSE, FALSE);
    if (!new_pass_1) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(new_pass_1, COLOR_DIALOG_INPUT);
    setCDKEntryHiddenChar(new_pass_1, '*' | COLOR_DIALOG_SELECT);
    
    /* New password field (2) */
    new_pass_2 = newCDKEntry(chg_pass_screen, (window_x + 1), (window_y + 4),
            NULL, "</B>Retype Password: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vHMIXED,
            20, 0, MAX_PASSWD_LEN, FALSE, FALSE);
    if (!new_pass_2) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(new_pass_2, COLOR_DIALOG_INPUT);
    setCDKEntryHiddenChar(new_pass_2, '*' | COLOR_DIALOG_SELECT);

    /* Buttons */
    ok_button = newCDKButton(chg_pass_screen, (window_x + 14), (window_y + 8),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(chg_pass_screen, (window_x + 24), (window_y + 8),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(chg_pass_screen);
    traverse_ret = traverseCDKScreen(chg_pass_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Make sure the password fields match */
        strncpy(password_1, getCDKEntryValue(new_pass_1), MAX_PASSWD_LEN);
        strncpy(password_2, getCDKEntryValue(new_pass_2), MAX_PASSWD_LEN);
        if (strcmp(password_1, password_2) != 0) {
            errorDialog(main_cdk_screen, "The given passwords do not match!", NULL);
            goto cleanup;
        }
        
        /* Check first password field (we assume both match if we got this far) */
        i = 0;
        while (password_1[i] != '\0') {
            if (!VALID_ASCII_CHAR(password_1[i])) {
                errorDialog(main_cdk_screen,
                        "The new password entry field contains invalid characters!",
                        VALID_ASCII_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }

        /* Set the new password */
        snprintf(chg_pass_cmd, MAX_SHELL_CMD_LEN, "echo '%s:%s' | %s -m > /dev/null 2>&1",
                user_acct, password_1, CHPASSWD_BIN);
        ret_val = system(chg_pass_cmd);
        if ((exit_stat = WEXITSTATUS(ret_val)) != 0) {
            asprintf(&error_msg, "Running %s failed; exited with %d.", CHPASSWD_BIN, exit_stat);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    }

    /* Done */
    cleanup:
    for (i = 0; i < CHG_PASSWD_INFO_LINES; i++)
        freeChar(screen_title[i]);
    if (chg_pass_screen != NULL) {
        destroyCDKScreenObjects(chg_pass_screen);
        destroyCDKScreen(chg_pass_screen);
    }
    delwin(chg_pass_window);
    return;
}


/*
 * Run the SCST Information dialog
 */
void scstInfoDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *scst_info = 0;
    char scst_ver[MAX_SYSFS_ATTR_SIZE] = {0}, scst_setup_id[MAX_SYSFS_ATTR_SIZE] = {0},
            scst_threads[MAX_SYSFS_ATTR_SIZE] = {0}, scst_sysfs_res[MAX_SYSFS_ATTR_SIZE] = {0},
            tmp_sysfs_path[MAX_SYSFS_PATH_SIZE] = {0}, tmp_attr_line[SCST_INFO_COLS] = {0};
    char *swindow_info[MAX_SCST_INFO_LINES] = {NULL};
    char *temp_pstr = NULL;
    FILE *sysfs_file = NULL;
    int i = 0;
    
    /* Setup scrolling window widget */
    scst_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
            SCST_INFO_ROWS+2, SCST_INFO_COLS+2,
            "<C></31/B>SCST Information / Statistics\n", MAX_SCST_INFO_LINES,
            TRUE, FALSE);
    if (!scst_info) {
        errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
        return;
    }
    setCDKSwindowBackgroundAttrib(scst_info, COLOR_DIALOG_TEXT);
    setCDKSwindowBoxAttribute(scst_info, COLOR_DIALOG_BOX);

    /* Grab some semi-useful information for our scrolling window widget */
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/version", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_ver);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/setup_id", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_setup_id);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/threads", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_threads);
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/last_sysfs_mgmt_res", SYSFS_SCST_TGT);
    readAttribute(tmp_sysfs_path, scst_sysfs_res);
    
    /* Add the attribute values collected above to our scrolling window widget */
    asprintf(&swindow_info[0], "</B>Version:<!B>\t%-15s</B>Setup ID:<!B>\t\t%s", scst_ver, scst_setup_id);
    addCDKSwindow(scst_info, swindow_info[0], BOTTOM);
    asprintf(&swindow_info[1], "</B>Threads:<!B>\t%-15s</B>Last sysfs result:<!B>\t%s", scst_threads, scst_sysfs_res);
    addCDKSwindow(scst_info, swindow_info[1], BOTTOM);

     /* Loop over the SGV global statistics attribute/file in sysfs */
    asprintf(&swindow_info[2], " ");
    addCDKSwindow(scst_info, swindow_info[2], BOTTOM);
    asprintf(&swindow_info[3], "</B>Global SGV cache statistics:<!B>");
    addCDKSwindow(scst_info, swindow_info[3], BOTTOM);
    i = 4;
    snprintf(tmp_sysfs_path, MAX_SYSFS_PATH_SIZE, "%s/sgv/global_stats", SYSFS_SCST_TGT);
    if ((sysfs_file = fopen(tmp_sysfs_path, "r")) == NULL) {
        asprintf(&swindow_info[i], "fopen(): %s", strerror(errno));
        addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
    } else {
        while (fgets(tmp_attr_line, sizeof (tmp_attr_line), sysfs_file) != NULL) {
            if (i < MAX_SCST_INFO_LINES) {
                temp_pstr = strrchr(tmp_attr_line, '\n');
                if (temp_pstr)
                    *temp_pstr = '\0';
                asprintf(&swindow_info[i], "%s", tmp_attr_line);
                addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
                i++;
            }
        }
        fclose(sysfs_file);
    }
    if (i < MAX_SCST_INFO_LINES) {
        asprintf(&swindow_info[i], " ");
        addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
        i++;
    }
    if (i < MAX_SCST_INFO_LINES) {
        asprintf(&swindow_info[i], CONTINUE_MSG);
        addCDKSwindow(scst_info, swindow_info[i], BOTTOM);
        i++;
    }

    /* The 'g' makes the swindow widget scroll to the top, then activate */
    injectCDKSwindow(scst_info, 'g');
    activateCDKSwindow(scst_info, 0);

    /* We fell through -- the user exited the widget, but we don't care how */
    destroyCDKSwindow(scst_info);
    for (i = 0; i < MAX_SCST_INFO_LINES; i++) {
        freeChar(swindow_info[i]);
    }
    return;
}


/*
 * Run the CRM Status dialog
 */
void crmStatusDialog(CDKSCREEN *main_cdk_screen) {
    CDKSWINDOW *crm_info = 0;
    char *swindow_info[MAX_CRM_INFO_LINES] = {NULL};
    char *error_msg = NULL, *crm_cmd = NULL;
    int i = 0, line_pos = 0, status = 0, ret_val = 0;
    char line[CRM_INFO_COLS] = {0};
    FILE *crm_proc = NULL;

    /* Run the crm command */
    asprintf(&crm_cmd, "%s status 2>&1", CRM_TOOL);
    if ((crm_proc = popen(crm_cmd, "r")) == NULL) {
        asprintf(&error_msg, "Couldn't open process for the %s command!", CRM_TOOL);
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
    } else {
        /* Add the contents to the scrolling window widget */
        line_pos = 0;
        while (fgets(line, sizeof (line), crm_proc) != NULL) {
            if (line_pos < MAX_CRM_INFO_LINES) {
                asprintf(&swindow_info[line_pos], "%s", line);
                line_pos++;
            }
        }

        /* Add a message to the bottom explaining how to close the dialog */
        if (line_pos < MAX_CRM_INFO_LINES) {
            asprintf(&swindow_info[line_pos], " ");
            line_pos++;
        }
        if (line_pos < MAX_CRM_INFO_LINES) {
            asprintf(&swindow_info[line_pos], CONTINUE_MSG);
            line_pos++;
        }
        
        /* Close the process stream and check exit status */
        if ((status = pclose(crm_proc)) == -1) {
            ret_val = -1;
        } else {
            if (WIFEXITED(status))
                ret_val = WEXITSTATUS(status);
            else
                ret_val = -1;
        }
        if (ret_val == 0) {
            /* Setup scrolling window widget */
            crm_info = newCDKSwindow(main_cdk_screen, CENTER, CENTER,
                    CRM_INFO_ROWS+2, CRM_INFO_COLS+2,
                    "<C></31/B>CRM Status\n",
                    MAX_CRM_INFO_LINES, TRUE, FALSE);
            if (!crm_info) {
                errorDialog(main_cdk_screen, "Couldn't create scrolling window widget!", NULL);
                return;
            }
            setCDKSwindowBackgroundAttrib(crm_info, COLOR_DIALOG_TEXT);
            setCDKSwindowBoxAttribute(crm_info, COLOR_DIALOG_BOX);

            /* Set the scrolling window content */
            setCDKSwindowContents(crm_info, swindow_info, line_pos);

            /* The 'g' makes the swindow widget scroll to the top, then activate */
            injectCDKSwindow(crm_info, 'g');
            activateCDKSwindow(crm_info, 0);

            /* We fell through -- the user exited the widget, but we don't care how */
            destroyCDKSwindow(crm_info);
        } else {
            asprintf(&error_msg, "The %s command exited with %d.", CRM_TOOL, ret_val);
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
        }
    }

    /* Done */
    for (i = 0; i < MAX_CRM_INFO_LINES; i++ )
        freeChar(swindow_info[i]);
    return;
}


/*
 * Run the Date & Time Settings dialog
 */
void dateTimeDialog(CDKSCREEN *main_cdk_screen) {
    WINDOW *date_window = 0;
    CDKSCREEN *date_screen = 0;
    CDKLABEL *date_title_label = 0;
    CDKBUTTON *ok_button = 0, *cancel_button = 0;
    CDKENTRY *ntp_server = 0;
    CDKRADIO *tz_select = 0;
    CDKCALENDAR *calendar = 0;
    CDKUSCALE *hour = 0, *minute = 0, *second = 0;
    tButtonCallback ok_cb = &okButtonCB, cancel_cb = &cancelButtonCB;
    int i = 0, window_y = 0, window_x = 0, traverse_ret = 0, file_cnt = 0,
            curr_tz_item = 0, temp_int = 0, new_day = 0, new_month = 0,
            new_year = 0, new_hour = 0, new_minute = 0, new_second = 0,
            curr_day = 0, curr_month = 0, curr_year = 0, curr_hour = 0,
            curr_minute = 0, curr_second = 0, date_window_lines = 0,
            date_window_cols = 0;
    static char *date_title_msg[] = {"</31/B>Edit date and time settings..."};
    char *tz_files[MAX_TZ_FILES] = {NULL};
    char *error_msg = NULL, *remove_me = NULL, *strstr_result = NULL;
    char zoneinfo_path[MAX_ZONEINFO_PATH] = {0}, ntp_serv_val[MAX_NTP_LEN] = {0},
            new_ntp_serv_val[MAX_NTP_LEN] = {0}, dir_name[MAX_ZONEINFO_PATH] = {0};
    FILE *ntp_server_file = NULL;
    DIR *tz_base_dir = NULL, *tz_sub_dir1 = NULL, *tz_sub_dir2 = NULL;
    struct dirent *base_dir_entry = NULL, *sub_dir1_entry = NULL, *sub_dir2_entry = NULL;
    struct tm *curr_date_info = NULL;
    time_t curr_clock = 0;

    /* New CDK screen for date and time settings */
    date_window_lines = 20;
    date_window_cols = 66;
    window_y = ((LINES / 2) - (date_window_lines / 2));
    window_x = ((COLS / 2) - (date_window_cols / 2));
    date_window = newwin(date_window_lines, date_window_cols,
            window_y, window_x);
    if (date_window == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new window!", NULL);
        goto cleanup;
    }
    date_screen = initCDKScreen(date_window);
    if (date_screen == NULL) {
        errorDialog(main_cdk_screen, "Couldn't create new CDK screen!", NULL);
        goto cleanup;
    }
    boxWindow(date_window, COLOR_DIALOG_BOX);
    wbkgd(date_window, COLOR_DIALOG_TEXT);
    wrefresh(date_window);

    /* Date/time title label */
    date_title_label = newCDKLabel(date_screen, (window_x + 1), (window_y + 1),
            date_title_msg, 1, FALSE, FALSE);
    if (!date_title_label) {
        errorDialog(main_cdk_screen, "Couldn't create label widget!", NULL);
        goto cleanup;
    }
    setCDKLabelBackgroundAttrib(date_title_label, COLOR_DIALOG_TEXT);

    /* Get time zone information  -- we only traverse two directories deep */
    file_cnt = 0;
    if ((tz_base_dir = opendir(ZONEINFO)) == NULL) {
        asprintf(&error_msg, "opendir(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }
    while (((base_dir_entry = readdir(tz_base_dir)) != NULL) &&
            (file_cnt < MAX_TZ_FILES)) {
        /* We want to skip the '.' and '..' directories */
        if ((base_dir_entry->d_type == DT_DIR) &&
                (strcmp(base_dir_entry->d_name, ".") != 0) &&
                (strcmp(base_dir_entry->d_name, "..") != 0)) {
            snprintf(dir_name, MAX_ZONEINFO_PATH, "%s/%s", ZONEINFO,
                    base_dir_entry->d_name);
            if ((tz_sub_dir1 = opendir(dir_name)) == NULL) {
                asprintf(&error_msg, "opendir(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            }
            while (((sub_dir1_entry = readdir(tz_sub_dir1)) != NULL) &&
                    (file_cnt < MAX_TZ_FILES)) {
                /* We want to skip the '.' and '..' directories */
                if ((sub_dir1_entry->d_type == DT_DIR) &&
                        (strcmp(sub_dir1_entry->d_name, ".") != 0) &&
                        (strcmp(sub_dir1_entry->d_name, "..") != 0)) {
                    snprintf(dir_name, MAX_ZONEINFO_PATH, "%s/%s/%s", ZONEINFO,
                            base_dir_entry->d_name, sub_dir1_entry->d_name);
                    if ((tz_sub_dir2 = opendir(dir_name)) == NULL) {
                        asprintf(&error_msg, "opendir(): %s", strerror(errno));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        freeChar(error_msg);
                        goto cleanup;
                    }
                    while (((sub_dir2_entry = readdir(tz_sub_dir2)) != NULL) &&
                            (file_cnt < MAX_TZ_FILES)) {
                        if (sub_dir2_entry->d_type == DT_REG) {
                            asprintf(&tz_files[file_cnt], "%s/%s/%s",
                                    base_dir_entry->d_name,
                                    sub_dir1_entry->d_name,
                                    sub_dir2_entry->d_name);
                            file_cnt++;
                        }
                    }
                    closedir(tz_sub_dir2);
                } else if (sub_dir1_entry->d_type == DT_REG) {
                    asprintf(&tz_files[file_cnt], "%s/%s",
                            base_dir_entry->d_name, sub_dir1_entry->d_name);
                    file_cnt++;
                }
            }
            closedir(tz_sub_dir1);
        } else if (base_dir_entry->d_type == DT_REG) {
            asprintf(&tz_files[file_cnt], "%s", base_dir_entry->d_name);
            file_cnt++;
        }
    }
    closedir(tz_base_dir);

    /* A radio widget for displaying/choosing time zone */
    tz_select = newCDKRadio(date_screen, (window_x + 1), (window_y + 3),
            NONE, 12, 34, "</B>Time Zone\n", tz_files, file_cnt,
            '#' | COLOR_DIALOG_SELECT, 1,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!tz_select) {
        errorDialog(main_cdk_screen, "Couldn't create radio widget!", NULL);
        goto cleanup;
    }
    setCDKRadioBackgroundAttrib(tz_select, COLOR_DIALOG_TEXT);
    
    /* Get the current time zone data file path (from sym. link) */
    if (readlink(LOCALTIME, zoneinfo_path, MAX_ZONEINFO_PATH) == -1) {
        asprintf(&error_msg, "readlink(): %s", strerror(errno));
        errorDialog(main_cdk_screen, error_msg, NULL);
        freeChar(error_msg);
        goto cleanup;
    }

    /* Parse the current time zone link target path and set the radio item */
    strstr_result = strstr(zoneinfo_path, ZONEINFO);
    if (strstr_result) {
        strstr_result = strstr_result + (sizeof(ZONEINFO) - 1);
        if (*strstr_result == '/')
            strstr_result++;
        for (i = 0; i < MAX_TZ_FILES; i++) {
            if (strcmp(tz_files[i], strstr_result) == 0) {
                setCDKRadioCurrentItem(tz_select, i);
                curr_tz_item = i;
                break;
            }
        }
    } else {
        setCDKRadioCurrentItem(tz_select, 0);
    }

    /* Get current date/time information */
    time(&curr_clock);
    curr_date_info = localtime(&curr_clock);
    curr_day = curr_date_info->tm_mday;
    curr_month = curr_date_info->tm_mon + 1;
    curr_year = curr_date_info->tm_year + 1900;
    curr_hour = curr_date_info->tm_hour;
    curr_minute = curr_date_info->tm_min;
    curr_second = curr_date_info->tm_sec;

    /* Calendar widget for displaying/setting current date */
    calendar = newCDKCalendar(date_screen, (window_x + 39), (window_y + 3),
            "</B>Current Date", curr_day, curr_month, curr_year,
            COLOR_DIALOG_TEXT, COLOR_DIALOG_TEXT, COLOR_DIALOG_TEXT,
            COLOR_DIALOG_SELECT, FALSE, FALSE);
    if (!calendar) {
        errorDialog(main_cdk_screen, "Couldn't create calendar widget!", NULL);
        goto cleanup;
    }
    setCDKCalendarBackgroundAttrib(calendar, COLOR_DIALOG_TEXT);
    
    /* Hour, minute, second scale widgets */
    hour = newCDKUScale(date_screen, (window_x + 39), (window_y + 15),
            "</B>Hour  ", NULL, COLOR_DIALOG_INPUT, 3, curr_hour, 0, 23,
            1, 5, FALSE, FALSE);
    if (!hour) {
        errorDialog(main_cdk_screen, "Couldn't create scale widget!", NULL);
        goto cleanup;
    }
    setCDKUScaleBackgroundAttrib(hour, COLOR_DIALOG_TEXT);
    minute = newCDKUScale(date_screen, (window_x + 47), (window_y + 15),
            "</B>Minute", NULL, COLOR_DIALOG_INPUT, 3, curr_minute, 0, 59,
            1, 5, FALSE, FALSE);
    if (!minute) {
        errorDialog(main_cdk_screen, "Couldn't create scale widget!", NULL);
        goto cleanup;
    }
    setCDKUScaleBackgroundAttrib(minute, COLOR_DIALOG_TEXT);
    second = newCDKUScale(date_screen, (window_x + 55), (window_y + 15),
            "</B>Second", NULL, COLOR_DIALOG_INPUT, 3, curr_second, 0, 59,
            1, 5, FALSE, FALSE);
    if (!second) {
        errorDialog(main_cdk_screen, "Couldn't create scale widget!", NULL);
        goto cleanup;
    }
    setCDKUScaleBackgroundAttrib(second, COLOR_DIALOG_TEXT);
    
    /* NTP server */
    ntp_server = newCDKEntry(date_screen, (window_x + 1), (window_y + 16),
            NULL, "</B>NTP Server: ",
            COLOR_DIALOG_SELECT, '_' | COLOR_DIALOG_INPUT, vMIXED, 20,
            0, MAX_NTP_LEN, FALSE, FALSE);
    if (!ntp_server) {
        errorDialog(main_cdk_screen, "Couldn't create entry widget!", NULL);
        goto cleanup;
    }
    setCDKEntryBoxAttribute(ntp_server, COLOR_DIALOG_INPUT);
    
    /* Get the current NTP server setting (if any) and set widget */
    if ((ntp_server_file = fopen(NTP_SERVER, "r")) == NULL) {
        /* ENOENT is okay since its possible this file doesn't exist yet */
        if (errno != ENOENT) {
            asprintf(&error_msg, "fopen(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    } else {
        fgets(ntp_serv_val, MAX_NTP_LEN, ntp_server_file);
        fclose(ntp_server_file);
        remove_me = strrchr(ntp_serv_val, '\n');
        if (remove_me)
            *remove_me = '\0';
        setCDKEntryValue(ntp_server, ntp_serv_val);
    }

    /* Buttons */
    ok_button = newCDKButton(date_screen, (window_x + 24), (window_y + 18),
            "</B>   OK   ", ok_cb, FALSE, FALSE);
    if (!ok_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(ok_button, COLOR_DIALOG_INPUT);
    cancel_button = newCDKButton(date_screen, (window_x + 34), (window_y + 18),
            "</B> Cancel ", cancel_cb, FALSE, FALSE);
    if (!cancel_button) {
        errorDialog(main_cdk_screen, "Couldn't create button widget!", NULL);
        goto cleanup;
    }
    setCDKButtonBackgroundAttrib(cancel_button, COLOR_DIALOG_INPUT);

    /* Allow user to traverse the screen */
    refreshCDKScreen(date_screen);
    traverse_ret = traverseCDKScreen(date_screen);

    /* User hit 'OK' button */
    if (traverse_ret == 1) {
        /* Turn the cursor off (pretty) */
        curs_set(0);

        /* Check time zone radio */
        temp_int = getCDKRadioSelectedItem(tz_select);
        /* If the time zone setting was changed, create a new sym. link */
        if (temp_int != curr_tz_item) {
            if (unlink(LOCALTIME) == -1) {
                asprintf(&error_msg, "unlink(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            } else {
                snprintf(dir_name, MAX_ZONEINFO_PATH, "%s/%s",
                        ZONEINFO, tz_files[temp_int]);
                if (symlink(dir_name, LOCALTIME) == -1) {
                    asprintf(&error_msg, "symlink(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    goto cleanup;
                }
            }
        }

        /* Check NTP server setting (field entry) */
        strncpy(new_ntp_serv_val, getCDKEntryValue(ntp_server), MAX_NTP_LEN);
        i = 0;
        while (new_ntp_serv_val[i] != '\0') {
            /* If the user didn't input an acceptable value, then cancel out */
            if (!VALID_NAME_CHAR(new_ntp_serv_val[i])) {
                errorDialog(main_cdk_screen,
                        "The NTP server entry field contains invalid characters!",
                        VALID_NAME_CHAR_MSG);
                goto cleanup;
            }
            i++;
        }

        /* If the value has changed, write it to the file */
        if (strcmp(ntp_serv_val, new_ntp_serv_val) != 0) {
            if ((ntp_server_file = fopen(NTP_SERVER, "w+")) == NULL) {
                asprintf(&error_msg, "fopen(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                freeChar(error_msg);
                goto cleanup;
            } else {
                fprintf(ntp_server_file, "%s", new_ntp_serv_val);
                if (fclose(ntp_server_file) != 0) {
                    asprintf(&error_msg, "fclose(): %s", strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    freeChar(error_msg);
                    goto cleanup;
                }
            }
        }
        
        /* Get/check date/time settings */
        getCDKCalendarDate(calendar, &new_day, &new_month, &new_year);
        //new_month = new_month - 1;
        //new_year = new_year + (2000 - 1900);
        new_hour = getCDKUScaleValue(hour);
        new_minute = getCDKUScaleValue(minute);
        new_second = getCDKUScaleValue(second);
        if (new_day != curr_day)
            curr_date_info->tm_mday = new_day;
        if (new_month != curr_month)
            curr_date_info->tm_mon = new_month - 1;
        if (new_year != curr_year)
            curr_date_info->tm_year = new_year - 1900;
        if (new_hour != curr_hour)
            curr_date_info->tm_hour = new_hour;
        if (new_minute != curr_minute)
            curr_date_info->tm_min = new_minute;
        if (new_second != curr_second)
            curr_date_info->tm_sec = new_second;
        /* Set date & time */
        const struct timeval time_val = {mktime(curr_date_info), 0};
        if (settimeofday(&time_val, 0) == -1) {
            asprintf(&error_msg, "settimeofday(): %s", strerror(errno));
            errorDialog(main_cdk_screen, error_msg, NULL);
            freeChar(error_msg);
            goto cleanup;
        }
    }

    /* All done -- clean up */
    cleanup:
    if (date_screen != NULL) {
        destroyCDKScreenObjects(date_screen);
        destroyCDKScreen(date_screen);
    }
    delwin(date_window);
    for (i = 0; i < MAX_TZ_FILES; i++)
        freeChar(tz_files[i]);
    return;
}
