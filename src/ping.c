#include <arpa/inet.h>
#include <linux/errqueue.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ping.h"

bool run = true;

int init_icmp_socket() {

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (fd < 0) {
    perror("[ERROR][socket]");
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1) {
    perror("[ERROR][setsockopt][SO_RCVTIMEO]");
    close(fd);
    return -1;
  }

  uint8_t size_ttl = TTL_UNIX_SIZE;
  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &size_ttl, sizeof(size_ttl)) == -1) {
    perror("[ERROR][setsockopt][IP_TTL]");
    close(fd);
    return -1;
  }

  uint8_t on = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &on, sizeof(on)) == -1) {
    perror("[ERROR][setsockopt][IP_RECVTTL]");
    close(fd);
    return -1;
  }

  if (setsockopt(fd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) == -1) {
    perror("[ERROR][setsockopt][IP_RECVERR]");
    close(fd);
    return -1;
  }

  return fd;
}

bool dns_resolver(char *hostname, char *ipname, struct sockaddr_in *dest_addr) {

  struct addrinfo info;
  struct addrinfo *result;
  memset(&info, 0, sizeof info);
  info.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &info, &result) != 0) {
    fprintf(stderr, "[ERROR][ping]: %s: No address associated with hostname\n", hostname);
    return false;
  }

  *dest_addr = *(struct sockaddr_in *)result->ai_addr;
  freeaddrinfo(result);

  if (inet_ntop(AF_INET, &dest_addr->sin_addr, ipname, 255) == NULL) {
    perror("[ERROR][inet_ntop]");
    return false;
  }

  return true;
}

bool send_pkt(int socket_fd, struct sockaddr_in *dest_addr, ping_stats *data) {

  icmppkt pkt;

  memset(&pkt, 0, sizeof(pkt));
  memset(&pkt.payload, 'M', sizeof(pkt.payload));
  pkt.header.type = ICMP_ECHO;
  pkt.header.un.echo.id = getpid();
  pkt.header.un.echo.sequence = data->sequence++;
  pkt.header.checksum = checksum(&pkt, sizeof(pkt));
  if (sendto(socket_fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) == -1) {
    perror("[ERROR][setsockopt]");
    return false;
  }
  return true;
}

bool recv_pkt(int socket_fd, ping_stats *data) {

  char buffer[ICMP_HEADER_SIZE + ICMP_PAYLOAD_SIZE];
  struct msghdr msg;
  struct iovec io;
  char control[CMSG_SPACE(sizeof(struct sock_extended_err) + sizeof(int))];
  io.iov_base = buffer;
  io.iov_len = sizeof(buffer);
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  data->bytes_read = recvmsg(socket_fd, &msg, MSG_ERRQUEUE);
  if (data->bytes_read == -1) {
    perror("[ERROR][recvmsg]");
    return false;
  }

  struct cmsghdr *cmsg;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IP) {
      {
        switch (cmsg->cmsg_type) {
        case IP_TTL:
          data->ttl = *CMSG_DATA(cmsg);
          printf("[DEBUG][IP_TTL]\n");
          break;
        case IP_RECVERR:
          printf("[DEBUG][IP_RECVERR]\n");
          break;
        }
      }
    }
  }
  return true;
}

void sigint_handler(int) { run = false; }

bool ping(int socket_fd, struct sockaddr_in *dest_addr, ping_stats *data) {

  struct timespec start, end;
  struct timespec start_program, end_program;
  clock_gettime(CLOCK_MONOTONIC, &start_program);
  signal(SIGINT, sigint_handler);
  print_header_ping(data);
  while (run) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (send_pkt(socket_fd, dest_addr, data) == false)
      return false;
    if (recv_pkt(socket_fd, data) == false)
      return false;
    clock_gettime(CLOCK_MONOTONIC, &end);
    print_body_ping(data, diff_ms(start, end));
    sleep(1);
  }
  clock_gettime(CLOCK_MONOTONIC, &end_program);
  print_footer_ping(data, diff_ms(start_program, end_program));

  return true;
}
