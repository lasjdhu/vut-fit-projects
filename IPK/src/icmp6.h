/**
 * @file icmp6.h
 * @brief Header for ICMPv6 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef ICMP6_H
#define ICMP6_H

#include "utils.h"

bool icmp6_ping(int sock, struct in6_addr *target_ip, int timeout, int idx);
bool icmp6_wait(int sock, int timeout, uint16_t id);
uint16_t icmp6_checksum(struct in6_addr *src, struct in6_addr *dst, void *data,
                        int len);

#endif
