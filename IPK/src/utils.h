/**
 * @file utils.h
 * @brief Header for utility functions
 * @author Dmitrii Ivanushkin xivanu00
 */

#ifndef UTILS_H
#define UTILS_H

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBNETS 128

/**
 * @brief Configuration structure
 */
typedef struct {
  bool list_interfaces;
  char *interface;
  int timeout;
  char *subnets[MAX_SUBNETS];
  int subnet_count;
} Config;

void print_help();
void parse_arguments(int argc, char **argv, Config *config);
void throw_error(char *msg);
void throw_error_and_help(char *msg);

#endif
