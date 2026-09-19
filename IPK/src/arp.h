/**
 * @file arp.h
 * @brief Header for ARP scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef ARP_H
#define ARP_H

#include "utils.h"

void scan_arp(char *interface, char *s, int timeout);
void get_info(int sock, char *ifname, int *idx, uint8_t *mac, uint32_t *ip);
void arp_ping(int sock, int idx, uint8_t *src_mac, uint32_t src_ip,
              uint32_t target_ip);
bool arp_wait(int sock, uint32_t target_ip, uint8_t *target_mac, int timeout);

#endif
