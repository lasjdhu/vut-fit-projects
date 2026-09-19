/**
 * @file scanner.h
 * @brief Header for scanner
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "utils.h"

#define MAX_INTERFACES 128

extern const uint8_t MASK[8];

/**
 * @brief Network subnet structure
 */
typedef struct {
  int family;
  int prefix;
  uint8_t net[16];
  unsigned long long hosts;
} Subnet;

void scan(Config *config);
char **get_allowed_interfaces(int *count);
void list_interfaces();
void print_subnets(char *s);
void get_subnet4(char *ip_str, int prefix, Subnet *info);
void get_subnet6(char *ip_str, int prefix, Subnet *info);

#endif
