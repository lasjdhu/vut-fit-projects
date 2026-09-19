/**
 * @file icmp6.c
 * @brief ICMPv6 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include "icmp6.h"

extern int terminator;

/**
 * @brief Sends an ICMPv6 echo request
 * @param sock Raw socket
 * @param target_ip Target IPv6
 * @param timeout Timeout
 * @param idx Interface index
 * @return true if echo reply received, false otherwise
 */
bool icmp6_ping(int sock, struct in6_addr *target_ip, int timeout, int idx) {
  struct icmp6_hdr icmp6;
  memset(&icmp6, 0, sizeof(icmp6));

  icmp6.icmp6_type = ICMP6_ECHO_REQUEST;
  icmp6.icmp6_code = 0;
  icmp6.icmp6_id = htons(getpid() & 0xFFFF);
  icmp6.icmp6_seq = 0;
  icmp6.icmp6_cksum = 0;

  struct sockaddr_in6 sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin6_family = AF_INET6;
  sa.sin6_scope_id = idx;
  memcpy(&sa.sin6_addr, target_ip, 16);

  sendto(sock, &icmp6, sizeof(icmp6), 0, (struct sockaddr *)&sa, sizeof(sa));

  return icmp6_wait(sock, timeout, icmp6.icmp6_id);
}

/**
 * @brief Waits for ICMPv6 echo reply
 * @param sock Raw socket
 * @param timeout Timeout
 * @param id Echo request id
 * @return true if matching echo reply received, false otherwise
 */
bool icmp6_wait(int sock, int timeout, uint16_t id) {
  struct pollfd pfd = {.fd = sock, .events = POLLIN};
  uint8_t buf[512];

  struct sockaddr_in6 from;
  socklen_t fromlen = sizeof(from);

  while (!terminator && poll(&pfd, 1, timeout) > 0) {
    ssize_t len =
        recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
    if (len < (ssize_t)sizeof(struct icmp6_hdr)) {
      continue;
    }

    struct icmp6_hdr *recv_icmp6 = (struct icmp6_hdr *)buf;
    if (recv_icmp6->icmp6_type == ICMP6_ECHO_REPLY &&
        recv_icmp6->icmp6_id == id) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Calculates the ICMPv6 checksum
 * @param src Source IPv6
 * @param dst Destination IPv6
 * @param data ICMPv6 data to checksum
 * @param len Length of ICMPv6 data
 * @return Calculated value
 */
uint16_t icmp6_checksum(struct in6_addr *src, struct in6_addr *dst, void *data,
                        int len) {
  uint32_t sum = 0;

  uint16_t *s = (uint16_t *)src;
  uint16_t *d = (uint16_t *)dst;
  for (int i = 0; i < 8; i++) {
    sum += s[i];
  }
  for (int i = 0; i < 8; i++) {
    sum += d[i];
  }

  sum += htons(len >> 16);
  sum += htons(len & 0xFFFF);
  sum += htons(IPPROTO_ICMPV6);

  uint16_t *p = (uint16_t *)data;
  while (len > 1) {
    sum += *p++;
    len -= 2;
  }
  if (len == 1) {
    sum += htons(*(uint8_t *)p << 8);
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}
