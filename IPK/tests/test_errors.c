#define _DEFAULT_SOURCE

#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "../src/utils.h"

Test(error_suite, test_throw_error, .exit_code = EXIT_FAILURE) {
  cr_redirect_stderr();
  throw_error("Simulated error");
}

Test(error_suite, test_throw_error_and_help, .exit_code = EXIT_FAILURE) {
  cr_redirect_stderr();
  cr_redirect_stdout();
  throw_error_and_help("Simulated error with usage prompt");
}
