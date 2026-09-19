/**
 * @file ndp.h
 * @brief Header for NDP scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef NDP_H
#define NDP_H

#include "utils.h"

void scan_ndp(char *interface, char *s, int timeout);
void get_info6(int sock, char *ifname, int *idx, uint8_t *mac,
               struct in6_addr *ip);
void ndp_ping(int sock, int idx, uint8_t *src_mac, struct in6_addr *src_ip,
              struct in6_addr *target_ip);
bool ndp_wait(int sock, struct in6_addr *target_ip, uint8_t *target_mac,
              int timeout);

#endif
