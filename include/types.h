#pragma once

#include <unistd.h>
#include <stdint.h>
#include <netinet/ip_icmp.h>

#define TTL_UNIX_SIZE 64
#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define ICMP_PAYLOAD_SIZE 56

typedef struct {
  struct icmphdr header;
  char payload[ICMP_PAYLOAD_SIZE];

} icmppkt;

typedef struct {
  char *hostname;
  char *ipname;
  ssize_t bytes_read;
  uint8_t ttl;
  uint16_t sequence;
  uint16_t pkt_size;

} ping_stats;

typedef struct {

} ping_opt;
