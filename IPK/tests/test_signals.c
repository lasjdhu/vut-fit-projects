#define _DEFAULT_SOURCE

#include <criterion/criterion.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../src/arp.h"
#include "../src/icmp4.h"

extern volatile int terminator;

Test(signal_suite, test_terminator_aborts_arp_wait) {
  terminator = 1;

  int dummy_sock = -1;
  uint8_t mac[6];
  bool res = arp_wait(dummy_sock, 0, mac, 5000);

  cr_assert_eq(res, false, "arp_wait should return false when terminated");
}

Test(signal_suite, test_terminator_aborts_icmp4_wait) {
  terminator = 1;

  int dummy_sock = -1;
  bool res = icmp4_wait(dummy_sock, 5000, 1234, 0);

  cr_assert_eq(res, false, "icmp4_wait should return false when terminated");
}
