#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <criterion/criterion.h>

#include "../src/scanner.h"

Test(subnet_suite, test_ipv4_standard_mask) {
  Subnet info;
  char ip[] = "192.168.0.5";

  get_subnet4(ip, 25, &info);

  cr_assert_eq(info.family, AF_INET, "Family should be AF_INET");
  cr_assert_eq(info.prefix, 25, "Prefix should be 25");
  cr_assert_eq(info.hosts, 126, "A /25 subnet should have 126 usable hosts");

  struct in_addr expected_net;
  inet_pton(AF_INET, "192.168.0.0", &expected_net);
  cr_assert_eq(memcmp(info.net, &expected_net.s_addr, 4), 0,
               "Network address mismatch");
}

Test(subnet_suite, test_ipv6_standard_mask) {
  Subnet info;
  char ip[] = "fd00:cafe:0000:face::1";

  get_subnet6(ip, 126, &info);

  cr_assert_eq(info.family, AF_INET6, "Family should be AF_INET6");
  cr_assert_eq(info.prefix, 126, "Prefix should be 126");
  cr_assert_eq(info.hosts, 3, "A /126 subnet should have 3 usable hosts");
}
