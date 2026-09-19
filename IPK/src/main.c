/**
 * @file main.c
 * @brief Main entry point
 * @author Dmitrii Ivanushkin xivanu00
 */

#include "scanner.h"
#include "utils.h"

volatile int terminator = 0;
/**
 * @brief Signal handler for SIGINT and SIGTERM
 * @param sig Signal number
 */
static void handle_terminate_signal(int sig) {
  (void)sig;
  terminator = 1;
}

/**
 * @brief Main
 * @param argc Number of arguments
 * @param argv Arguments
 * @return Exit code
 */
int main(int argc, char **argv) {
  signal(SIGINT, handle_terminate_signal);
  signal(SIGTERM, handle_terminate_signal);

  Config config;
  parse_arguments(argc, argv, &config);
  scan(&config);

  return 0;
}
