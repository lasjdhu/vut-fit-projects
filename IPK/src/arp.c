/**
 * @file arp.c
 * @brief ARP and ICMPv4 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "arp.h"
#include "icmp4.h"
#include "scanner.h"

extern int terminator;

/**
 * @brief Gets info about interface
 * @param sock Raw socket
 * @param ifname Interface
 * @param idx Interface index
 * @param mac MAC
 * @param ip IPv4
 */
void get_info(int sock, char *ifname, int *idx, uint8_t *mac, uint32_t *ip) {
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

  if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
    throw_error("Interface's IP doesn't exist");
  }
  *ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

/**
 * @brief Sends ARP request
 * @param sock Raw socket
 * @param idx Interface index
 * @param src_mac Source MAC
 * @param src_ip Source IPv6
 * @param target_ip Target IPv6
 */
void arp_ping(int sock, int idx, uint8_t *src_mac, uint32_t src_ip,
              uint32_t target_ip) {
  uint8_t buf[sizeof(struct ethhdr) + sizeof(struct ether_arp)];
  memset(buf, 0, sizeof(buf));

  struct ethhdr *eth = (struct ethhdr *)buf;
  struct ether_arp *arp = (struct ether_arp *)(buf + sizeof(struct ethhdr));

  memset(eth->h_dest, 0xff, ETH_ALEN);
  memcpy(eth->h_source, src_mac, ETH_ALEN);
  eth->h_proto = htons(ETH_P_ARP);

  arp->arp_hrd = htons(ARPHRD_ETHER);
  arp->arp_pro = htons(ETH_P_IP);
  arp->arp_hln = ETH_ALEN;
  arp->arp_pln = 4;
  arp->arp_op = htons(ARPOP_REQUEST);

  memcpy(arp->arp_sha, src_mac, ETH_ALEN);
  memcpy(arp->arp_spa, &src_ip, 4);
  memset(arp->arp_tha, 0x00, ETH_ALEN);
  memcpy(arp->arp_tpa, &target_ip, 4);

  struct sockaddr_ll sa = {
      .sll_family = AF_PACKET, .sll_ifindex = idx, .sll_halen = ETH_ALEN};
  memset(sa.sll_addr, 0xff, ETH_ALEN);

  sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&sa, sizeof(sa));
}

/**
 * @brief Waits for ARP reply
 * @param sock Raw socket
 * @param target_ip Target IPv4
 * @param target_mac Target MAC
 * @param timeout Timeout
 * @return true if reply received, false otherwise.
 */
bool arp_wait(int sock, uint32_t target_ip, uint8_t *target_mac, int timeout) {
  struct pollfd pfd = {.fd = sock, .events = POLLIN};
  uint8_t buf[512];

  while (!terminator && poll(&pfd, 1, timeout) > 0) {
    ssize_t len = recv(sock, buf, sizeof(buf), 0);
    if (len < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct ether_arp))) {
      continue;
    }

    struct ethhdr *eth = (struct ethhdr *)buf;
    struct ether_arp *arp = (struct ether_arp *)(buf + sizeof(struct ethhdr));

    if (eth->h_proto == htons(ETH_P_ARP) && arp->arp_op == htons(ARPOP_REPLY) &&
        memcmp(arp->arp_spa, &target_ip, sizeof(target_ip)) == 0) {
      memcpy(target_mac, arp->arp_sha, ETH_ALEN);
      return true;
    }
  }
  return false;
}

/**
 * @brief ARP and ICMPv4 scanning
 * @param interface Interface
 * @param s Subnet
 * @param timeout Timeout
 */
void scan_arp(char *interface, char *s, int timeout) {
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
  get_subnet4(temp, atoi(slash + 1), &info);
  free(temp);

  uint32_t net;
  memcpy(&net, info.net, 4);
  uint32_t start = (info.prefix <= 30) ? ntohl(net) + 1 : ntohl(net);

  int a_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if (a_sock < 0) {
    throw_error("Failed to create ARP socket");
  }

  int i_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (i_sock < 0) {
    throw_error("Failed to create ICMP socket");
  }

  int idx;
  uint8_t source_mac[ETH_ALEN];
  uint32_t source_ip;
  get_info(a_sock, interface, &idx, source_mac, &source_ip);

  for (unsigned long long i = 0; i < info.hosts; i++) {
    if (terminator) {
      break;
    }

    uint32_t target_ip = htonl(start + i);
    struct in_addr ta = {.s_addr = target_ip};
    printf("%s ", inet_ntoa(ta));

    arp_ping(a_sock, idx, source_mac, source_ip, target_ip);
    uint8_t target_mac[ETH_ALEN];
    bool a_ok = arp_wait(a_sock, target_ip, target_mac, timeout);

    bool i_ok = false;
    if (!terminator) {
      i_ok = icmp4_ping(i_sock, target_ip, timeout);
    }

    printf("arp %s", a_ok ? "OK" : "FAIL");
    if (a_ok) {
      printf(" (%02x-%02x-%02x-%02x-%02x-%02x)", target_mac[0], target_mac[1],
             target_mac[2], target_mac[3], target_mac[4], target_mac[5]);
    }
    printf(", icmpv4 %s\n", i_ok ? "OK" : "FAIL");

    if (terminator) {
      break;
    }
  }

  close(a_sock);
  close(i_sock);
}
