/**
 * @file ndp.c
 * @brief NDP and ICMPv6 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "icmp6.h"
#include "ndp.h"
#include "scanner.h"

extern int terminator;

/**
 * @brief Gets info about interface
 * @param sock Raw socket
 * @param ifname Interface
 * @param idx Interface index
 * @param mac MAC
 * @param ip IPv6
 */
void get_info6(int sock, char *ifname, int *idx, uint8_t *mac,
               struct in6_addr *ip) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

  if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
    throw_error("Interface doesn't exist");
  }
  *idx = ifr.ifr_ifindex;

  if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
    throw_error("Interface's MAC doesn't exist");
  }
  memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

  struct ifaddrs *ifaddr, *ifa;
  getifaddrs(&ifaddr);
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6 &&
        strcmp(ifa->ifa_name, ifname) == 0) {
      memcpy(ip, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, 16);
      break;
    }
  }
  freeifaddrs(ifaddr);
}

/**
 * @brief Sends NDP Neighbor Solicitation
 * @param sock Raw socket
 * @param idx Interface index
 * @param src_mac Source MAC
 * @param src_ip Source IPv6
 * @param target_ip Target IPv6
 */
void ndp_ping(int sock, int idx, uint8_t *src_mac, struct in6_addr *src_ip,
              struct in6_addr *target_ip) {
  uint8_t buf[sizeof(struct ethhdr) + sizeof(struct ip6_hdr) +
              sizeof(struct nd_neighbor_solicit) + 8];
  memset(buf, 0, sizeof(buf));

  struct ethhdr *eth = (struct ethhdr *)buf;
  struct ip6_hdr *ip6 = (struct ip6_hdr *)(buf + sizeof(struct ethhdr));
  struct nd_neighbor_solicit *ns = (struct nd_neighbor_solicit *)(ip6 + 1);

  eth->h_dest[0] = 0x33;
  eth->h_dest[1] = 0x33;
  eth->h_dest[2] = 0xff;
  memcpy(&eth->h_dest[3], &target_ip->s6_addr[13], 3);
  memcpy(eth->h_source, src_mac, ETH_ALEN);
  eth->h_proto = htons(ETH_P_IPV6);

  ip6->ip6_vfc = 0x60;
  ip6->ip6_plen = htons(sizeof(struct nd_neighbor_solicit) + 8);
  ip6->ip6_nxt = IPPROTO_ICMPV6;
  ip6->ip6_hlim = 255;
  memcpy(&ip6->ip6_src, src_ip, 16);
  ip6->ip6_dst.s6_addr[0] = 0xff;
  ip6->ip6_dst.s6_addr[1] = 0x02;
  ip6->ip6_dst.s6_addr[11] = 0x01;
  ip6->ip6_dst.s6_addr[12] = 0xff;
  memcpy(&ip6->ip6_dst.s6_addr[13], &target_ip->s6_addr[13], 3);

  ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
  memcpy(&ns->nd_ns_target, target_ip, 16);
  uint8_t *opt = (uint8_t *)(ns + 1);
  opt[0] = ND_OPT_SOURCE_LINKADDR;
  opt[1] = 1;
  memcpy(&opt[2], src_mac, ETH_ALEN);

  ns->nd_ns_cksum = icmp6_checksum(&ip6->ip6_src, &ip6->ip6_dst, ns,
                                   sizeof(struct nd_neighbor_solicit) + 8);

  struct sockaddr_ll sa = {
      .sll_family = AF_PACKET, .sll_ifindex = idx, .sll_halen = ETH_ALEN};
  memcpy(sa.sll_addr, eth->h_dest, ETH_ALEN);
  sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&sa, sizeof(sa));
}

/**
 * @brief Waits for NDP Advertisement reply
 * @param sock Raw socket
 * @param target_ip Target IPv6
 * @param target_mac Target MAC
 * @param timeout Timeout
 * @return true if advertisement received, false otherwise
 */
bool ndp_wait(int sock, struct in6_addr *target_ip, uint8_t *target_mac,
              int timeout) {
  struct pollfd pfd = {.fd = sock, .events = POLLIN};
  uint8_t buf[512];

  while (!terminator && poll(&pfd, 1, timeout) > 0) {
    ssize_t len = recv(sock, buf, sizeof(buf), 0);
    if (len < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct ip6_hdr) +
                        sizeof(struct nd_neighbor_advert))) {
      continue;
    }

    struct ethhdr *eth = (struct ethhdr *)buf;
    struct ip6_hdr *ip6 = (struct ip6_hdr *)(buf + sizeof(struct ethhdr));
    struct nd_neighbor_advert *na = (struct nd_neighbor_advert *)(ip6 + 1);

    if (eth->h_proto == htons(ETH_P_IPV6) && ip6->ip6_nxt == IPPROTO_ICMPV6 &&
        na->nd_na_type == ND_NEIGHBOR_ADVERT &&
        memcmp(&na->nd_na_target, target_ip, sizeof(struct in6_addr)) == 0) {
      uint8_t *opt = (uint8_t *)(na + 1);
      if (opt[0] == ND_OPT_TARGET_LINKADDR) {
        memcpy(target_mac, &opt[2], ETH_ALEN);
        return true;
      }
    }
  }
  return false;
}

/**
 * @brief NDP and ICMPv6 scanning
 * @param interface Interface
 * @param s Subnet
 * @param timeout Timeout
 */
void scan_ndp(char *interface, char *s, int timeout) {
  char *temp = malloc(strlen(s) + 1);
  if (!temp) {
    throw_error("Memory allocation failed");
  }
  strcpy(temp, s);

  char *slash = strchr(temp, '/');
  if (slash == NULL) {
    throw_error("Missing prefix in subnet");
  }

  *slash = '\0';
  Subnet info;
  get_subnet6(temp, atoi(slash + 1), &info);
  free(temp);

  int n_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IPV6));
  if (n_sock < 0) {
    throw_error("Failed to create ARP socket");
  }

  int i_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  if (i_sock < 0) {
    throw_error("Failed to create ICMP socket");
  }

  int idx;
  uint8_t source_mac[ETH_ALEN];
  struct in6_addr source_ip;
  get_info6(n_sock, interface, &idx, source_mac, &source_ip);

  struct in6_addr network_base;
  memcpy(&network_base, info.net, 16);

  for (unsigned long long i = 0; i < info.hosts; i++) {
    if (terminator) {
      break;
    }

    struct in6_addr target_ip = network_base;
    unsigned long long skip = (info.prefix < 128) ? i + 1 : i;
    for (int j = 15; j >= 0 && skip > 0; j--) {
      int add = target_ip.s6_addr[j] + (skip & 0xFF);
      target_ip.s6_addr[j] = (uint8_t)(add & 0xFF);
      skip >>= 8;
      if (add > 0xFF)
        skip++;
    }

    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &target_ip, addr_str, INET6_ADDRSTRLEN);
    printf("%s ", addr_str);

    ndp_ping(n_sock, idx, source_mac, &source_ip, &target_ip);
    uint8_t target_mac[ETH_ALEN];
    bool n_ok = ndp_wait(n_sock, &target_ip, target_mac, timeout);

    bool i_ok = false;
    if (!terminator) {
      i_ok = icmp6_ping(i_sock, &target_ip, timeout, idx);
    }

    printf("ndp %s", n_ok ? "OK" : "FAIL");
    if (n_ok) {
      printf(" (%02x-%02x-%02x-%02x-%02x-%02x)", target_mac[0], target_mac[1],
             target_mac[2], target_mac[3], target_mac[4], target_mac[5]);
    }
    printf(", icmpv6 %s\n", i_ok ? "OK" : "FAIL");

    if (terminator) {
      break;
    }
  }

  close(n_sock);
  close(i_sock);
}
