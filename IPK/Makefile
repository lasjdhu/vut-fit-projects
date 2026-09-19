CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Werror -pedantic -Iinclude

APP_OBJ = src/utils.o src/scanner.o src/arp.o src/icmp4.o src/ndp.o src/icmp6.o
MAIN_OBJ = src/main.o

all: ipk-L2L3-scan

ipk-L2L3-scan: $(APP_OBJ) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -lpcap

NixDevShellName:
	@echo c

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

TEST_SRC = $(wildcard tests/*.c)
TEST_BIN = run_tests

test: $(APP_OBJ)
	$(CC) $(CFLAGS) $(TEST_SRC) $(APP_OBJ) -o $(TEST_BIN) -lcriterion
	./$(TEST_BIN)

clean:
	rm -f ipk-L2L3-scan $(TEST_BIN) src/*.o tests/*.o

.PHONY: all clean test NixDevShellName
