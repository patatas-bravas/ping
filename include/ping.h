#pragma once

#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <unistd.h>

#define TTL_UNIX_SIZE 64
#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE sizeof(struct icmphdr)
#define ICMP_PAYLOAD_SIZE 56
#define ICMP_PKT_SIZE (IP_HEADER_SIZE + ICMP_HEADER_SIZE + ICMP_PAYLOAD_SIZE)

#define NO_RECV 0
#define VALID_RECV 1

#define FATAL_ERR -1
#define IGNORE_ERR -2
#define WARNING_ERR -3

#define RECV_BUFFER_SIZE 512

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

} icmp_rtt;

typedef struct {
  int8_t verbose;
  int8_t quiet;
  int8_t count;
  long count_arg;
  int8_t ttl;
  long ttl_arg;
  int8_t interval;
  float interval_arg;
  int8_t timeout;
  double timeout_arg;

} ping_opt;

extern uint8_t run;
extern char *hostname;
extern char ipname[INET_ADDRSTRLEN];
extern ssize_t bytes_read;
extern size_t send_packet;
extern size_t recv_packet;
extern uint8_t err;
extern char buffer[RECV_BUFFER_SIZE];
extern ping_opt opt;
extern icmp_rtt rtt;

int8_t handle_opt(int argc, char **argv);
int8_t dns_resolver(struct sockaddr_in *addr_dest);
socket_t init_icmp_socket();
int8_t ft_ping(socket_t fd, struct sockaddr_in *addr_dest);
