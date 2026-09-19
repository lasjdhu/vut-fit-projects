/**
 * @file scanner.c
 * @brief Scanning processor
 * @author Dmitrii Ivanushkin xivanu00
 */

#include <arpa/inet.h>
#include <ifaddrs.h>

#include "arp.h"
#include "ndp.h"
#include "scanner.h"

const uint8_t MASK[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};

/**
 * @brief Scanning processor
 * @param config Configuration structure
 */
void scan(Config *config) {
  if (config->list_interfaces) {
    list_interfaces();
  }

  int count = 0;
  char **interfaces = get_allowed_interfaces(&count);
  bool is_valid = false;

  for (int i = 0; i < count; i++) {
    if (strcmp(interfaces[i], config->interface) == 0) {
      is_valid = true;
    }
    free(interfaces[i]);
  }
  free(interfaces);

  if (!is_valid) {
    throw_error("Specified interface was not found");
  }

  printf("Scanning ranges:\n");
  for (int i = 0; i < config->subnet_count; i++) {
    print_subnets(config->subnets[i]);
  }
  printf("\n");

  for (int i = 0; i < config->subnet_count; i++) {
    if (strchr(config->subnets[i], ':')) {
      scan_ndp(config->interface, config->subnets[i], config->timeout);
    } else {
      scan_arp(config->interface, config->subnets[i], config->timeout);
    }
  }
}

/**
 * @brief Gets all interfaces
 * @param count Number of interfaces
 * @return Array of interfaces
 */
char **get_allowed_interfaces(int *count) {
  struct ifaddrs *addrs, *ifa;
  *count = 0;

  if (getifaddrs(&addrs) == -1) {
    throw_error("Failed to get interfaces");
  }

  char **interfaces = malloc(MAX_INTERFACES * sizeof(char *));
  if (interfaces == NULL) {
    throw_error("Memory allocation failed");
  }

  ifa = addrs;
  while (ifa != NULL) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET) {
      interfaces[*count] = malloc(strlen(ifa->ifa_name) + 1);
      if (interfaces[*count] == NULL) {
        throw_error("Memory allocation failed");
      }
      strcpy(interfaces[*count], ifa->ifa_name);
      (*count)++;
    }
    ifa = ifa->ifa_next;
  }

  freeifaddrs(addrs);
  return interfaces;
}

/**
 * @brief Lists all interfaces
 */
void list_interfaces() {
  int count = 0;
  char **interfaces = get_allowed_interfaces(&count);

  for (int i = 0; i < count; i++) {
    printf("%s\n", interfaces[i]);
    free(interfaces[i]);
  }
  free(interfaces);

  exit(EXIT_SUCCESS);
}

/**
 * @brief Prints subnet information
 * @param s Subnet string
 */
void print_subnets(char *s) {
  char *slash = strchr(s, '/');
  if (slash == NULL) {
    throw_error("Missing prefix in subnet");
  }

  *slash = '\0';
  int prefix = atoi(slash + 1);
  Subnet info;

  if (strchr(s, ':')) {
    get_subnet6(s, prefix, &info);
  } else {
    get_subnet4(s, prefix, &info);
  }

  char net[INET6_ADDRSTRLEN];
  inet_ntop(info.family, info.net, net, sizeof(net));
  printf("%s/%d %llu\n", net, info.prefix, info.hosts);

  *slash = '/';
}

/**
 * @brief Calculates IPv4 subnet information from address and prefix
 * @param s IPv4 address
 * @param prefix Prefix length
 * @param info Subnet structure
 */
void get_subnet4(char *s, int prefix, Subnet *info) {
  struct in_addr addr;
  if (inet_pton(AF_INET, s, &addr) != 1) {
    throw_error("Invalid IPv4 address");
  }

  uint8_t *ip_bytes = (uint8_t *)&addr.s_addr;
  int p = prefix;
  for (int i = 0; i < (int)sizeof(struct in_addr); i++) {
    if (p >= 8) {
      p -= 8;
    } else if (p > 0) {
      ip_bytes[i] &= MASK[p];
      p = 0;
    } else {
      ip_bytes[i] = 0;
    }
  }

  info->family = AF_INET;
  info->prefix = prefix;
  memcpy(info->net, &addr.s_addr, sizeof(struct in_addr));

  if (prefix == 32) {
    info->hosts = 1;
  } else if (prefix == 31) {
    info->hosts = 2;
  } else {
    info->hosts = (1ULL << (32 - prefix)) - 2;
  }
}

/**
 * @brief Calculates IPv6 subnet information from address and prefix
 * @param s IPv6 address
 * @param prefix Prefix length
 * @param info Subnet structure
 */
void get_subnet6(char *s, int prefix, Subnet *info) {
  struct in6_addr addr;
  if (inet_pton(AF_INET6, s, &addr) != 1) {
    throw_error("Invalid IPv6 address");
  }

  int p = prefix;
  for (int i = 0; i < (int)sizeof(struct in6_addr); i++) {
    if (p >= 8) {
      p -= 8;
    } else if (p > 0) {
      addr.s6_addr[i] &= MASK[p];
      p = 0;
    } else {
      addr.s6_addr[i] = 0;
    }
  }

  info->family = AF_INET6;
  info->prefix = prefix;
  memcpy(info->net, &addr.s6_addr, sizeof(struct in6_addr));

  if (prefix == 128) {
    info->hosts = 1;
  } else if (prefix > 64) {
    info->hosts = (1ULL << (128 - prefix)) - 1;
  } else {
    info->hosts = ~0ULL;
  }
}
