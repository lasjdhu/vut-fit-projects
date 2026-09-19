#!/usr/bin/env bash

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

BIN="./ipk-L2L3-scan"
PASSED=0
FAILED=0

echo "========================================"
echo " Starting L2/L3 Scanner Integration Test"
echo "========================================"

if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}[ERROR] Integration tests require raw sockets. Please run as root (sudo).${NC}"
  exit 1
fi

if [ ! -f "$BIN" ]; then
  echo -e "${RED}[ERROR] Binary $BIN not found. Run 'make' first.${NC}"
  exit 1
fi

function print_result() {
    if [ "$1" -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $2"
        ((PASSED++))
    else
        echo -e "${RED}[FAIL]${NC} $2"
        ((FAILED++))
    fi
}

echo -e "\n${YELLOW}--- Testing Interface Listing (-i) ---${NC}"
IFACE_OUT=$($BIN -i 2>/dev/null)
if [ -n "$IFACE_OUT" ] && echo "$IFACE_OUT" | grep -q "lo"; then
    print_result 0 "Interface list successfully returned active interfaces (found 'lo')"
else
    print_result 1 "Interface list failed or did not return expected output"
fi

echo -e "\n${YELLOW}--- Testing Live IPv4 Target ---${NC}"
IPV4_NEIGH=$(ip -4 neigh show | grep -E "REACHABLE|STALE|DELAY" | head -n 1)

if [ -z "$IPV4_NEIGH" ]; then
    echo -e "${YELLOW}[SKIP] No active IPv4 neighbors found in ARP cache to test against.${NC}"
else
    TARGET_IP=$(echo "$IPV4_NEIGH" | awk '{print $1}')
    TARGET_IFACE=$(echo "$IPV4_NEIGH" | awk '{print $3}')
    RAW_MAC=$(echo "$IPV4_NEIGH" | awk '{print $5}')
    TARGET_MAC=$(echo "$RAW_MAC" | tr ':' '-')

    echo "Scanning $TARGET_IP on $TARGET_IFACE. Expecting MAC: $TARGET_MAC"

    OUT=$($BIN -i "$TARGET_IFACE" -s "$TARGET_IP/32" -w 1000)

    if echo "$OUT" | grep -q "arp OK ($TARGET_MAC)"; then
        print_result 0 "IPv4 ARP Scan successfully identified MAC address"
    else
        print_result 1 "IPv4 ARP Scan failed to find expected MAC"
        echo "Output was: $OUT"
    fi

    if echo "$OUT" | grep -q "icmpv4 OK"; then
        print_result 0 "IPv4 ICMP Scan successfully pinged target"
    else
        print_result 1 "IPv4 ICMP Scan failed"
    fi
fi

echo -e "\n${YELLOW}--- Testing Dead IPv4 Target ---${NC}"
DEAD_IP="192.0.2.200"
TEST_IFACE=${TARGET_IFACE:-$(ip route | grep default | awk '{print $5}' | head -n 1)}

OUT=$($BIN -i "$TEST_IFACE" -s "$DEAD_IP/32" -w 500)
if echo "$OUT" | grep -q "arp FAIL, icmpv4 FAIL"; then
    print_result 0 "IPv4 correctly reported FAIL for dead host ($DEAD_IP)"
else
    print_result 1 "IPv4 did not report pure FAIL for dead host"
    echo "Output was: $OUT"
fi

echo -e "\n${YELLOW}--- Testing Live IPv6 Target ---${NC}"
IPV6_NEIGH=$(ip -6 neigh show | grep -E "REACHABLE|STALE|DELAY" | grep -v "router" | head -n 1)
if [ -z "$IPV6_NEIGH" ]; then
    IPV6_NEIGH=$(ip -6 neigh show | grep -E "REACHABLE|STALE|DELAY" | head -n 1)
fi

if [ -z "$IPV6_NEIGH" ]; then
    echo -e "${YELLOW}[SKIP] No active IPv6 neighbors found in NDP cache to test against.${NC}"
else
    TARGET_IP6=$(echo "$IPV6_NEIGH" | awk '{print $1}')
    TARGET_IFACE6=$(echo "$IPV6_NEIGH" | awk '{print $3}')
    RAW_MAC6=$(echo "$IPV6_NEIGH" | awk '{print $5}')
    TARGET_MAC6=$(echo "$RAW_MAC6" | tr ':' '-')

    echo "Scanning $TARGET_IP6 on $TARGET_IFACE6. Expecting MAC: $TARGET_MAC6"

    OUT6=$($BIN -i "$TARGET_IFACE6" -s "$TARGET_IP6/128" -w 1000)

    if echo "$OUT6" | grep -q "ndp OK ($TARGET_MAC6)"; then
        print_result 0 "IPv6 NDP Scan successfully identified MAC address"
    else
        print_result 1 "IPv6 NDP Scan failed to find expected MAC"
        echo "Output was: $OUT6"
    fi

    if echo "$OUT6" | grep -q "icmpv6 OK"; then
        print_result 0 "IPv6 ICMP Scan successfully pinged target"
    else
        print_result 1 "IPv6 ICMP Scan failed"
    fi
fi

echo -e "\n${YELLOW}--- Testing Dead IPv6 Target ---${NC}"
DEAD_IP6="2001:db8:ffff:ffff:ffff:ffff:ffff:ffff"
TEST_IFACE6=${TARGET_IFACE6:-$TEST_IFACE}

OUT6=$($BIN -i "$TEST_IFACE6" -s "$DEAD_IP6/128" -w 500)
if echo "$OUT6" | grep -q "ndp FAIL, icmpv6 FAIL"; then
    print_result 0 "IPv6 correctly reported FAIL for dead host ($DEAD_IP6)"
else
    print_result 1 "IPv6 did not report pure FAIL for dead host"
    echo "Output was: $OUT6"
fi

echo -e "\n========================================"
echo -e " Integration Results: ${GREEN}$PASSED Passed${NC}, ${RED}$FAILED Failed${NC}"
echo "========================================"

if [ "$FAILED" -gt 0 ]; then
    exit 1
else
    exit 0
fi
