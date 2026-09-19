#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <criterion/criterion.h>

#include "../src/scanner.h"

Test(subnet_edge_suite, test_ipv4_slash_32) {
  Subnet info;
  char ip[] = "10.0.0.1";

  get_subnet4(ip, 32, &info);
  cr_assert_eq(info.hosts, 1, "A /32 IPv4 subnet must yield exactly 1 host");
}

Test(subnet_edge_suite, test_ipv6_slash_128) {
  Subnet info;
  char ip[] = "2001:db8::1";

  get_subnet6(ip, 128, &info);
  cr_assert_eq(info.hosts, 1, "A /128 IPv6 subnet must yield exactly 1 host");
}

Test(subnet_edge_suite, test_ipv6_slash_64) {
  Subnet info;
  char ip[] = "fe80::1";

  get_subnet6(ip, 64, &info);
  cr_assert_eq(info.hosts, ~0ULL,
               "A /64 IPv6 subnet should max out the 64-bit unsigned int");
}
