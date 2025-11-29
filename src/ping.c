#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "ping.h"

uint8_t run;
char *hostname;
char ipname[INET_ADDRSTRLEN];
ssize_t bytes_read;
size_t send_packet;
size_t recv_packet;
char *buffer;
uint8_t err;
ping_opt opt;
icmp_rtt rtt;

double diff_ms(const struct timespec start, const struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

void update_rtt(const double latency) {
  static size_t count;
  static double sum_of_square_delta;
  count++;
  if (rtt.min > latency)
    rtt.min = latency;
  if (rtt.max < latency)
    rtt.max = latency;

  double delta = latency - rtt.average;
  rtt.average += delta / count;

  double delta2 = latency - rtt.average;
  sum_of_square_delta += delta * delta2;
  rtt.stddev = sqrt(sum_of_square_delta / count);
}

int8_t handle_opt(int argc, char **argv) {
  char *endptr;
  int current_opt = 0;
  int option_index = 0;

  struct option options[] = {{"verbose", no_argument, 0, 'v'},        {"ttl", required_argument, 0, 't'},
                             {"interval", required_argument, 0, 'i'}, {"size", required_argument, 0, 's'},
                             {"quiet", no_argument, 0, 'q'},          {"count", required_argument, 0, 'c'},
                             {"help", no_argument, 0, 'h'},           {0, 0, 0, 0}};

  while ((current_opt = getopt_long(argc, argv, "v?hqc:t:i:s:", options, &option_index)) != -1) {
    switch (current_opt) {
    case 'v':
      opt.verbose = 1;
      break;

    case 'c':
      opt.count = 1;
      opt.count_arg = strtol(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "[ERROR][ft_ping]: invalid value %s\n", optarg);
        return FATAL_ERR;
      }
      break;

    case 't':
      if (strcmp("0", optarg) == 0) {
        fprintf(stderr, "[ERROR][ft_ping]: option value too small: 0\n");
        return FATAL_ERR;
      }
      opt.ttl = 1;
      opt.ttl_arg = strtol(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "[ERROR][ft_ping]: invalid value %s\n", optarg);
        return FATAL_ERR;
      }
      if (opt.ttl_arg < 1 || opt.ttl_arg > 255 || errno == ERANGE) {
        fprintf(stderr, "[ERROR][ft_ping]: option value too big: %s\n", optarg);
        return FATAL_ERR;
      }
      break;

    case 'i':
      opt.interval = 1;
      opt.interval_arg = strtof(optarg, &endptr);
      if (opt.interval_arg < 0.2 || errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "[ERROR][ft_ping]: option value too small: %s\n", optarg);
        return FATAL_ERR;
      }
      break;

    case 's':
      opt.size = strtol(optarg, &endptr, 10);
      if (errno == ERANGE || opt.size > 65399 || opt.size < 0) {
        fprintf(stderr, "[ERROR][ft_ping]: option value too big: %s\n", optarg);
        return FATAL_ERR;
      }
      break;
    case 'q':
      opt.quiet = 1;
      break;
    case 'h':
    case '?':
      printf("Usage: ft_ping [OPTION...] HOST \n");
      printf("Send ICMP ECHO_REQUEST packets to network hosts.\n\n");
      printf("-?, -h, --help             give this help list\n");
      printf("-v, --verbose              verbose output\n");
      printf("-c, --count=NUMBER         stop after sending NUMBER packets\n");
      printf("-t, --ttl=N                specify N as time-to-live\n");
      printf("-i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n");
      printf("-s, --size=NUMBER          send NUMBER data octets\n");
      printf("-q, --quiet                quiet output\n");
      return FATAL_ERR;

    default:
      fprintf(stderr, "Try 'ft_ping --help' for more information.\n");
      return FATAL_ERR;
    }
  }

  int remaining_arg = 0;
  for (int i = optind; i < argc; i++) {
    remaining_arg++;
  }
  if (remaining_arg != 1) {
    fprintf(stderr, "Try 'ft_ping --help' for more information.\n");
    return FATAL_ERR;
  }

  return 0;
}

void print_header() {
  pid_t pid = getpid();
  if (opt.verbose)
    printf("PING %s (%s): %ld context bytes, id 0x%x = %d\n", hostname, ipname, opt.size, pid, pid);
  else
    printf("PING %s (%s): %ld context bytes\n", hostname, ipname, opt.size);
}

void print_body(const float time) {
  struct iphdr ip = *(struct iphdr *)buffer;
  struct icmphdr icmp = *(struct icmphdr *)(buffer + ip.ihl * 4);
  uint8_t ttl = ip.ttl;

  if (!opt.quiet || err) {

    printf("%ld bytes from %s: ", bytes_read - IP_HEADER_SIZE, ipname);
    switch (icmp.type) {
    case ICMP_ECHOREPLY:
      printf("icmp_seq=%ld ttl=%d time=%.3lf ms\n", send_packet - 1, ttl, time);
      break;
    case ICMP_DEST_UNREACH:
      switch (icmp.code) {
      case ICMP_NET_UNREACH:
        printf("Destination Network Unreachable\n");
        break;
      case ICMP_HOST_UNREACH:
        printf("Destination Host Unreachable\n");
        break;
      }
      break;
    case ICMP_TIME_EXCEEDED:
      printf("Time to live exceeded\n");
      break;
    }
  }

  if (opt.verbose && err) {
    struct iphdr ip_err = *(struct iphdr *)(buffer + (ip.ihl * 4) + ICMP_HEADER_SIZE);
    struct icmphdr icmp_err = *(struct icmphdr *)(buffer + (ip.ihl * 4) + ICMP_HEADER_SIZE + (ip_err.ihl * 4));

    unsigned char *ip_bytes = (unsigned char *)&(ip_err);
    printf("IP Hdr Dump:\n ");
    for (int i = 0; i < IP_HEADER_SIZE; i++) {
      printf("%02x", ip_bytes[i]);
      if (i % 2 == 1)
        printf(" ");
    }
    printf("\n");

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_err.saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_err.daddr, dst_ip, INET_ADDRSTRLEN);

    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data\n");

    uint16_t frag_off = ip_err.frag_off;
    uint8_t flags = (frag_off >> 13) & 0x7;
    uint16_t offset = frag_off & 0x1FFF;

    printf(" %d  %d  %02x %04x %04x   %d %04x  %02x  %02x %04x %s  %s\n", ip_err.version, ip_err.ihl, ip_err.tos,
           ntohs(ip_err.tot_len), ntohs(ip_err.id), flags, offset, ip_err.ttl, ip_err.protocol, ntohs(ip_err.check),
           src_ip, dst_ip);

    printf("ICMP: type %d, code %d, size %ld, id 0x%04x, seq 0x%04x\n", icmp_err.type, icmp_err.code,
           ICMP_HEADER_SIZE + opt.size, icmp_err.un.echo.id, icmp_err.un.echo.sequence);
  }
}

void print_footer() {
  uint16_t percentage;
  if (send_packet == 0)
    percentage = 0;
  else
    percentage = (send_packet - recv_packet) * 100 / send_packet;

  printf("--- %s ping statistics ---\n", hostname);
  printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n", send_packet, recv_packet, percentage);

  double min = rtt.min;
  double max = rtt.max;
  double average = rtt.average;
  double stddev = rtt.stddev;
  if (!err)
    printf("round-trip min/avg/max/stddev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", min, average, max, stddev);
}

socket_t init_icmp_socket() {

  socket_t fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    perror("[ERROR][socket]");
    return FATAL_ERR;
  }

  uint8_t size_ttl;
  if (opt.ttl)
    size_ttl = opt.ttl_arg;
  else
    size_ttl = TTL_UNIX_SIZE;

  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &size_ttl, sizeof(size_ttl)) == -1) {
    perror("[ERROR][setsockopt][IP_TTL]");
    close(fd);
    return FATAL_ERR;
  }

  return fd;
}

int8_t dns_resolver(struct sockaddr_in *dest_addr) {

  struct addrinfo info;
  struct addrinfo *result;
  memset(&info, 0, sizeof info);
  info.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &info, &result) != 0) {
    fprintf(stderr, "[ERROR][ft_ping]: %s: No address associated with hostname\n", hostname);
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

uint16_t checksum(const void *addr, size_t size) {
  unsigned short *ptr = (unsigned short *)addr;
  uint32_t sum = 0;
  while (size > 1) {
    sum += *ptr;
    size -= 2;
    ptr++;
  }
  if (size == 1)
    sum += *(unsigned char *)ptr;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

int8_t send_pkt(const socket_t fd, struct sockaddr_in *dest_addr) {

  size_t size_pkt = opt.size + ICMP_HEADER_SIZE;
  uint8_t *pkt = malloc(size_pkt * sizeof(char));
  if (pkt == NULL) {
    perror("[ERROR][malloc]");
    return FATAL_ERR;
  }

  memset(pkt, 0, size_pkt);
  struct icmphdr *hdr = (struct icmphdr *)pkt;
  char *payload = (char *)pkt + ICMP_HEADER_SIZE;
  memset(payload, 'M', opt.size);
  hdr->type = ICMP_ECHO;
  hdr->un.echo.id = getpid();
  hdr->un.echo.sequence = send_packet++;
  hdr->checksum = checksum(pkt, size_pkt);

  if (sendto(fd, pkt, size_pkt, 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) == -1) {
    perror("[ERROR][sendto]");
    free(pkt);
    return FATAL_ERR;
  }

  free(pkt);
  return 0;
}

int8_t handle_icmp_hdr() {
  pid_t recv_pid = 0;
  struct iphdr ip = *(struct iphdr *)buffer;
  struct icmphdr icmp = *(struct icmphdr *)(buffer + ip.ihl * 4);

  if (icmp.type == ICMP_ECHOREPLY) {
    recv_pid = icmp.un.echo.id;
  } else if (icmp.type == ICMP_TIME_EXCEEDED || icmp.type == ICMP_DEST_UNREACH) {
    struct iphdr err_ip = *(struct iphdr *)(buffer + ip.ihl * 4 + ICMP_HEADER_SIZE);
    struct icmphdr err_hdr = *(struct icmphdr *)(buffer + (ip.ihl * 4) + ICMP_HEADER_SIZE + (err_ip.ihl * 4));
    recv_pid = err_hdr.un.echo.id;
  }

  if (recv_pid != getpid())
    return IGNORE_ERR;

  if (icmp.type == ICMP_ECHOREPLY) {
    recv_packet++;
    err = 0;
  } else
    err = 1;

  return 0;
}

int8_t recv_pkt(const socket_t fd) {
  struct timeval timeout;
  timeout.tv_sec = 2.0;
  timeout.tv_usec = 0;

  fd_set readfd;
  FD_ZERO(&readfd);
  FD_SET(fd, &readfd);

  int8_t n = select(fd + 1, &readfd, NULL, NULL, &timeout);
  if (n == -1) {
    if (errno == EINTR)
      return IGNORE_ERR;
    perror("[ERROR][select]");
    return FATAL_ERR;
  } else if (n == 0)
    return NO_RECV;

  struct sockaddr_in recv_addr;
  socklen_t recv_addr_size = sizeof(struct sockaddr_in);
  bytes_read = recvfrom(fd, buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addr_size);
  if (bytes_read == -1) {
    if (errno == EINTR)
      return IGNORE_ERR;
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
      return NO_RECV;
    else
      return FATAL_ERR;
  }

  if (handle_icmp_hdr() == IGNORE_ERR)
    return IGNORE_ERR;

  if (inet_ntop(AF_INET, &recv_addr.sin_addr, ipname, INET_ADDRSTRLEN) == NULL) {
    perror("[ERROR][inet_ntop]");
    return FATAL_ERR;
  }

  return VALID_RECV;
}

void sigint_handler(int code) {
  (void)code;
  run = 0;
}

int8_t ft_ping(const socket_t fd, struct sockaddr_in *dest_addr) {

  struct timespec start, end, interval;
  if (opt.interval)
    interval.tv_sec = (time_t)opt.interval_arg;
  else
    interval.tv_sec = 1.0;
  interval.tv_nsec = 0;

  signal(SIGINT, sigint_handler);
  print_header();
  while (run) {

    clock_gettime(CLOCK_MONOTONIC, &start);

    if (send_pkt(fd, dest_addr) == FATAL_ERR)
      return FATAL_ERR;

    switch (recv_pkt(fd)) {
    case VALID_RECV:
      clock_gettime(CLOCK_MONOTONIC, &end);
      double latency = diff_ms(start, end);
      update_rtt(latency);
      print_body(latency);
      break;
    case FATAL_ERR:
      return FATAL_ERR;
    case IGNORE_ERR:
      continue;
    }
    if (opt.count && (size_t)opt.count_arg == send_packet)
      break;
    nanosleep(&interval, NULL);
  }
  print_footer();
  return 0;
}
