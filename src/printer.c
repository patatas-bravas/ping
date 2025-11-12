#include "printer.h"

void print_header_ping(ping_stats *data) {

  char *hostname = data->hostname;
  char *ipname = data->ipname;
  size_t icmp_payload_size = data->pkt_size - ICMP_HEADER_SIZE - IP_HEADER_SIZE;
  printf("PING %s (%s): %ld data bytes\n", hostname, ipname, icmp_payload_size);
}

void print_body_ping(ping_stats *data, float time) {
  size_t bytes_read = data->bytes_read;
  char *ipname = data->ipname;
  uint8_t ttl = data->ttl;
  uint16_t sequence = data->sequence;
  printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3lf ms\n", bytes_read, ipname, sequence, ttl, time);
}

void print_footer_ping(ping_stats *data, double time) {

  printf("--- %s ping statistics ---\n", data->hostname);
  printf("%d pkts transmitted, %d received, 0%% pkt loss, time %.0lfms", data->sequence, data->sequence, time);
}
