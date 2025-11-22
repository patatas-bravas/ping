#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ping.h"
#include "types.h"

uint8_t run = 1;

socket_t init_icmp_socket() {

  socket_t fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    perror("[ERROR][socket]");
    return FATAL_ERR;
  }

  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1) {
    perror("[ERROR][setsockopt][SO_RCVTIMEO]");
    close(fd);
    return FATAL_ERR;
  }

  uint8_t size_ttl = TTL_UNIX_SIZE;
  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &size_ttl, sizeof(size_ttl)) == -1) {
    perror("[ERROR][setsockopt][IP_TTL]");
    close(fd);
    return FATAL_ERR;
  }

  return fd;
}

int8_t dns_resolver(const char *hostname, char *ipname, struct sockaddr_in *dest_addr) {

  struct addrinfo info;
  struct addrinfo *result;
  memset(&info, 0, sizeof info);
  info.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &info, &result) != 0) {
    fprintf(stderr, "[ERROR][ping]: %s: No address associated with hostname\n", hostname);
    return FATAL_ERR;
  }

  *dest_addr = *(struct sockaddr_in *)result->ai_addr;
  freeaddrinfo(result);

  if (inet_ntop(AF_INET, &dest_addr->sin_addr, ipname, INET_ADDRSTRLEN) == NULL) {
    perror("[ERROR][inet_ntop]");
    return FATAL_ERR;
  }

  return 0;
}

int8_t send_pkt(const socket_t fd, struct sockaddr_in *dest_addr, ping_stats *stats) {

  icmppkt pkt;
  memset(&pkt, 0, sizeof(pkt));
  memset(&pkt.payload, 'M', sizeof(pkt.payload));
  pkt.header.type = ICMP_ECHO;
  pkt.header.un.echo.id = getpid();
  pkt.header.un.echo.sequence = stats->send_pkt++;

  if (sendto(fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) == -1) {
    perror("[ERROR][sendto]");
    return FATAL_ERR;
  }

  return 0;
}

int8_t recv_pkt(const socket_t fd, ping_stats *stats) {
  char buffer[ICMP_PKT_SIZE];
  struct sockaddr_in recv_addr;
  socklen_t recv_addr_size = sizeof(struct sockaddr_in);

  stats->bytes_read = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_size);
  if (stats->bytes_read == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return WARNING_ERR;
    } else {
      perror("[ERROR][recvfrom]");
      return FATAL_ERR;
    }
  }
  struct iphdr *ip = (struct iphdr *)buffer;
  stats->ttl = ip->ttl;
  stats->recv_hdr_pkt = *(struct icmphdr *)(buffer + IP_HEADER_SIZE);
  if (handle_icmp_hdr(buffer, stats) == IGNORE_ERR)
    return IGNORE_ERR;

  if (inet_ntop(AF_INET, &recv_addr.sin_addr, stats->ipname, INET_ADDRSTRLEN) == NULL) {
    perror("[ERROR][inet_ntop]");
    return FATAL_ERR;
  }
  return 0;
}

int8_t handle_icmp_hdr(char *buffer, ping_stats *stats) {
  pid_t recv_pid = 0;

  if (stats->recv_hdr_pkt.type == ICMP_ECHOREPLY) {
    recv_pid = stats->recv_hdr_pkt.un.echo.id;
  } else if (stats->recv_hdr_pkt.type == ICMP_TIME_EXCEEDED || stats->recv_hdr_pkt.type == ICMP_DEST_UNREACH) {
    struct icmphdr *err_hdr = (struct icmphdr *)(buffer + 48);
    recv_pid = err_hdr->un.echo.id;
  }
  if (recv_pid != getpid())
    return IGNORE_ERR;
  if (stats->recv_hdr_pkt.type == ICMP_ECHOREPLY)
    stats->recv_pkt++;
  return 0;
}

void sigint_handler(int code) { run = code - code; }

int8_t ping(const socket_t fd, struct sockaddr_in *dest_addr, ping_stats *stats) {

  struct timespec start, end;
  signal(SIGINT, sigint_handler);
  print_header(stats);
  while (run) {
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (send_pkt(fd, dest_addr, stats) == FATAL_ERR)
      return FATAL_ERR;

    switch (recv_pkt(fd, stats)) {
    case FATAL_ERR:
      return FATAL_ERR;
    case IGNORE_ERR:
      continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double latency = diff_ms(start, end);
    print_body(stats, latency);
    update_rtt(&stats->rtt, latency);
    sleep(1);
  }
  print_footer(stats);

  return 0;
}
