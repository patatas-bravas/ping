#include "printer.h"
#include "types.h"
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void print_header(ping_stats *stats) {

  char *hostname = stats->hostname;
  char *ipname = stats->ipname;
  size_t icmp_payload_size = ICMP_PAYLOAD_SIZE;
  printf("PING %s (%s): %ld stats bytes\n", hostname, ipname, icmp_payload_size);
}

void print_body(ping_stats *stats, float time) {
  size_t bytes_read = stats->bytes_read;
  char *ipname = stats->ipname;
  uint8_t ttl = stats->ttl;
  size_t send_pkt = stats->send_pkt;
  struct icmphdr hdr = stats->recv_hdr_pkt;

  switch (hdr.type) {
  case ICMP_ECHOREPLY:
    printf("%ld bytes from %s: icmp_seq=%ld ttl=%d time=%.3lf ms\n", bytes_read - IP_HEADER_SIZE, ipname, send_pkt, ttl,
           time);
    break;

  case ICMP_DEST_UNREACH:
    switch (hdr.code) {
    case ICMP_NET_UNREACH:
      printf("%ld bytes from %s: Destination Network Unreachable\n", bytes_read, ipname);
      break;
    case ICMP_HOST_UNREACH:
      printf("%ld bytes from %s: Destination Hostname Unreachable\n", bytes_read, ipname);
      break;
    }
    break;
  case ICMP_TIME_EXCEEDED:
    printf("%ld bytes from %s: Time to live exceeded\n", bytes_read, ipname);
    break;
  }
}

void print_footer(ping_stats *stats) {

  printf("--- %s ping statistics ---\n", stats->hostname);
  size_t send_pkt = stats->send_pkt;
  size_t recv_pkt = stats->recv_pkt;
  uint16_t percentage = (stats->send_pkt - stats->recv_pkt) / stats->send_pkt * 100;
  printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n", send_pkt, recv_pkt, percentage);

  double min = stats->rtt.min;
  double max = stats->rtt.max;
  double average = stats->rtt.average;
  double stddev = stats->rtt.stddev;
  printf("round-trip min/avg/max/stddev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", min, average, max, stddev);
}
