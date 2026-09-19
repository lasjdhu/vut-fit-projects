# DELTA: L2/L3 Scanner

## Project Overview

The application scans for presence of L2 and L3 devices on given network segment(s).
The program discovers what devices are available from a selected range of IP addresses.
It prints to standard output the availability status of the given IP addresses at the L2 and L3 layers.
It supports ICMPv4, ICMPv6, ARP and NDP scans.

## Known limitations

None are known

## Usage

### Build

```bash
make
```

### Run

```bash
./ipk-L2L3-scan -i INTERFACE [-s SUBNET]... [-w TIMEOUT] [-h | --help]
```

### Environment

1. Nix dev shell for C was used
2. Root user privilegies

### Examples

```bash
./ipk-L2L3-scan -i
./ipk-L2L3-scan -h
./ipk-L2L3-scan --help
./ipk-L2L3-scan -i eth0 -w 1000 -s 192.168.0.0/25 -s 192.168.128.0/29
./ipk-L2L3-scan -i eth0 -w 1000 -s fd00:cafe:0000:face::0/120
```

### Example output

```bash
./ipk-L2L3-scan -i eth0 -s 192.168.0.1/30 -s 192.168.0.130/29
```

```text
Scanning ranges:
192.168.0.0/30 2
192.168.0.128/29 6

192.168.0.1 arp OK (00-50-56-f1-c7-1b), icmpv4 OK
192.168.0.2 arp OK (00-22-14-ec-46-bb), icmpv4 FAIL
192.168.0.129 arp FAIL, icmpv4 FAIL
192.168.0.130 arp FAIL, icmpv4 FAIL
192.168.0.131 arp FAIL, icmpv4 FAIL
192.168.0.132 arp FAIL, icmpv4 FAIL
192.168.0.133 arp FAIL, icmpv4 FAIL
192.168.0.134 arp FAIL, icmpv4 FAIL
```

## Implemented Features

### Scanner of Protocols

1. `ARP`: Address Resolution Protocol for IPv4
2. `ICMPv4`: Internet Control Message Protocol for IPv4
3. `NDP`: Neighbor Discovery Protocol for IPv6
4. `ICMPv6`: Internet Control Message Protocol for IPv6

### Misc

1. Multiple subnets are allowed
2. Shutdown via SIGINT or SIGTERM
3. Configurable timeout
4. Interface validation and listing
5. Error handling

## Design Decisions

### Raw Sockets

Application uses raw sockets.
For L2 (ARP/NDP), `AF_PACKET` sockets are used.
For L3 (ICMPv4/v6), `AF_INET` and `AF_INET6` sockets with `IPPROTO_ICMP`/`IPPROTO_ICMPV6` are used.
Timeouts are handled using the `poll()` system call.

### L2/L3 Scanning

The application scans sequentially. For each host it sends L2 request (ARP or NDP).
Then it waits for a reply up to the specified timeout.
Regardless of the L2 result, it scans L3 with echo request (ICMPv4 or ICMPv6).

### Subnet Calculation

Subnet network addresses are calculated by applying a byte-mask array to the parsed IP address.
For any IPv6 prefix of `/64` or larger, the maximum number of iterations is capped at the 64-bit unsigned integer limit (`~0ULL`).

### Checksum Calculation

RFC 1071 standard is implemented for ICMPv4 and ICMPv6 headers.
For L2 the checksums are manually calculated before sending.
For `AF_INET6` sockets, the Linux kernel automatically calculates the ICMPv6 checksum.

## Testing

This project has unit tests (Criterion) and integration tests (Bash).

### Unit tests

1. `args_suite`: Command-line argument parsing
2. `subnet_suite`: Subnet calculation for standard prefixes
3. `subnet_edge_suite`: Edge cases in subnet calculations
4. `error_suite`: Throwing errors with and without help message
5. `signal_suite`: Program termination via SIGINT and SIGTERM

- What was tested: Argument parsing, valid/edge-case subnet calculations, error throwing, and signal handling.
- Why it was tested: To ensure the internal logic correctly parses user constraints, calculates bounds, and safely terminates when requested by the user.
- How it was tested: Using the Criterion framework. Sometimes using dummy sockets.
- Example:
  - Input: `get_subnet4("192.168.0.5", 25, &info)`
  - Expected Output: `info.hosts = 126, info.net = 192.168.0.0`
  - Actual Output: Matches expected output

#### Usage

```bash
make test
```

#### Environment

1. Nix dev shell for C was used (with Criterion)
2. Root user privilegies

#### Example output

```bash
[====] Synthesis: Tested: 14 | Passing: 14 | Failing: 0 | Crashing: 0
```

### Integration tests

1. `Interface Listing`
2. `Live IPv4 Target`: ARP prints OK and identifies MAC, ICMPv4 prints OK
3. `Dead IPv4 Target`: ARP and ICMPv4 scans fail for dead host
4. `Live IPv6 Target`: NDP prints OK and identifies MAC, ICMPv6 prints OK
5. `Dead IPv6 Target`: NDP and ICMPv6 scans fail for dead host

- What was tested: Interface listing, L2/L3 scanning on live and dead targets (both IPv4 and IPv6).
- Why it was tested: To verify that the application correctly formats, sends, and parses network and formats the output properly.
- How it was tested: A Bash script executes the compiled binary against known live and dead IPs on the local network.
- Example:
  - Input: `./ipk-L2L3-scan -i eth0 -w 1000 -s <KNOWN_LIVE_IPV4>/32`
  - Expected Output: `<IP> arp OK (<MAC>), icmpv4 OK`
  - Actual Output: Matches expected string

#### Usage

```bash
chmod +x tests/integration.sh
sudo tests/integration.sh
```

#### Environment

1. Shell script has correct rights to be executed
2. Root user privilegies

#### Example output

```bash
========================================
 Starting L2/L3 Scanner Integration Test
========================================

--- Testing Interface Listing (-i) ---
[PASS] Interface list successfully returned active interfaces (found 'lo')

--- Testing Live IPv4 Target ---
Scanning x.x.x.x on wlan0. Expecting MAC: xx-xx-xx-xx-xx-xx
[PASS] IPv4 ARP Scan successfully identified MAC address
[PASS] IPv4 ICMP Scan successfully pinged target

--- Testing Dead IPv4 Target ---
[PASS] IPv4 correctly reported FAIL for dead host (x.x.x.x)

--- Testing Live IPv6 Target ---
Scanning xxxx::xxxx:xxxx:xxxx:xxxx on wlan0. Expecting MAC: xx-xx-xx-xx-xx-xx
[PASS] IPv6 NDP Scan successfully identified MAC address
[PASS] IPv6 ICMP Scan successfully pinged target

--- Testing Dead IPv6 Target ---
[PASS] IPv6 correctly reported FAIL for dead host (xxxx::xxxx:xxxx:xxxx:xxxx )

========================================
 Integration Results: 7 Passed, 0 Failed
========================================
```

## References

- RFC 1071. Computing the Internet Checksum. Online. 1988. Available from: https://www.rfc-editor.org/rfc/rfc1071. [cit. 2026-04-05].
- Michael Kerrisk. Linux manual pages online. Online. 2026. Available from: https://www.man7.org/linux/man-pages/index.html. [cit. 2026-04-05].
- Geeks for Geeks. Ping in C. Online. 2025. Available from: https://www.geeksforgeeks.org/computer-networks/ping-in-c/. [cit. 2026-04-05].
- Geeks for Geeks. Internet Control Message Protocol (ICMP). Online. 2025. Available from: https://www.geeksforgeeks.org/computer-networks/internet-control-message-protocol-icmp/. [cit. 2026-04-05].
- Open Source For You. A Guide to Using Raw Sockets. Online. 2015. Available from: https://www.opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/. [cit. 2026-04-05].
