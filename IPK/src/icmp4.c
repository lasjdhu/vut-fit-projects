/**
 * @file icmp4.c
 * @brief ICMPv4 scanning
 * @author Dmitrii Ivanushkin xivanu00
 */

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <unistd.h>

#include "icmp4.h"

extern int terminator;

/**
 * @brief Sends an ICMPv4 echo request
 * @param sock Raw socket
 * @param target_ip Target IPv4
 * @param timeout Timeout
 * @return true if echo reply received, false otherwise
 */
bool icmp4_ping(int sock, uint32_t target_ip, int timeout) {
  struct icmphdr icmp;
  memset(&icmp, 0, sizeof(icmp));

  icmp.type = ICMP_ECHO;
  icmp.code = 0;
  uint16_t id = htons(getpid() & 0xFFFF);
  icmp.un.echo.id = id;
  icmp.un.echo.sequence = 0;
  icmp.checksum = icmp4_checksum(&icmp, sizeof(icmp));

  struct sockaddr_in sa = {.sin_family = AF_INET, .sin_addr.s_addr = target_ip};
  sendto(sock, &icmp, sizeof(icmp), 0, (struct sockaddr *)&sa, sizeof(sa));

  return icmp4_wait(sock, timeout, id, target_ip);
}

/**
 * @brief Waits for ICMPv4 echo reply
 * @param sock Raw socket
 * @param timeout Timeout
 * @param id Echo request id
 * @param target_ip IP of the reply
 * @return true if matching echo reply received, false otherwise
 */
bool icmp4_wait(int sock, int timeout, uint16_t id, uint32_t target_ip) {
  struct pollfd pfd = {.fd = sock, .events = POLLIN};
  uint8_t buf[512];

  while (!terminator && poll(&pfd, 1, timeout) > 0) {
    ssize_t len = recv(sock, buf, sizeof(buf), 0);
    if (len < (ssize_t)(sizeof(struct iphdr) + sizeof(struct icmphdr))) {
      continue;
    }

    struct iphdr *ip = (struct iphdr *)buf;
    if (ip->saddr != target_ip) {
      continue;
    }

    struct icmphdr *recv_icmp = (struct icmphdr *)(buf + (ip->ihl * 4));

    if (recv_icmp->type == ICMP_ECHOREPLY && recv_icmp->un.echo.id == id) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Calculates the ICMPv4 checksum
 * @param data Data to checksum
 * @param len Length of data
 * @return Calculated value
 */
uint16_t icmp4_checksum(void *data, int len) {
  uint16_t *buf = data;
  uint32_t sum = 0;

  for (sum = 0; len > 1; len -= 2) {
    sum += *buf++;
  }

  if (len == 1) {
    sum += *(uint8_t *)buf;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  return (uint16_t)(~sum);
}
