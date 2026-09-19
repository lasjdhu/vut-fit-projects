/**
 * @file utils.c
 * @brief Utility functions
 * @author Dmitrii Ivanushkin xivanu00
 */

#include "utils.h"

/**
 * @brief Prints usage
 */
void print_help() {
  printf("Usage: ./ipk-L2L3-scan -i INTERFACE [-s SUBNET]... [-w TIMEOUT] [-h "
         "| --help]\n");
}

/**
 * @brief Prints an error message to stderr, then exits
 * @param msg Message
 */
void throw_error(char *msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(EXIT_FAILURE);
}

/**
 * @brief Prints an error message and usage to stderr, then exits
 * @param msg Message
 */
void throw_error_and_help(char *msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  print_help();
  exit(EXIT_FAILURE);
}

/**
 * @brief Arguments parser
 * @param argc Number of arguments
 * @param argv Arguments
 * @param config Config structure
 */
void parse_arguments(int argc, char **argv, Config *config) {
  config->list_interfaces = false;
  config->timeout = 1000;
  config->interface = NULL;
  config->subnet_count = 0;
  memset(config->subnets, 0, sizeof(config->subnets));

  if (argc == 1) {
    throw_error_and_help("No arguments provided");
  }

  if (argc == 2 && strcmp(argv[1], "-i") == 0) {
    config->list_interfaces = true;
    return;
  }

  int opt;
  static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  opterr = 0;

  while ((opt = getopt_long(argc, argv, "hi:s:w:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'h':
      print_help();
      exit(EXIT_SUCCESS);
    case 'i':
      config->interface = optarg;
      break;
    case 'w': {
      char *end;
      long val = strtol(optarg, &end, 10);
      if (*end != '\0' || val <= 0) {
        throw_error_and_help("Invalid timeout value");
      }
      config->timeout = (int)val;
      break;
    }
    case 's':
      if (config->subnet_count >= MAX_SUBNETS) {
        throw_error_and_help("Maximum number of subnets exceeded");
      }
      config->subnets[config->subnet_count++] = optarg;
      break;
    case '?':
      throw_error_and_help("Invalid argument or missing required value");
    }
  }

  if (!config->list_interfaces) {
    if (config->interface == NULL) {
      throw_error_and_help("Interface is required for scanning");
    }

    if (config->subnet_count == 0) {
      throw_error_and_help("At least one subnet is required for scanning");
    }
  }
}
