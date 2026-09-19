/**
 * @file icmp4.h
 * @brief Header for ICMPv4 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef ICMP_H
#define ICMP_H

#include "utils.h"

bool icmp4_ping(int sock, uint32_t target_ip, int timeout);
bool icmp4_wait(int sock, int timeout, uint16_t id, uint32_t target_ip);
uint16_t icmp4_checksum(void *data, int len);

#endif
