#pragma once

#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define TTL_UNIX_SIZE 1
#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define ICMP_PAYLOAD_SIZE 56
#define ICMP_PKT_SIZE (IP_HEADER_SIZE + ICMP_HEADER_SIZE + ICMP_PAYLOAD_SIZE)

#define FATAL_ERR -1
#define IGNORE_ERR -2
#define WARNING_ERR -3

typedef int socket_t;

typedef struct {
  struct icmphdr header;
  char payload[ICMP_PAYLOAD_SIZE];

} icmppkt;

typedef struct {
  double min;
  double max;
  double average;
  double stddev;

} rtt;

typedef struct {
  char *hostname;
  char ipname[INET_ADDRSTRLEN];
  ssize_t bytes_read;
  uint8_t ttl;
  size_t send_pkt;
  size_t recv_pkt;
  struct icmphdr recv_hdr_pkt;
  rtt rtt;

} ping_stats;

typedef struct {

} ping_opt;
