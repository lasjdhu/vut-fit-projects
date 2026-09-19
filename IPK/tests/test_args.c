#define _DEFAULT_SOURCE

#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <getopt.h>

#include "../src/utils.h"

volatile int terminator = 0;

void setup_args(void) { optind = 1; }

Test(args_suite, test_valid_arguments, .init = setup_args) {
  Config config;
  char *argv[] = {"./ipk-L2L3-scan", "-i", "eth0", "-w", "2000", "-s",
                  "10.0.0.0/24"};

  parse_arguments(7, argv, &config);

  cr_assert_eq(config.timeout, 2000, "Timeout should be parsed as 2000");
  cr_assert_str_eq(config.interface, "eth0", "Interface should be eth0");
  cr_assert_eq(config.subnet_count, 1, "Should have 1 subnet");
  cr_assert_str_eq(config.subnets[0], "10.0.0.0/24", "Subnet string mismatch");
}

Test(args_suite, test_missing_interface, .init = setup_args,
     .exit_code = EXIT_FAILURE) {
  Config config;
  char *argv[] = {"./ipk-L2L3-scan", "-s", "192.168.1.0/24"};

  cr_redirect_stderr();
  cr_redirect_stdout();
  parse_arguments(3, argv, &config);
}

Test(args_suite, test_list_interfaces, .init = setup_args) {
  Config config;
  char *argv[] = {"./ipk-L2L3-scan", "-i"};

  parse_arguments(2, argv, &config);
  cr_assert_eq(config.list_interfaces, true,
               "List interfaces flag should be true");
}

Test(args_suite, test_help, .init = setup_args) {
  Config config;
  char *argv[] = {"./ipk-L2L3-scan", "--help"};

  cr_redirect_stdout();
  parse_arguments(2, argv, &config);
}

Test(args_suite, test_short_help, .init = setup_args,
     .exit_code = EXIT_SUCCESS) {
  Config config;
  char *argv[] = {"./ipk-L2L3-scan", "-h"};

  cr_redirect_stdout();
  parse_arguments(2, argv, &config);
}
